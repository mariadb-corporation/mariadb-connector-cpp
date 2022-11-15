/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB

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


#include "ClientPrepareResult.h"

namespace sql
{
namespace mariadb
{
  const SQLString SpecChars("();><=-+,");

  ClientPrepareResult::ClientPrepareResult(
    const SQLString& _sql,
    std::vector<SQLString>& _queryParts,
    bool isQueryMultiValuesRewritable,
    bool isQueryMultipleRewritable,
    bool _rewriteType)
    : sql(_sql)
    , queryParts(_queryParts)
    , isQueryMultiValuesRewritableFlag(isQueryMultiValuesRewritable)
    , isQueryMultipleRewritableFlag(isQueryMultipleRewritable)
    , paramCount(static_cast<uint32_t>(queryParts.size()) - (_rewriteType ? 3 : 1))
    , rewriteType(_rewriteType)
  {
  }

  /**
    * Separate query in a String list and set flag isQueryMultipleRewritable. The resulting string
    * list is separed by ? that are not in comments. isQueryMultipleRewritable flag is set if query
    * can be rewrite in one query (all case but if using "-- comment"). example for query : "INSERT
    * INTO tableName(id, name) VALUES (?, ?)" result list will be : {"INSERT INTO tableName(id, name)
    * VALUES (", ", ", ")"}
    *
    * @param queryString query
    * @param noBackslashEscapes escape mode
    * @return ClientPrepareResult
    */
  ClientPrepareResult* ClientPrepareResult::parameterParts(const SQLString& queryString, bool noBackslashEscapes)
  {
    bool reWritablePrepare= false;
    bool multipleQueriesPrepare= true;
    std::vector<SQLString> partList;
    LexState state= LexState::Normal;
    char lastChar= '\0';
    bool endingSemicolon= false;

    bool singleQuotes= false;
    std::size_t lastParameterPosition= 0;

    const char* query= queryString.c_str();
    std::size_t queryLength= queryString.length();
    for (std::size_t i= 0; i <queryLength; i++) {

      char car= query[i];
      if (state == LexState::Escape
        && !((car == '\'' && singleQuotes) || (car == '"' && !singleQuotes))) {
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
        else if (state == LexState::Normal &&lastChar == '/') {
          state= LexState::EOLComment;
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
          multipleQueriesPrepare= false;
        }
        break;

      case '\n':
        if (state == LexState::EOLComment) {
          multipleQueriesPrepare= true;
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
      case ';':
        if (state == LexState::Normal) {
          endingSemicolon= true;
          multipleQueriesPrepare= false;
        }
        break;
      case '?':
        if (state == LexState::Normal) {
          partList.push_back(
            queryString.substr(lastParameterPosition, i - lastParameterPosition)/*.getBytes(StandardCharsets.UTF_8)*/);
          lastParameterPosition= i + 1;
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

        if (state == LexState::Normal &&endingSemicolon &&((int8_t)car >=40)) {
          endingSemicolon= false;
          multipleQueriesPrepare= true;
        }
        break;
      }
      lastChar= car;
    }
    if (lastParameterPosition == 0) {
      partList.push_back(queryString/*.getBytes(StandardCharsets.UTF_8)*/);
    }
    else {
      partList.push_back(
        queryString
        .substr(lastParameterPosition, queryLength)
        /*.getBytes(StandardCharsets.UTF_8)*/);
    }

    return new ClientPrepareResult(
      queryString, partList, reWritablePrepare, multipleQueriesPrepare, false);
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

        if (state == LexState::Normal &&endingSemicolon &&((int8_t)car >=40)) {
          endingSemicolon= false;
        }
        break;
      }
      lastChar= car;
    }
    return state != LexState::EOLComment &&!endingSemicolon;
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
    std::vector<SQLString> partList;
    LexState state= LexState::Normal;
    char lastChar= '\0';

    SQLString sb("");

    SQLString preValuePart1;
    SQLString preValuePart2;
    SQLString postValuePart;

    bool singleQuotes= false;

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
        &&!((car == '\''&&singleQuotes)||(car == '"'&&!singleQuotes))) {
        sb.append(car);
        lastChar= car;
        state= LexState::SqlString;
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
        else if (state == LexState::SqlString &&!singleQuotes) {
          state= LexState::Normal;
        }
        else if (state == LexState::Escape &&!singleQuotes) {
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

      case '?':
        if (state == LexState::Normal) {
          hasParam= true;
          if (preValuePart1.empty()) {
            preValuePart1= sb;
            sb.clear();
          }
          if (preValuePart2.empty()) {
            preValuePart2= sb;
            sb.clear();
          }
          else {
            if (!postValuePart.empty()) {

              reWritablePrepare= false;

              sb= postValuePart.append(sb);
              postValuePart.clear();
            }
            partList.push_back(sb/*.getBytes(StandardCharsets.UTF_8)*/);
            sb.clear();
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
          && postValuePart.empty()
          && queryLength > i + 7
          && (query[i + 1] == 'e' || query[i + 1] == 'E')
          && (query[i + 2] == 'l' || query[i + 2] == 'L')
          && (query[i + 3] == 'e' || query[i + 3] == 'E')
          && (query[i + 4] == 'c' || query[i + 4] == 'C')
          && (query[i + 5] == 't' || query[i + 5] == 'T')) {


          if (i > 0 && (query[i - 1] > ' ' && SpecChars.find_first_of(query[i - 1]) == std::string::npos)) {
            break;
          }
          if (query[i + 6] >' '&& SpecChars.find_first_of(query[i + 6]) == std::string::npos) {
            break;
          }


          reWritablePrepare= false;
        }
        break;
      case 'v':
      case 'V':
        if (state == LexState::Normal
          && preValuePart1.empty()
          && (lastChar == ')'||((int8_t)lastChar <=40))
          && queryLength >i +7
          && (query[i + 1] == 'a' || query[i + 1] == 'A')
          && (query[i + 2] == 'l' || query[i + 2] == 'L')
          && (query[i + 3] == 'u' || query[i + 3] == 'U')
          && (query[i + 4] == 'e' || query[i + 4] == 'E')
          && (query[i + 5] == 's' || query[i + 5] == 'S')
          && (query[i + 6] == '(' || ((int8_t)query[i + 6] <= 40))) {
          sb.append(car);
          sb.append(query[i + 1]);
          sb.append(query[i + 2]);
          sb.append(query[i + 3]);
          sb.append(query[i + 4]);
          sb.append(query[i + 5]);
          i= i + 5;
          preValuePart1= sb;
          sb.clear();
          skipChar= true;
        }
        break;
      case 'l':
      case 'L':
        if (state == LexState::Normal
          &&queryLength >i +14
          &&(query[i +1] =='a'||query[i +1] =='A')
          &&(query[i +2] =='s'||query[i +2] =='S')
          &&(query[i +3] =='t'||query[i +3] =='T')
          &&query[i +4] =='_'
          &&(query[i +5] =='i'||query[i +5] =='I')
          &&(query[i +6] =='n'||query[i +6] =='N')
          &&(query[i +7] =='s'||query[i +7] =='S')
          &&(query[i +8] =='e'||query[i +8] =='E')
          &&(query[i +9] =='r'||query[i +9] =='R')
          &&(query[i +10] =='t'||query[i +10] =='T')
          &&query[i +11] =='_'
          &&(query[i +12] =='i'||query[i +12] =='I')
          &&(query[i +13] =='d'||query[i +13] =='D')
          &&query[i +14] =='(') {
          sb.append(car);
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
          if (isInParenthesis == 0 && !preValuePart2.empty() && postValuePart.empty()) {
            sb.append(car);
            postValuePart= sb;
            sb.clear();
            skipChar= true;
          }
        }
        break;
      default:
        if (state == LexState::Normal &&isFirstChar &&((int8_t)car >=40)) {
          if (car == 'I'||car == 'i') {
            isInsert= true;
          }
          isFirstChar= false;
        }

        if (state == LexState::Normal &&semicolon &&((int8_t)car >=40)) {
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
        sb.append(car);
      }
    }

    if (!hasParam) {

      if (preValuePart1.empty()) {
        partList.insert(partList.begin(), sb/*.getBytes(StandardCharsets.UTF_8)*/);
        partList.insert(partList.begin(), "");
      }
      else {

        partList.insert(partList.begin(), preValuePart1/*.getBytes(StandardCharsets.UTF_8)*/);
        partList.insert(partList.begin(), sb/*.getBytes(StandardCharsets.UTF_8)*/);
      }
      sb.clear();
    }
    else {
      partList.insert(partList.begin(), preValuePart1/*.getBytes(StandardCharsets.UTF_8)*/);
      partList.insert(partList.begin(), preValuePart2/*.getBytes(StandardCharsets.UTF_8)*/);
    }

    if (!isInsert) {
      reWritablePrepare= false;
    }


    if (hasParam) {
      partList.push_back(postValuePart/*.getBytes(StandardCharsets.UTF_8)*/);
    }
    partList.push_back(sb/*.getBytes(StandardCharsets.UTF_8)*/);


    return new ClientPrepareResult(
      queryString, partList, reWritablePrepare, multipleQueriesPrepare, true);
  }

  const SQLString& ClientPrepareResult::getSql() const
  {
    return sql;
  }

  const std::vector<SQLString>& ClientPrepareResult::getQueryParts() const
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

  /*void ClientPrepareResult::assembleQueryText(SQLString& resultSql, const ClientPrepareResult* clientPrepareResult, const std::vector<ParameterHolder>& parameters)
  {
      //if (queryTimeout > 0) {
      //  (("SET STATEMENT max_statement_time=" + queryTimeout + " FOR ").getBytes());
      //}
    auto queryParts= clientPrepareResult->getQueryParts();

      if (clientPrepareResult->isRewriteType()) {

        resultSql.append(queryParts[0]);
        resultSql.append(queryParts[1]);
        for (uint32_t i = 0; i < clientPrepareResult->getParamCount(); i++) {
          parameters[i].writeTo(out);
          out.write(clientPrepareResult->getQueryParts().get(i + 2));
        }
        out.write(clientPrepareResult->getQueryParts().get(clientPrepareResult->getParamCount() + 2));

      }
      else {

        out.write(clientPrepareResult->getQueryParts().get(0));
        for (int i = 0; i < clientPrepareResult->getParamCount(); i++) {
          parameters[i].writeTo(out);
          out.write(clientPrepareResult->getQueryParts().get(i + 1));
        }
      }
    }
  }*/
}
}
