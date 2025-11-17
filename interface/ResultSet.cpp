/************************************************************************************
   Copyright (C) 2020, 2025 MariaDB Corporation plc

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

#include <algorithm>
#include "ResultSet.h"
#include "ServerPrepareResult.h"
#include "ResultSetBin.h"
#include "ResultSetText.h"
#include "class/Results.h"
#include "class/TextRow.h"
#include "interface/Exception.h"
#include "PreparedStatement.h"

namespace mariadb
{
  const MYSQL_FIELD FIELDBIGINT{0,0,0,0,0,0,0, 21, 0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_LONGLONG, 0};
  const MYSQL_FIELD FIELDSTRING{0,0,0,0,0,0,0, 255,0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_STRING, 0};
  const MYSQL_FIELD FIELDSHORT {0,0,0,0,0,0,0, 6,  0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_SHORT, 0};
  const MYSQL_FIELD FIELDINT   {0,0,0,0,0,0,0, 11, 0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_LONG, 0};

  static std::vector<ColumnDefinition> INSERT_ID_COLUMNS{{"insert_id", &FIELDBIGINT}};

  int32_t ResultSet::TINYINT1_IS_BIT= 1;
  int32_t ResultSet::YEAR_IS_DATE_TYPE= 2;

  uint64_t ResultSet::MAX_ARRAY_SIZE= INT32_MAX - 8;


  ResultSet::ResultSet(Protocol* guard, Results* results) :
    protocol(guard),
    fetchSize(results->getFetchSize()),
    statement(results->getStatement()),
    resultSetScrollType(results->getResultSetScrollType())
  {}


  ResultSet::ResultSet(Protocol* guard, std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    int32_t rsScrollType) :
    protocol(guard),
    fetchSize(0),
    row(new TextRow(nullptr)),
    isEof(true),
    columnsInformation(std::move(columnInformation)),
    columnInformationLength(static_cast<int32_t>(columnsInformation.size())),
    data(resultSet),
    dataSize(data.size()),
    resultSetScrollType(rsScrollType)
  {}


  ResultSet::ResultSet(Protocol* guard, Results* results,
    const std::vector<ColumnDefinition>& columnInformation) :
    protocol(guard),
    fetchSize(results->getFetchSize()),
    resultSetScrollType(results->getResultSetScrollType()),
    columnsInformation(columnInformation),
    columnInformationLength(static_cast<int32_t>(columnsInformation.size())),
    statement(results->getStatement())
  {}

  ResultSet::ResultSet(Protocol * _protocol, const MYSQL_FIELD* field,
    std::vector<std::vector<mariadb::bytes_view>>& resultSet, int32_t rsScrollType) :
    protocol(_protocol),
    fetchSize(0),
    row(new TextRow(nullptr)),
    isEof(true),
    // If resultset is empty - this won't work. we need columns count here
    columnInformationLength(static_cast<int32_t>(data.front().size())),
    data(std::move(resultSet)),
    dataSize(data.size()),
    resultSetScrollType(rsScrollType)
  {
    for (int32_t i= 0; i < columnInformationLength; ++i) {
      columnsInformation.emplace_back(&field[i], false);
    }
  }


  ResultSet* ResultSet::create(Results* results,
                               Protocol* _protocol,
                               ServerPrepareResult * spr, bool callableResult)
  {
    return new ResultSetBin(results, _protocol, spr, callableResult);
  }


  ResultSet* ResultSet::create(Results* results,
                               Protocol* _protocol,
                               MYSQL* connection)
  {
    return new ResultSetText(results, _protocol, connection);
  }

  /**
    * Create filled result-set.
    *
    * @param columnInformation column information
    * @param resultSet result-set data
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.SQL_CURSOR_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    */
  ResultSet* ResultSet::create(
    const MYSQL_FIELD* columnInformation,
    std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType)
  {
    return new ResultSetText(columnInformation, resultSet, _protocol, resultSetScrollType);
  }

  /**
    * Create filled result-set.
    *
    * @param columnInformation column information
    * @param resultSet result-set data
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.SQL_CURSOR_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    */
  ResultSet* ResultSet::create(
    std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType)
  {
    return new ResultSetText(columnInformation, resultSet, _protocol, resultSetScrollType);
  }

  /**
    * Create a result set from given data. Useful for creating "fake" resultsets for
    * DatabaseMetaData, (one example is MariaDbDatabaseMetaData.getTypeInfo())
    *
    * @param data - each element of this array represents a complete row in the ResultSet. Each value
    *     is given in its string representation, as in MariaDB text protocol, except boolean (BIT(1))
    *     values that are represented as "1" or "0" strings
    * @param protocol protocol
    * @param findColumnReturnsOne - special parameter, used only in generated key result sets
    * @return resultset
    */
  ResultSet* ResultSet::createGeneratedData(std::vector<int64_t>& data, bool findColumnReturnsOne)
  {
    /*std::vector<ColumnDefinition> columns{ColumnDefinition::create("insert_id", &bigint)};*/
    std::vector<std::vector<mariadb::bytes_view>> rows;
    std::string idAsStr;


    for (int64_t rowData : data) {
      std::vector<mariadb::bytes_view> row;
      if (rowData != 0) {
        idAsStr= std::to_string(rowData);
        row.emplace_back(idAsStr);
        rows.push_back(row);
      }
    }
    if (findColumnReturnsOne) {
      return create(INSERT_ID_COLUMNS[0].getColumnRawData(), rows, nullptr, TYPE_SCROLL_SENSITIVE);
      /*int32_t ResultSet::findColumn(const SQLString& name) {
        return 1;
      }*/
    }

    return new ResultSetText(INSERT_ID_COLUMNS, rows, nullptr, TYPE_SCROLL_SENSITIVE);
  }


  ResultSet* ResultSet::createEmptyResultSet() {
    static std::vector<std::vector<mariadb::bytes_view>> emptyRs;

    return create(INSERT_ID_COLUMNS[0].getColumnRawData(), emptyRs, nullptr, TYPE_SCROLL_SENSITIVE);
  }


  ResultSet * ResultSet::createResultSet(const std::vector<SQLString>& columnNames,
    const std::vector<const MYSQL_FIELD*>& columnTypes,
    const std::vector<std::vector<mariadb::bytes_view>>& data)
  {
    std::size_t columnNameLength= columnNames.size();

    std::vector<ColumnDefinition> columns;
    columns.reserve(columnTypes.size());

    for (std::size_t i= 0; i < columnNameLength; i++) {
      columns.emplace_back(columnNames[i], columnTypes[i]);
    }

    return create(columns, data, nullptr, TYPE_SCROLL_SENSITIVE);
  }


  ResultSet::~ResultSet()
  {
    delete row;
  }

  /** Close resultSet. */
  void ResultSet::close() {
    isClosedFlag= true;
    if (!isEof) {
      while (!isEof) {
        dataSize= 0; // to avoid storing data
        readNextValue();
      }
    }
    checkOut();
    resetVariables();

    data.clear();

    if (statement != nullptr) {
      statement= nullptr;
    }
  }


  void ResultSet::resetVariables() {
    isEof= true;
  }


  void ResultSet::handleIoException(std::exception& ioe) const
  {
    throw 1; /*SQLException(
        "Server has closed the connection. \n"
        "Please check net_read_timeout/net_write_timeout/wait_timeout server variables. "
        "If result set contain huge amount of data, Server expects client to"
        " read off the result set relatively fast. "
        "In this case, please consider increasing net_read_timeout session variable"
        " / processing your result set faster (check Streaming result sets documentation for more information)",
        "HY000", &ioe);*/
  }


  void ResultSet::checkOut()
  {
    if (statement != nullptr && statement->getInternalResults()) {
      statement->getInternalResults()->checkOut(this);
    }
  }


  bool ResultSet::next()
  {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
    if (rowPointer < static_cast<int32_t>(dataSize) - 1) {
      ++rowPointer;
      return true;
    }
    else {
      if (streaming && !isEof) {
        try {
          if (!isEof) {
            nextStreamingValue();
          }
        }
        catch (std::exception& /*ioe*/) {
          // We don't do anything else there now
          throw 1;
        }

        if (resultSetScrollType == TYPE_FORWARD_ONLY) {

          rowPointer= lastRowPointer= 0;
          return dataSize > 0;
        }
        else {
          ++rowPointer;
          return dataSize > static_cast<std::size_t>(rowPointer);
        }
      }
      rowPointer= static_cast<int32_t>(dataSize);
      return false;
    }
  }


  /**
    * This permit to add next streaming values to existing resultSet.
    *
    * @throws IOException if socket exception occur
    * @throws SQLException if server return an unexpected error
    */
  void ResultSet::addStreamingValue(bool cacheLocally) {

    int32_t fetchSizeTmp= fetchSize;
    while (fetchSizeTmp > 0 && readNextValue(cacheLocally)) {
      --fetchSizeTmp;
    }
    ++dataFetchTime;
  }

  // These following helpers and getCached should probably be moved to a separate module/class, or maybe getCached should be static
  // with different interface
  std::size_t strToDate(MYSQL_TIME* date, const SQLString str, std::size_t initialOffset)
  {
    std::size_t offset= initialOffset;
    if (str[offset] == '-') {
      date->neg= '\1';
      ++offset;
    }
    else {
      date->neg= '\0';
    }
    date->year=  static_cast<uint32_t>(std::stoll(str.substr(offset, 4)));
    date->month= static_cast<uint32_t>(std::stoll(str.substr(offset + 5, 2)));
    date->day=   static_cast<uint32_t>(std::stoll(str.substr(offset + 8, 2)));
    return offset + 11;
  }


  void strToTime(MYSQL_TIME* time, const SQLString str, std::size_t initialOffset)
  {
    std::size_t offset= initialOffset;
    if (str[offset] == '-') {
      time->neg= '\1';
      ++offset;
    }
    else {
      time->neg= '\0';
    }
    time->hour=   static_cast<uint32_t>(std::stoll(str.substr(offset, 2)));
    time->minute= static_cast<uint32_t>(std::stoll(str.substr(offset + 3, 2)));
    time->second= static_cast<uint32_t>(std::stoll(str.substr(offset + 6, 2)));
    time->second_part= 0;
    if (str[offset + 8] == '.') {
      auto fractPartLen= std::min(str.length() - offset - 9, std::size_t(6));
      time->second_part= static_cast<unsigned long>(std::stoll(str.substr(offset + 9, fractPartLen)));
      // Need to make it microseconds
      switch (fractPartLen) {
      case 1:
        time->second_part*= 10000;
        break;
      case 2:
        time->second_part*= 10000;
        break;
      case 3:
        time->second_part*= 1000;
        break;
      case 4:
        time->second_part*= 100;
        break;
      case 5:
        time->second_part*= 10;
      default:
        break;
      }
    }
  }


  bool floatColumnType(enum_field_types columnType)
  {
    switch (columnType)
    {
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE:
    case MYSQL_TYPE_NEWDECIMAL:
      return true;
    default:
      return false;
    }
    return false;
  }

  /* getCached is intended for cached resultsets, text protocol resultset can be considered cached. It does data types transcoding job
   * (unlike binary protocol results, where C/C provides this server)
   */
  bool ResultSet::getCached(MYSQL_BIND* bind, uint32_t column0basedIdx, uint64_t offset)
  {
    uint32_t terminateWithNull= 1, position= column0basedIdx + 1;
    bool intAppType= false;

    if (bind->error == nullptr) {
      bind->error= &bind->error_value;
    }
    if (bind->is_null == nullptr) {
      bind->is_null= &bind->is_null_value;
    }
    if (row->lastValueWasNull()) {
      *bind->is_null= '\1';
      checkObjectRange(position);
      return true;
    }
    else {
      *bind->is_null= '\0';
    }

    if (bind->length == nullptr) {
      bind->length= &bind->length_value;
    }
    // Setting error for the column, that will be reset if nothing throws during type conversions
    *bind->error= '\1';
    switch (bind->buffer_type)
    {
    case MYSQL_TYPE_BIT:
    case MYSQL_TYPE_TINY:
      *static_cast<uint8_t*>(bind->buffer)= getByte(position);
      *bind->length= 1;
      intAppType= true;
      break;
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_SHORT:
      *static_cast<int16_t*>(bind->buffer)= getShort(position);
      *bind->length= 2;
      intAppType= true;
      break;
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONG:
      if (bind->is_unsigned) {
        *static_cast<uint32_t*>(bind->buffer)= getUInt(position);
      }
      else {
        *static_cast<int32_t*>(bind->buffer)= getInt(position);
      }
      *bind->length= 4;
      intAppType= true;
      break;
    case MYSQL_TYPE_FLOAT:
      *static_cast<float*>(bind->buffer)= getFloat(position);
      *bind->length= 4;
      break;
    case MYSQL_TYPE_DOUBLE:
      *static_cast<double*>(bind->buffer)= getDouble(position);
      *bind->length= 8;
      break;
    case MYSQL_TYPE_NULL:
      *bind->is_null= isNull(column0basedIdx) ? '\1' : '\0';
      *bind->length= 0;
      break;
    case MYSQL_TYPE_LONGLONG:
      if (bind->is_unsigned) {
        *static_cast<uint64_t*>(bind->buffer)= getUInt64(position);
      }
      else {
        *static_cast<int64_t*>(bind->buffer)= getLong(position);
      }
      *bind->length= 8;
      intAppType= true;
      break;
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_NEWDATE:
    {
      MYSQL_TIME* date= static_cast<MYSQL_TIME*>(bind->buffer);
      Date str(row->getInternalDate(&columnsInformation[column0basedIdx]));
      strToDate(date, str, 0);
      break;
    }
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_TIME2:
    {
      MYSQL_TIME* time= static_cast<MYSQL_TIME*>(bind->buffer);
      Time str(row->getInternalTime(&columnsInformation[column0basedIdx], time));
      //strToTime(time, str, 0);
      break;
    }
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_DATETIME2:
    case MYSQL_TYPE_TIMESTAMP2:
    {
      MYSQL_TIME* timestamp= static_cast<MYSQL_TIME*>(bind->buffer);
      Timestamp str(row->getInternalTimestamp(&columnsInformation[column0basedIdx]));
      std::size_t timeOffset= strToDate(timestamp, str, 0);
      strToTime(timestamp, str, timeOffset);
      break;
    }
    case MARIADB_BINARY_TYPES:
      terminateWithNull= 0;
    default:
      if (bind->length != nullptr) {
        *bind->length= row->getLengthMaxFieldSize();
      }
      if (bind->buffer == nullptr || bind->buffer_length == 0) {
      }
      else {
        if (offset > row->getLengthMaxFieldSize()) {
          return true;
        }
        std::size_t bytesToCopy= std::min(static_cast<std::size_t>(bind->buffer_length) - terminateWithNull,
          static_cast<std::size_t>(row->getLengthMaxFieldSize() - offset));
        std::memcpy(bind->buffer, row->fieldBuf.arr + row->pos + offset, bytesToCopy);
        if (terminateWithNull > 0) {
          *(static_cast<char*>(bind->buffer) + bytesToCopy)= '\0';
        }
        if (bytesToCopy < row->getLengthMaxFieldSize()) {
          // Not resetting error
          return false;
        }
      }
      break;
    }
    //Checking if had truncation of decimal part
    if (!(intAppType && floatColumnType(columnsInformation[column0basedIdx].getColumnType()))) {
      *bind->error= '\0';
    }
    return false;
  }

  /**
  * This permit to replace current stream results by next ones.
  *
  * @throws IOException if socket exception occur
  * @throws SQLException if server return an unexpected error
  */
  void ResultSet::nextStreamingValue() {
    lastRowPointer= -1;

    if (resultSetScrollType == TYPE_FORWARD_ONLY) {
      dataSize= 0;
    }
    addStreamingValue(fetchSize > 1);
  }

  // It has to be const, because it's called by getters, and properties it changes are mutable
  // If something is changed here - might be needed into get() method, cuz for the work with rs callbacks it
  // knows too much about how resetRow works
  void ResultSet::resetRow() const
  {
    if (rowPointer > -1 && data.size() > static_cast<std::size_t>(rowPointer)) {
      row->resetRow(const_cast<std::vector<mariadb::bytes_view>&>(data[rowPointer]));
    }
    else {
      if (rowPointer != lastRowPointer + 1) {
        row->installCursorAtPosition(rowPointer > -1 ? rowPointer : 0);
      }
      row->fetchNext();
    }
    lastRowPointer= rowPointer;
  }


  void ResultSet::checkObjectRange(int32_t position) const {
    if (rowPointer < 0) {
      throw SQLException("Current position is before the first row", "22023");
    }

    if (static_cast<uint32_t>(rowPointer) >= dataSize) {
      throw SQLException("Current position is after the last row", "22023");
    }

    if (position <= 0 || position > columnInformationLength) {
      throw SQLException("No such column: " + std::to_string(position), "22023");
    }

    if (lastRowPointer != rowPointer) {
      resetRow();
    }
    row->setPosition(position - 1);
  }


  bool ResultSet::isNull(int32_t columnIndex) const
  {
    checkObjectRange(columnIndex);
    return row->lastValueWasNull();
  }


  bool ResultSet::fillBuffers(MYSQL_BIND* resBind)
  {
    bool truncations= false;
    if (resBind != nullptr) {
      for (int32_t i= 0; i < columnInformationLength; ++i) {
        try {
          get(&resBind[i], i, 0);
        }
        catch (int rc) {
          if (rc == MYSQL_DATA_TRUNCATED) {
            *resBind[i].error= '\1';
          }
        }
        if (*resBind[i].error) {
          truncations= true;
        }
      }
    }
    return truncations;
  }
} // namespace mariadb
