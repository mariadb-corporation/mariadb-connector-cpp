/************************************************************************************
   Copyright (C) 2022,2025 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/


#include "mysql.h"
#include "ClientPrepareResult.h"
#include "Parameter.h"
#include "Protocol.h"


namespace mariadb
{
  const SQLString SpecChars("();><=-+,");
  const char QUOTE= '\'';
  static const char DBL_QUOTE= '"';
  static const char ZERO_BYTE= '\0';
  static const char BACKSLASH= '\\';

  void escapeData(const char* in, std::size_t len, bool noBackslashEscapes, SQLString& out)
  {
    if (out.capacity() - out.length() > len * 2) {
      out.reserve(out.length() + len*2);
    }
    
    if (noBackslashEscapes) {
      for (size_t i= 0; i < len; i++) {
        if (QUOTE == in[i]) {
          out.push_back(QUOTE);
        }
        out.push_back(in[i]);
      }
    }
    else {
      for (size_t i= 0; i < len; i++) {
        if (in[i] == QUOTE
          || in[i] == BACKSLASH
          || in[i] == DBL_QUOTE
          || in[i] == ZERO_BYTE) {
          out.push_back('\\');
        }
        out.push_back(in[i]);
      }
    }
  }


  ClientPrepareResult::ClientPrepareResult(
    const SQLString& _sql,
    std::vector<std::pair<std::size_t, std::size_t>>& _queryParts,
    bool _isQueryMultiValuesRewritable,
    bool _isQueryMultipleRewritable,
    bool _rewriteType,
    bool _noBackslashEscapes)
    : sql(_sql)
    , queryParts(_queryParts)
    , isQueryMultiValuesRewritableFlag(_isQueryMultiValuesRewritable)
    , isQueryMultipleRewritableFlag(_isQueryMultipleRewritable)
    , paramCount(static_cast<uint32_t>(queryParts.size()) - (_rewriteType ? 3 : 1))
    , rewriteType(_rewriteType)
    , noBackslashEscapes(_noBackslashEscapes)
  {
  }


  /**
    * Valid that query is valid (no ending semi colon, or end-of line comment ).
    *
    * @param queryString query
    * @param noBackslashEscapes escape
    * @return valid flag
    */
  bool ClientPrepareResult::canAggregateSemiColon(const SQLString& queryString, bool noBackslashEscapes)
  {
    LexState state= LexState::Normal;
    char lastChar= '\0';

    bool singleQuotes= false;
    bool endingSemicolon= false;

    for (char car : queryString) {

      if (state == LexState::Escape
        &&!((car == '\''&&singleQuotes)||(car == '"'&&!singleQuotes))) {
        state= LexState::SqlString;
        lastChar= car;
        continue;
      }

      switch (car) {
      case '*':
        if (state == LexState::Normal &&lastChar == '/') {
          state= LexState::SlashStarComment;
        }
        break;

      case '/':
        if (state == LexState::SlashStarComment &&lastChar == '*') {
          state= LexState::Normal;
        }
        break;

      case '#':
        if (state == LexState::Normal) {
          state= LexState::EOLComment;
        }
        break;

      case '-':
        if (state == LexState::Normal &&lastChar == '-') {
          state= LexState::EOLComment;
        }
        break;
      case ';':
        if (state == LexState::Normal) {
          endingSemicolon= true;
        }
        break;
      case '\n':
        if (state == LexState::EOLComment) {
          state= LexState::Normal;
        }
        break;
      case '"':
        if (state == LexState::Normal) {
          state= LexState::SqlString;
          singleQuotes= false;
        }
        else if (state == LexState::SqlString &&!singleQuotes) {
          state= LexState::Normal;
        }
        else if (state == LexState::Escape &&!singleQuotes) {
          state= LexState::SqlString;
        }
        break;

      case '\'':
        if (state == LexState::Normal) {
          state= LexState::SqlString;
          singleQuotes= true;
        }
        else if (state == LexState::SqlString &&singleQuotes) {
          state= LexState::Normal;
        }
        else if (state == LexState::Escape &&singleQuotes) {
          state= LexState::SqlString;
        }
        break;

      case '\\':
        if (noBackslashEscapes) {
          break;
        }
        if (state == LexState::SqlString) {
          state= LexState::Escape;
        }
        break;
      case '`':
        if (state == LexState::Backtick) {
          state= LexState::Normal;
        }
        else if (state == LexState::Normal) {
          state= LexState::Backtick;
        }
        break;
      default:

        if (state == LexState::Normal && endingSemicolon && ((int8_t)car >=40)) {
          endingSemicolon= false;
        }
        break;
      }
      lastChar= car;
    }
    return state != LexState::EOLComment && !endingSemicolon;
  }


  std::pair<std::size_t, std::size_t>& partAppend(std::pair<std::size_t, std::size_t>& part, std::size_t curPos, std::size_t len= 1) {
    // First one should begin from 0
    if (part.second == 0) { 
      part.first= curPos;
    }
    part.second+= len;
    return part;
  }

  // Need evaluation for max vector size
  void smartPush(std::vector<std::pair<std::size_t, std::size_t>>& partList, std::pair<std::size_t, std::size_t>& item) {
    if (partList.capacity() <= partList.size() + 1 + 4/* max we might need later*/) {
      partList.reserve(partList.size() * 2);
    }
    partList.push_back(item);
  }

  /**
    * Separate query in a String list and set flag isQueryMultiValuesRewritable The parameters "?"
    * (not in comments) emplacements are to be known.
    *
    * <p>The only rewritten queries follow these notation: INSERT [LOW_PRIORITY | DELAYED |
    * HIGH_PRIORITY] [IGNORE] [INTO] tbl_name [PARTITION (partition_list)] [(col,...)] {VALUES |
    * VALUE} (...) [ ON DUPLICATE KEY UPDATE col=expr [, col=expr] ... ] With expr without parameter.
    *
    * <p>Query with LAST_INSERT_ID() will not be rewritten
    *
    * <p>INSERT ... SELECT will not be rewritten.
    *
    * <p>String list :
    *
    * <ul>
    *   <li>pre value part
    *   <li>After value and first parameter part
    *   <li>for each parameters :
    *       <ul>
    *         <li>part after parameter and before last parenthesis
    *         <li>Last query part
    *       </ul>
    * </ul>
    *
    * <p>example : INSERT INTO TABLE(col1,col2,col3,col4, col5) VALUES (9, ?, 5, ?, 8) ON DUPLICATE
    * KEY UPDATE col2=col2+10
    *
    * <ul>
    *   <li>pre value part : INSERT INTO TABLE(col1,col2,col3,col4, col5) VALUES
    *   <li>after value part : "(9 "
    *   <li>part after parameter 1: ", 5," - ", 5," - ",8)"
    *   <li>last part : ON DUPLICATE KEY UPDATE col2=col2+10
    * </ul>
    *
    * <p>With 2 series of parameters, this query will be rewritten like [INSERT INTO
    * TABLE(col1,col2,col3,col4, col5) VALUES][ (9, param0_1, 5, param0_2, 8)][, (9, param1_1, 5,
    * param1_2, 8)][ ON DUPLICATE KEY UPDATE col2=col2+10]
    *
    * @param queryString query String
    * @param noBackslashEscapes must backslash be escaped.
    * @return List of query part.
    */
  ClientPrepareResult* ClientPrepareResult::rewritableParts(const SQLString& queryString, bool noBackslashEscapes)
  {
    bool reWritablePrepare= true;
    bool multipleQueriesPrepare= true;
    std::vector<std::pair<std::size_t, std::size_t>> partList;
    LexState state= LexState::Normal;
    char lastChar= '\0';

    std::pair<std::size_t, std::size_t> sb(0,0);
    partList.reserve(std::max(std::size_t(10), queryString.length()/64)); // TODO: consts with some meaningful names in appropriate place
    std::pair<std::size_t, std::size_t> preValuePart1;
    std::pair<std::size_t, std::size_t> preValuePart2;
    std::pair<std::size_t, std::size_t> postValuePart;

    bool singleQuotes= false, preValuePart2isNotSet= true; //that preValuePart2 is kind of null

    int32_t isInParenthesis= 0;
    bool skipChar= false;
    bool isFirstChar= true;
    bool isInsert= false;
    bool semicolon= false;
    bool hasParam= false;

    const char* query= queryString.c_str();
    size_t queryLength= queryString.length();

    for (size_t i= 0; i < queryLength; i++) {

      char car= query[i];
      if (state == LexState::Escape
        && !((car == '\'' && singleQuotes) || (car == '"' && !singleQuotes))) {
        partAppend(sb, i);
        lastChar= car;
        state= LexState::SqlString;
        continue;
      }

      switch (car) {
      case '*':
        if (state == LexState::Normal && lastChar == '/') {
          state= LexState::SlashStarComment;
        }
        break;
      case '/':
        if (state == LexState::SlashStarComment && lastChar == '*') {
          state= LexState::Normal;
        }
        break;

      case '#':
        if (state == LexState::Normal) {
          state= LexState::EOLComment;
        }
        break;

      case '-':
        if (state == LexState::Normal && lastChar == '-') {
          state= LexState::EOLComment;
          multipleQueriesPrepare= false;
        }
        break;

      case '\n':
        if (state == LexState::EOLComment) {
          state= LexState::Normal;
        }
        break;

      case '"':
        if (state == LexState::Normal) {
          state= LexState::SqlString;
          singleQuotes= false;
        }
        else if (state == LexState::SqlString && !singleQuotes) {
          state= LexState::Normal;
        }
        else if (state == LexState::Escape && !singleQuotes) {
          state= LexState::SqlString;
        }
        break;
      case ';':
        if (state == LexState::Normal) {
          semicolon= true;
          multipleQueriesPrepare= false;
        }
        break;
      case '\'':
        if (state == LexState::Normal) {
          state= LexState::SqlString;
          singleQuotes= true;
        }
        else if (state == LexState::SqlString && singleQuotes) {
          state= LexState::Normal;
        }
        else if (state == LexState::Escape && singleQuotes) {
          state= LexState::SqlString;
        }
        break;

      case '\\':
        if (noBackslashEscapes) {
          break;
        }
        if (state == LexState::SqlString) {
          state= LexState::Escape;
        }
        break;

      case '?':
        if (state == LexState::Normal) {
          hasParam= true;
          if (preValuePart1.second == 0) {
            preValuePart1= sb;
            sb= {0,0};
          }
          if (preValuePart2isNotSet) {
            preValuePart2= sb;
            sb= {0,0};
            preValuePart2isNotSet= false;
          }
          else {
            if (postValuePart.second != 0) {

              reWritablePrepare= false;

              sb.first= postValuePart.first;
              sb.second+= postValuePart.second;
              postValuePart= {0,0};
            }
            smartPush(partList, sb);
            sb= {0,0};
          }

          skipChar= true;
        }
        break;
      case '`':
        if (state == LexState::Backtick) {
          state= LexState::Normal;
        }
        else if (state == LexState::Normal) {
          state= LexState::Backtick;
        }
        break;

      case 's':
      case 'S':
        if (state == LexState::Normal
          && postValuePart.second == 0
          && queryLength > i + 7
          && (query[i + 1] == 'e' || query[i + 1] == 'E')
          && (query[i + 2] == 'l' || query[i + 2] == 'L')
          && (query[i + 3] == 'e' || query[i + 3] == 'E')
          && (query[i + 4] == 'c' || query[i + 4] == 'C')
          && (query[i + 5] == 't' || query[i + 5] == 'T')) {

          if (i > 0 && (query[i - 1] > ' ' && SpecChars.find_first_of(query[i - 1]) == std::string::npos)) {
            break;
          }
          if (query[i + 6] > ' ' && SpecChars.find_first_of(query[i + 6]) == std::string::npos) {
            break;
          }
          reWritablePrepare= false;
        }
        break;
      case 'v':
      case 'V':
        if (state == LexState::Normal
          && preValuePart1.second == 0
          && (lastChar == ')' || ((int8_t)lastChar <= 40))
          && queryLength > i +7
          && (query[i + 1] == 'a' || query[i + 1] == 'A')
          && (query[i + 2] == 'l' || query[i + 2] == 'L')
          && (query[i + 3] == 'u' || query[i + 3] == 'U')
          && (query[i + 4] == 'e' || query[i + 4] == 'E')
          && (query[i + 5] == 's' || query[i + 5] == 'S')
          && (query[i + 6] == '(' || ((int8_t)query[i + 6] <= 40))) {
          partAppend(sb, i, 6);
          /*streamAppend(sb, query[i + 1]);
          streamAppend(sb, query[i + 2]);
          streamAppend(sb, query[i + 3]);
          streamAppend(sb, query[i + 4]);
          streamAppend(sb, query[i + 5]);*/
          i= i + 5;
          preValuePart1= sb;
          sb= {0,0};
          skipChar= true;
        }
        break;
      case 'l':
      case 'L':
        if (state == LexState::Normal
          && queryLength > i + 14
          && (query[i + 1] == 'a' || query[i + 1] == 'A')
          && (query[i + 2] == 's' || query[i + 2] == 'S')
          && (query[i + 3] == 't' || query[i + 3] == 'T')
          && query[i + 4] == '_'
          && (query[i + 5] == 'i' || query[i + 5] == 'I')
          && (query[i + 6] == 'n' || query[i + 6] == 'N')
          && (query[i + 7] == 's' || query[i + 7] == 'S')
          && (query[i + 8] == 'e' || query[i + 8] == 'E')
          && (query[i + 9] == 'r' || query[i + 9] == 'R')
          && (query[i + 10] == 't' || query[i + 10] == 'T')
          && query[i + 11] == '_'
          && (query[i + 12] == 'i' || query[i + 12] == 'I')
          && (query[i + 13] == 'd' || query[i + 13] == 'D')
          && query[i + 14] == '(') {
          partAppend(sb, i);
          reWritablePrepare= false;
          skipChar= true;
        }
        break;
      case '(':
        if (state == LexState::Normal) {
          isInParenthesis++;
        }
        break;
      case ')':
        if (state == LexState::Normal) {
          isInParenthesis--;
          if (isInParenthesis == 0 && preValuePart2.second != 0 && postValuePart.second == 0) {
            partAppend(sb, i);
            postValuePart= sb;
            sb= {0,0};
            skipChar= true;
          }
        }
        break;
      default:
        if (state == LexState::Normal && isFirstChar && ((int8_t)car >= 40)) {
          if (car == 'I'||car == 'i') {
            isInsert= true;
          }
          isFirstChar= false;
        }

        if (state == LexState::Normal && semicolon && ((int8_t)car >= 40)) {
          reWritablePrepare= false;
          multipleQueriesPrepare= true;
        }
        break;
      }

      lastChar= car;
      if (skipChar) {
        skipChar= false;
      }
      else {
        partAppend(sb, i);
      }
    }
    // We have 3 or 4 more entries to add 
    partList.reserve(partList.size() + 3 + (hasParam ? 1 : 0));
    if (!hasParam) {

      if (preValuePart1.second == 0) {
        partList.insert(partList.begin(), sb);
        partList.insert(partList.begin(), {0,0});
      }
      else {

        partList.insert(partList.begin(), preValuePart1);
        partList.insert(partList.begin(), sb);
      }
      sb= {0,0};
    }
    else {
      partList.insert(partList.begin(), preValuePart1);
      partList.insert(partList.begin(), preValuePart2);
    }

    if (!isInsert) {
      reWritablePrepare= false;
    }

    if (hasParam) {
      partList.push_back(postValuePart);
    }
    partList.push_back(sb);

    return new ClientPrepareResult(
      queryString, partList, reWritablePrepare, multipleQueriesPrepare, true, noBackslashEscapes);
  }


  const SQLString& ClientPrepareResult::getSql() const
  {
    return sql;
  }


  const std::vector<std::pair<std::size_t, std::size_t>>& ClientPrepareResult::getQueryParts() const
  {
    return queryParts;
  }


  bool ClientPrepareResult::isQueryMultiValuesRewritable() const
  {
    return isQueryMultiValuesRewritableFlag;
  }

  bool ClientPrepareResult::isQueryMultipleRewritable() const
  {
    return isQueryMultipleRewritableFlag;
  }

  bool ClientPrepareResult::isRewriteType() const
  {
    return rewriteType;
  }

  std::size_t ClientPrepareResult::getParamCount() const
  {
    return paramCount;
  }


  std::size_t estimatePreparedQuerySize(const ClientPrepareResult* clientPrepareResult, const std::vector<std::pair<std::size_t, std::size_t>>& queryPart,
    MYSQL_BIND* parameters)
  {
    std::size_t estimate= queryPart.front().second + 1/* for \0 */, offset= 0;
    if (clientPrepareResult->isRewriteType()) {
      estimate += queryPart[1].second + queryPart[clientPrepareResult->getParamCount() + 2].second;
      offset= 1;
    }
    for (uint32_t i= 0; i < clientPrepareResult->getParamCount(); ++i) {
      estimate += (parameters)[i].buffer_length*2 + 2; // The string needs to be escaped - all characters in the worst case,
                                                       // + 2 quotes
      estimate += queryPart[i + 1 + offset].second;
    }
    estimate= ((estimate + 7) / 8) * 8;
    return estimate;
  }


  void assemblePreparedQueryForExec(
    SQLString& out,
    const ClientPrepareResult* clientPrepareResult,
    MYSQL_BIND* parameters,
    std::map<uint32_t,std::string>& longData,
    bool noBackSlashEscapes)
  {
    const std::vector<std::pair<std::size_t, std::size_t>>& queryPart= clientPrepareResult->getQueryParts();
    const SQLString &query= clientPrepareResult->getSql();
    const auto paramCount= clientPrepareResult->getParamCount();

    for (auto pair : longData) {
      if (parameters[pair.first].buffer == nullptr) {
        parameters[pair.first].buffer= const_cast<void*>(static_cast<const void*>(pair.second.data()));
        parameters[pair.first].buffer_length= static_cast<unsigned long>(pair.second.length());
        parameters[pair.first].buffer_type= MYSQL_TYPE_BLOB;
      }
    }
    std::size_t estimate= estimatePreparedQuerySize(clientPrepareResult, queryPart, parameters);

    if (estimate > out.capacity() - out.length()) {
      out.reserve(out.length() + estimate);
    }
    if (clientPrepareResult->isRewriteType()) {

      out.append(query.c_str() + queryPart[1].first, queryPart[1].second);
      out.append(query.c_str() + queryPart[0].first, queryPart[0].second);

      for (uint32_t i= 0; i < clientPrepareResult->getParamCount(); i++) {
        Parameter::toString(out, parameters[i], noBackSlashEscapes);
        out.append(query.c_str() + queryPart[i + 2].first, queryPart[i + 2].second);
      }
      out.append(query.c_str() + queryPart[paramCount + 2].first, queryPart[paramCount + 2].second);
    }
    else {
      out.append(query.c_str() + queryPart.front().first, queryPart.front().second);
      for (uint32_t i= 0; i < clientPrepareResult->getParamCount(); i++) {
        Parameter::toString(out, parameters[i], noBackSlashEscapes);
        out.append(query.c_str() + queryPart[i + 1].first, queryPart[i + 2].second);
      }
    }
  }


  SQLString& ClientPrepareResult::assembleQuery(SQLString& sql, MYSQL_BIND* parameters, std::map<uint32_t, std::string> &longData) const
  {
    if (getParamCount() == 0) {
      return sql.append(this->sql);
    }
    assemblePreparedQueryForExec(sql, this, parameters, longData, noBackslashEscapes);
    return sql;
  }


  bool skipParamRow(MYSQL_BIND* param, std::size_t paramCount, std::size_t row)
  {
    for (std::size_t col= 0; col < paramCount; ++col) {
      if (param[col].u.indicator != nullptr && param[col].u.indicator[row] == STMT_INDICATOR_IGNORE_ROW) {
        return true;
      }
    }
    return false;
  }

  /* Constructs using parameter arrays INSERT with multiple VALUES sets(i.e. VALUES(...),(...),...) */
  std::size_t assembleMultiValuesQuery(SQLString& pos, const ClientPrepareResult* clientPrepareResult,
    MYSQL_BIND* parameters, uint32_t arraySize, std::size_t currentIndex, bool noBackslashEscapes)
  {
    std::size_t index= currentIndex, capacity= pos.capacity(), estimatedLength= 0;
    const std::vector<std::pair<std::size_t, std::size_t>> &queryParts= clientPrepareResult->getQueryParts();
    const std::size_t paramCount= clientPrepareResult->getParamCount();
    const char *query= clientPrepareResult->getSql().c_str();

    pos.append(query + queryParts[1].first, queryParts[1].second); // firstPart
    pos.append(query + queryParts.front().first, queryParts.front().second); // secondPart)
    size_t lastPartLength= queryParts[paramCount + 2].second;
    size_t intermediatePartLength= queryParts[1].second;

    while (skipParamRow(parameters, paramCount, index)) { ++index; }
    estimatedLength= pos.length();
    for (size_t i= 0; i < paramCount; ++i) {
      Parameter::toString(pos, parameters[i], index, noBackslashEscapes);
      pos.append(query + queryParts[i + 2].first, queryParts[i + 2].second);
      intermediatePartLength+= queryParts[i + 2].second;
    }
    ++index;
    // Now we have one paramset length in estimatedLength, that we can take for estimation
    estimatedLength= pos.length() + (pos.length() - estimatedLength)*(arraySize - index);
    if (estimatedLength > capacity) {
      pos.reserve(((std::min(Protocol::MAX_PACKET_LENGTH, static_cast<int64_t>((estimatedLength)) + 7) / 8) * 8));
    }

    while (index < arraySize) {

      if (skipParamRow(parameters, paramCount, index)) {
        ++index;
        continue;
      }
      int64_t parameterLength= 0;
      bool knownParameterSize= true;
      for (size_t i= 0; i < paramCount; ++i) {
        int64_t paramSize= Parameter::getApproximateStringLength(parameters[i], index);
        if (paramSize == -1) {
          knownParameterSize= false;
          break;
        }
        parameterLength+= paramSize;
      }

      if (knownParameterSize) {

        if (Protocol::checkRemainingSize(pos.length() + 1 + parameterLength + intermediatePartLength + lastPartLength)) {
          pos.append(1, ',');
          pos.append(query + queryParts.front().first, queryParts.front().second);

          for (size_t i= 0; i < paramCount; i++) {
            Parameter::toString(pos, parameters[i], index, noBackslashEscapes);
            pos.append(query + queryParts[i + 2].first, queryParts[i + 2].second);
          }
          ++index;
        }
        else {
          break;
        }
      }
      else {
        pos.append(1, ',');
        pos.append(query + queryParts.front().first, queryParts.front().second);

        for (size_t i= 0; i < paramCount; i++) {
          Parameter::toString(pos, parameters[i], index, noBackslashEscapes);
          pos.append(query + queryParts[i + 2].first, queryParts[i + 2].second);
        }
        ++index;
        break;
      }
    }
    pos.append(query + queryParts[paramCount + 2].first, queryParts[paramCount + 2].second);
    return index;
  }

  /* Assembles query batch using parameter arrays */
  std::size_t assembleBatchRewriteQuery(SQLString& pos, const ClientPrepareResult* clientPrepareResult,
    MYSQL_BIND* parameters, uint32_t arraySize, std::size_t currentIndex, bool noBackslashEscapes)
  {
    std::size_t index= currentIndex, capacity= pos.capacity(), estimatedLength;
    const std::vector<std::pair<size_t, size_t>> queryParts= clientPrepareResult->getQueryParts();
    const SQLString &query= clientPrepareResult->getSql();
    const std::size_t paramCount= clientPrepareResult->getParamCount();
    const auto& firstPart= queryParts[1];
    const auto& secondPart= queryParts.front();

    pos.append(query, firstPart.first, firstPart.second);
    pos.append(query, secondPart.first, secondPart.second);

    std::size_t staticLength= 1;
    for (auto& queryPart : queryParts) {
      staticLength += queryPart.second;
    }

    for (size_t i= 0; i < paramCount; ++i) {
      Parameter::toString(pos, parameters[i], index, noBackslashEscapes);
      pos.append(query, queryParts[i + 2].first, queryParts[i + 2].second);
    }
    pos.append(query, queryParts[paramCount + 2].first, queryParts[paramCount + 2].second);
    ++index;
    estimatedLength= pos.length() * (arraySize - currentIndex);
    if (estimatedLength > capacity) {
      pos.reserve(((std::min<std::size_t>(Protocol::MAX_PACKET_LENGTH, estimatedLength) + 7) / 8) * 8);
    }

    while (index < arraySize) {
      int64_t parameterLength= 0;
      bool knownParameterSize= true;

      for (std::size_t i= 0; i < paramCount; ++i) {
        int64_t paramSize= Parameter::getApproximateStringLength(parameters[i], index);
        if (paramSize == -1) {
          knownParameterSize= false;
          break;
        }
        parameterLength += paramSize;
      }

      if (knownParameterSize) {

        if (Protocol::checkRemainingSize(pos.length() + staticLength + parameterLength)) {
          pos.append(1, ';');
          pos.append(query, firstPart.first, firstPart.second);
          pos.append(query, secondPart.first, secondPart.second);
          for (size_t i= 0; i < paramCount; i++) {
            Parameter::toString(pos, parameters[i], index, noBackslashEscapes);
            pos.append(query, queryParts[i + 2].first, queryParts[i + 2].second);
          }
          pos.append(query, queryParts[paramCount + 2].first, queryParts[paramCount + 2].second);
          ++index;
        }
        else {
          break;
        }
      }
      else {

        pos.append(1, ';');
        pos.append(query, firstPart.first, firstPart.second);
        pos.append(query, secondPart.first, secondPart.second);
        for (size_t i= 0; i < paramCount; i++) {
          Parameter::toString(pos, parameters[i], index, noBackslashEscapes);
          pos.append(query, queryParts[i + 2].first, queryParts[i + 2].second);
        }
        pos.append(query, queryParts[paramCount + 2].first, queryParts[paramCount + 2].second);
        ++index;
        break;
      }
    }
    return index;
  }


  std::size_t ClientPrepareResult::assembleBatchQuery(SQLString& sql, MYSQL_BIND* parameters, uint32_t arraySize,
    std::size_t nextIndex) const
  {
    sql.reserve(2048);
    if (isQueryMultiValuesRewritable()) {
      // values rewritten in one query :
      // INSERT INTO X(a,b) VALUES (1,2), (3,4), ...
      nextIndex= assembleMultiValuesQuery(sql, this, parameters, arraySize, nextIndex, noBackslashEscapes);
    }
    else if (isQueryMultipleRewritable()) {
      nextIndex= assembleBatchRewriteQuery(sql, this, parameters, arraySize, nextIndex, noBackslashEscapes);
    }
    return nextIndex;
  }
} // namespace mariadb
