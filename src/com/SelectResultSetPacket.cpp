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


#include <vector>
#include <array>

#include "SelectResultSet.h"
#include "Results.h"

#include "Protocol.h"
#include "protocol/capi/ColumnInformationCapi.h"
#include "ExceptionMapper.h"
#include "SqlStates.h"
#include "Packet.h"
#include "RowProtocol.h"
#include "protocol/capi/BinRowProtocolCapi.h"
#include "protocol/capi/TextRowProtocolCapi.h"
#include "util/ServerPrepareResult.h"

namespace sql
{
namespace mariadb
{
  class BinaryRowProtocol;
  class TextRowProtocol;
  class ErrorPacket;

  int32_t SelectResultSet::TINYINT1_IS_BIT= 1;
  int32_t SelectResultSet::YEAR_IS_DATE_TYPE= 2;
  const SQLString SelectResultSet::NOT_UPDATABLE_ERROR("Updates are not supported when using ResultSet::CONCUR_READ_ONLY");

  std::vector<Shared::ColumnInformation> SelectResultSet::INSERT_ID_COLUMNS{
    capi::ColumnInformationCapi::create("insert_id", ColumnType::BIGINT)
  };

  int32_t SelectResultSet::MAX_ARRAY_SIZE= INT32_MAX - 8;

  /**
    * Create Streaming resultSet.
    *
    * @param columnInformation column information
    * @param results results
    * @param protocol current protocol
    * @param reader stream fetcher
    * @param callableResult is it from a callableStatement ?
    * @param eofDeprecated is EOF deprecated
    * @throws IOException if any connection error occur
    * @throws SQLException if any connection error occur
    */
  SelectResultSet::SelectResultSet(
    std::vector<Shared::ColumnInformation>& columnInformation,
    Results* results,
    Protocol* protocol,
    PacketInputStream* reader,
    bool callableResult,
    bool eofDeprecated)
    : statement(results->getStatement()),
      isClosedFlag(false),
      protocol(protocol),
      options(protocol->getOptions()),
      noBackslashEscapes(protocol->noBackslashEscapes()),
      columnsInformation(columnInformation),
      columnNameMap(new ColumnNameMap(columnsInformation)),
      columnInformationLength(columnInformation.size()),
      reader(reader),
      fetchSize(results->getFetchSize()),
      dataSize(0),
      resultSetScrollType(results->getResultSetScrollType()),
      dataFetchTime(0),
      rowPointer(-1),
      callableResult(callableResult),
      eofDeprecated(eofDeprecated),
      isEof(false)
    //      timeZone(protocol->getTimeZone(),
  {
    if (results->isBinaryFormat()) {
      row= new /*BinaryRowProtocol*/capi::BinRowProtocolCapi(
        columnsInformation, columnInformationLength, results->getMaxFieldSize(), options);
    }
    else {
      row= new /*TextRowProtocol*/capi::TextRowProtocolCapi(results->getMaxFieldSize(), options);
    }

    if (fetchSize == 0 || callableResult) {
      data.reserve(10);//= new char[10]; // This has to be array of arrays. Need to decide what to use for its representation
      fetchAllResults();
      streaming= false;
    }
    else {
      lock= protocol->getLock();
      protocol->setActiveStreamingResult(Shared::Results(results));
      protocol->removeHasMoreResults();
      data.reserve(std::max(10, fetchSize)); // Same
      nextStreamingValue();
      streaming= true;
    }
  }

  SelectResultSet::SelectResultSet(Results * results, Protocol * protocol, ServerPrepareResult * spr, bool callableResult, bool eofDeprecated)
    : statement(results->getStatement()),
      isClosedFlag(false),
      protocol(protocol),
      options(protocol->getOptions()),
      noBackslashEscapes(protocol->noBackslashEscapes()),
      columnsInformation(spr->getColumns()),
      columnNameMap(new ColumnNameMap(columnsInformation)),
      columnInformationLength(columnsInformation.size()),
      reader(reader),
      fetchSize(results->getFetchSize()),
      dataSize(0),
      resultSetScrollType(results->getResultSetScrollType()),
      dataFetchTime(0),
      rowPointer(-1),
      callableResult(callableResult),
      eofDeprecated(eofDeprecated),
      isEof(false)
    //      timeZone(protocol->getTimeZone(),
  {
    row= new capi::BinRowProtocolCapi(
        columnsInformation, columnInformationLength, results->getMaxFieldSize(), options);

    if (fetchSize == 0 || callableResult) {
      data.reserve(10);//= new char[10]; // This has to be array of arrays. Need to decide what to use for its representation
      fetchAllResults();
      streaming= false;
    }
    else {
      lock= protocol->getLock();
      protocol->setActiveStreamingResult(Shared::Results(results));
      protocol->removeHasMoreResults();
      data.reserve(std::max(10, fetchSize)); // Same
      nextStreamingValue();
      streaming= true;
    }
  }

  SelectResultSet::SelectResultSet(std::vector<Shared::ColumnInformation>& columnInformation, Results * results, Protocol * protocol, capi::MYSQL_RES * res, bool eofDeprecated)
  {
  }

  /**
    * Create filled result-set.
    *
    * @param columnInformation column information
    * @param resultSet result-set data
    * @param protocol current protocol
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    */
  SelectResultSet::SelectResultSet(
    std::vector<Shared::ColumnInformation>& columnInformation,
    std::vector<sql::bytes>& resultSet,
    Protocol* protocol,
    int32_t resultSetScrollType)
    : statement(NULL),
      row(new capi::TextRowProtocolCapi(0, this->options)),
      data(resultSet), /*.toArray(new char[10][]){*/
      isClosedFlag(false),
      columnsInformation(columnInformation),
      columnNameMap(new ColumnNameMap(columnsInformation)),
      columnInformationLength(columnInformation.size()),
      isEof(true),
      fetchSize(0),
      resultSetScrollType(resultSetScrollType),
      dataSize(resultSet.size()),
      dataFetchTime(0),
      rowPointer(-1),
      callableResult(false),
      streaming(false)
  {
    if (protocol) {
      this->options= protocol->getOptions();
      //this->timeZone= protocol->getTimeZone();
    }
    else {
      // this->timeZone= TimeZone.getDefault();
    }
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
  ResultSet* SelectResultSet::createGeneratedData(std::vector<int64_t>& data, Shared::Protocol& protocol, bool findColumnReturnsOne)
  {
    std::vector<Shared::ColumnInformation> columns{capi::ColumnInformationCapi::create("insert_id", ColumnType::BIGINT)};
    std::vector<sql::bytes>rows;

    for (int64_t rowData : data) {
      if (rowData != 0) {
        rows.push_back(StandardPacketInputStream::create(std::to_string(rowData)));
      }
    }
    if (findColumnReturnsOne) {
      return new SelectResultSet(columns, rows, protocol.get(), TYPE_SCROLL_SENSITIVE);
      /*int32_t SelectResultSet::findColumn(const SQLString& name) {
        return 1;
      }*/
    }

    return new SelectResultSet(columns, rows, protocol.get(), TYPE_SCROLL_SENSITIVE);
  }


  SelectResultSet* SelectResultSet::createEmptyResultSet() {
    return new SelectResultSet(INSERT_ID_COLUMNS, std::vector<sql::bytes>(), nullptr, TYPE_SCROLL_SENSITIVE);
  }

  /**
    * Indicate if result-set is still streaming results from server.
    *
    * @return true if streaming is finished
    */
  bool SelectResultSet::isFullyLoaded() {


    return isEof;
  }

  void SelectResultSet::fetchAllResults()
  {
    dataSize= 0;
    while (readNextValue()) {
    }
    dataFetchTime++;
  }

  /**
    * When protocol has a current Streaming result (this) fetch all to permit another query is
    * executing.
    *
    * @throws SQLException if any error occur
    */
  void SelectResultSet::fetchRemaining() {
    if (!isEof) {
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {
        lastRowPointer= -1;
        while (!isEof) {
          addStreamingValue();
        }

      }
      catch (SQLException& queryException) {
        throw ExceptionMapper::getException(queryException, NULL, statement, false);
      }
      catch (std::exception& ioe) {
        throw handleIoException(ioe);
      }
      dataFetchTime++;
    }
  }

  SQLException SelectResultSet::handleIoException(std::exception& ioe)
  {
    return ExceptionMapper::getException(
      SQLException(
        "Server has closed the connection. \n"
        "Please check net_read_timeout/net_write_timeout/wait_timeout server variables. "
        "If result set contain huge amount of data, Server expects client to"
        " read off the result set relatively fast. "
        "In this case, please consider increasing net_read_timeout session variable"
        " / processing your result set faster (check Streaming result sets documentation for more information)",
        CONNECTION_EXCEPTION.getSqlState(), 0,
        &ioe),
      NULL,
      statement,
      false);
  }

  /**
    * This permit to replace current stream results by next ones.
    *
    * @throws IOException if socket exception occur
    * @throws SQLException if server return an unexpected error
    */
  void SelectResultSet::nextStreamingValue() {
    lastRowPointer= -1;


    if (resultSetScrollType == TYPE_FORWARD_ONLY) {
      dataSize= 0;
    }

    addStreamingValue();
  }

  /**
    * This permit to add next streaming values to existing resultSet.
    *
    * @throws IOException if socket exception occur
    * @throws SQLException if server return an unexpected error
    */
  void SelectResultSet::addStreamingValue() {


    int32_t fetchSizeTmp= fetchSize;
    while (fetchSizeTmp >0 && readNextValue()) {
      fetchSizeTmp--;
    }
    dataFetchTime++;
  }

  /**
    * Read next value.
    *
    * @return true if have a new value
    * @throws IOException exception
    * @throws SQLException exception
    */
  bool SelectResultSet::readNextValue()
  {
    sql::bytes buf= reader->getPacketArray(false);

    if (buf[0] == Packet::ERROR) {
      protocol->removeActiveStreamingResult();
      protocol->removeHasMoreResults();
      protocol->setHasWarnings(false);
      ErrorPacket* errorPacket= new ErrorPacket(new Buffer(buf));
      resetVariables();
      throw ExceptionMapper::get(
        errorPacket->getMessage(),
        errorPacket->getSqlState(),
        errorPacket->getErrorNumber(),
        NULL,
        false);
    }

    if (buf[0] == EOF
      && ((eofDeprecated && buf.length < 0xffffff) || (!eofDeprecated && buf.length < 8))) {
      int32_t serverStatus;
      int32_t warnings;

      if (!eofDeprecated) {

        warnings= (buf[1] &0xff)+((buf[2] &0xff)<<8);
        serverStatus= ((buf[3] &0xff)+((buf[4] &0xff)<<8));

        // CallableResult has been read from intermediate EOF server_status
        // and is mandatory because :
        //
        // - Call query will have an callable resultSet for OUT parameters
        //   this resultSet must be identified and not listed in JDBC statement.getResultSet()
        //
        // - after a callable resultSet, a OK packet is send,
        //   but mysql before 5.7.4 doesn't send MORE_RESULTS_EXISTS flag
        if (callableResult) {
          serverStatus|= MORE_RESULTS_EXISTS;
        }
      }
      else {
        // OK_Packet with a 0xFE header
        int32_t pos= skipLengthEncodedValue(buf, 1);
        pos= skipLengthEncodedValue(buf, pos);
        serverStatus= ((buf[pos++] &0xff)+((buf[pos++] &0xff)<<8));
        warnings= (buf[pos++] &0xff)+((buf[pos] &0xff)<<8);
        callableResult= (serverStatus &PS_OUT_PARAMETERS)!=0;
      }
      protocol->setServerStatus(static_cast<int16_t>(serverStatus));
      protocol->setHasWarnings(warnings >0);
      if ((serverStatus & MORE_RESULTS_EXISTS) == 0) {
        protocol->removeActiveStreamingResult();
      }

      resetVariables();
      return false;
    }


    if (dataSize + 1 >= data.size()) {
      growDataArray();
    }
    data[dataSize++] =buf;
    return true;
  }

  /**
    * Get current row's raw bytes.
    *
    * @return row's raw bytes
    */
  std::string& SelectResultSet::getCurrentRowData() {
    return data[rowPointer];
  }

  /**
    * Update row's raw bytes. in case of row update, refresh the data. (format must correspond to
    * current resultset binary/text row encryption)
    *
    * @param rawData new row's raw data.
    */
  void SelectResultSet::updateRowData(std::string& rawData)
  {
    data[rowPointer]= rawData;
    row->resetRow(data[rowPointer]);
  }

  /**
    * Delete current data. Position cursor to the previous row->
    *
    * @throws SQLException if previous() fail.
    */
  void SelectResultSet::deleteCurrentRowData() {


    System.arraycopy(data, rowPointer +1, data, rowPointer, dataSize -1 -rowPointer);
    data[dataSize -1] =NULL;
    dataSize--;
    lastRowPointer= -1;
    previous();
  }

  void SelectResultSet::addRowData(std::string& rawData) {
    if (dataSize +1 >=data->length) {
      growDataArray();
    }
    data[dataSize] =rawData;
    rowPointer= dataSize;
    dataSize++;
  }

  int32_t SelectResultSet::skipLengthEncodedValue(std::string& buf, int32_t pos) {
    int32_t type= buf[pos++] &0xff;
    switch (type) {
    case 251:
      return pos;
    case 252:
      return pos +2 +(0xffff &(((buf[pos] &0xff)+((buf[pos +1] &0xff)<<8))));
    case 253:
      return pos
        +3
        +(0xffffff
          &((buf[pos] &0xff)
            +((buf[pos +1] &0xff)<<8)
            +((buf[pos +2] &0xff)<<16)));
    case 254:
      return (int32_t)
        (pos
          +8
          +((buf[pos] &0xff)
            +((int64_t)(buf[pos +1] &0xff)<<8)
            +((int64_t)(buf[pos +2] &0xff)<<16)
            +((int64_t)(buf[pos +3] &0xff)<<24)
            +((int64_t)(buf[pos +4] &0xff)<<32)
            +((int64_t)(buf[pos +5] &0xff)<<40)
            +((int64_t)(buf[pos +6] &0xff)<<48)
            +((int64_t)(buf[pos +7] &0xff)<<56)));
    default:
      return pos +type;
    }
  }

  /** Grow data array. */
  void SelectResultSet::growDataArray() {
    int32_t newCapacity= data.size() + (data.size() >>1);

    if (newCapacity - MAX_ARRAY_SIZE > 0) {
      newCapacity= MAX_ARRAY_SIZE;
    }
    data.reserve(newCapacity);
  }

  /**
    * Connection.abort() has been called, abort result-set.
    *
    * @throws SQLException exception
    */
  void SelectResultSet::abort() {
    isClosedFlag= true;
    resetVariables();

    for (int32_t i= 0; i < data->length; i++) {
      data[i]= NULL;
    }

    if (statement != nullptr) {
      statement->checkCloseOnCompletion(this);
      statement= NULL;
    }
  }

  /** Close resultSet. */
  void SelectResultSet::close() {
    isClosedFlag= true;
    if (!isEof) {
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {
        while (!isEof) {
          dataSize= 0;
          readNextValue();
        }

      }
      catch (SQLException& queryException) {
        throw ExceptionMapper::getException(queryException, NULL, this->statement, false);
      }
      catch (SQLException& queryException) {
        resetVariables();
        throw handleIoException(ioe);
      }
    }
    resetVariables();

    for (int32_t i= 0; i < data.length(); i++)
    {
      if (data[i])
      {
        delete data[i];
        data[i]= NULL;
      }
    }

    if (statement != nullptr) {
      statement->checkCloseOnCompletion(this);
      statement= NULL;
    }
  }

  void SelectResultSet::resetVariables() {
    protocol.reset();
    reader= NULL;
    isEof= true;
  }

  bool SelectResultSet::next() {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
    if (rowPointer <dataSize -1) {
      rowPointer++;
      return true;
    }
    else {
      if (streaming &&!isEof) {
        std::lock_guard<std::mutex> localScopeLock(*lock);
        try {
          if (!isEof) {
            nextStreamingValue();
          }
        }
        catch (std::exception& ioe) {
          throw handleIoException(ioe);
        }

        if (resultSetScrollType == TYPE_FORWARD_ONLY) {

          rowPointer= 0;
          return dataSize >0;
        }
        else {
          rowPointer++;
          return dataSize >rowPointer;
        }
      }


      rowPointer= dataSize;
      return false;
    }
  }

  void SelectResultSet::checkObjectRange(int32_t position) {
    if (rowPointer <0) {
      throw SQLDataException("Current position is before the first row", "22023");
    }

    if (rowPointer >=dataSize) {
      throw SQLDataException("Current position is after the last row", "22023");
    }

    if (position <=0 ||position >columnInformationLength) {
      throw SQLDataException("No such column: "+position, "22023");
    }

    if (lastRowPointer != rowPointer) {
      row->resetRow(data[rowPointer]);
      lastRowPointer= rowPointer;
    }
    row->setPosition(position -1);
  }

  SQLWarning* SelectResultSet::getWarnings() {
    if (this->statement == NULL) {
      return NULL;
    }
    return this->statement->getWarnings();
  }

  void SelectResultSet::clearWarnings() {
    if (this->statement != nullptr) {
      this->statement->clearWarnings();
    }
  }

  bool SelectResultSet::isBeforeFirst() {
    checkClose();
    return (dataFetchTime >0) ? rowPointer == -1 &&dataSize >0 :rowPointer == -1;
  }

  bool SelectResultSet::isAfterLast() {
    checkClose();
    if (rowPointer <dataSize) {

      return false;
    }
    else {

      if (streaming &&!isEof) {

        std::lock_guard<std::mutex> localScopeLock(*lock);
        try {

          if (!isEof) {
            addStreamingValue();
          }
        }
        catch (std::exception& ioe) {
          throw handleIoException(ioe);
        }

        return dataSize == rowPointer;
      }

      return dataSize >0 ||dataFetchTime >1;
    }
  }

  bool SelectResultSet::isFirst() {
    checkClose();
    return dataFetchTime == 1 &&rowPointer == 0 &&dataSize >0;
  }

  bool SelectResultSet::isLast() {
    checkClose();
    if (rowPointer <dataSize -1) {
      return false;
    }
    else if (isEof) {
      return rowPointer == dataSize -1 &&dataSize >0;
    }
    else {


      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {
        if (!isEof) {
          addStreamingValue();
        }
      }
      catch (std::exception& ioe) {
        throw handleIoException(ioe);
      }

      if (isEof) {

        return rowPointer == dataSize -1 &&dataSize >0;
      }

      return false;
    }
  }

  void SelectResultSet::beforeFirst() {
    checkClose();

    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    rowPointer= -1;
  }

  void SelectResultSet::afterLast() {
    checkClose();
    fetchRemaining();
    rowPointer= dataSize;
  }

  bool SelectResultSet::first() {
    checkClose();

    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }

    rowPointer= 0;
    return dataSize >0;
  }

  bool SelectResultSet::last() {
    checkClose();
    fetchRemaining();
    rowPointer= dataSize -1;
    return dataSize >0;
  }

  int32_t SelectResultSet::getRow() {
    checkClose();
    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      return 0;
    }
    return rowPointer +1;
  }

  bool SelectResultSet::absolute(int32_t row) {
    checkClose();

    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }

    if (row >=0 &&row <=dataSize) {
      rowPointer= row -1;
      return true;
    }


    fetchRemaining();

    if (row >=0) {

      if (row <=dataSize) {
        rowPointer= row -1;
        return true;
      }

      rowPointer= dataSize;
      return false;

    }
    else {

      if (dataSize +row >=0) {

        rowPointer= dataSize +row;
        return true;
      }

      rowPointer= -1;
      return false;
    }
  }

  bool SelectResultSet::relative(int32_t rows) {
    checkClose();
    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    int32_t newPos= rowPointer +rows;
    if (newPos <=-1) {
      rowPointer= -1;
      return false;
    }
    else if (newPos >=dataSize) {
      rowPointer= dataSize;
      return false;
    }
    else {
      rowPointer= newPos;
      return true;
    }
  }

  bool SelectResultSet::previous() {
    checkClose();
    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    if (rowPointer >-1) {
      rowPointer--;
      return rowPointer != -1;
    }
    return false;
  }

  int32_t SelectResultSet::getFetchDirection() {
    return FETCH_UNKNOWN;
  }

  void SelectResultSet::setFetchDirection(int32_t direction) {
    if (direction == FETCH_REVERSE) {
      throw SQLException(
        "Invalid operation. Allowed direction are ResultSet::FETCH_FORWARD and ResultSet::FETCH_UNKNOWN");
    }
  }

  int32_t SelectResultSet::getFetchSize() {
    return this->fetchSize;
  }

  void SelectResultSet::setFetchSize(int32_t fetchSize) {
    if (streaming &&fetchSize == 0) {
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {

        while (!isEof) {
          addStreamingValue();
        }
      }
      catch (std::exception& ioe) {
        throw handleIoException(ioe);
      }
      streaming= dataFetchTime == 1;
    }
    this->fetchSize= fetchSize;
  }

  int32_t SelectResultSet::getType() {
    return resultSetScrollType;
  }

  int32_t SelectResultSet::getConcurrency() {
    return CONCUR_READ_ONLY;
  }

  void SelectResultSet::checkClose() {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
  }

  bool SelectResultSet::isCallableResult() {
    return callableResult;
  }

  bool SelectResultSet::isClosed() const {
    return isClosedFlag;
  }

  MariaDbStatement* SelectResultSet::getStatement() {
    return statement;
  }

  void SelectResultSet::setStatement(MariaDbStatement* statement)
  {
    this->statement= statement;
  }

  /** {inheritDoc}. */
  bool SelectResultSet::wasNull() {
    return row->wasNull();
  }

#ifdef MAYBE_IN_BETA
  /** {inheritDoc}. */
  std::istream* SelectResultSet::getAsciiStream(const SQLString& columnLabel) {
    return getAsciiStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  std::istream* SelectResultSet::getAsciiStream(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    return new ByteArrayInputStream(
      new SQLString(row->buf, row->pos, row->getLengthMaxFieldSize()).c_str());/*, StandardCharsets.UTF_8*/
  }
#endif

  /** {inheritDoc}. */
  SQLString SelectResultSet::getString(int32_t columnIndex)
  {
    checkObjectRange(columnIndex);
    return row->getInternalString(columnsInformation[columnIndex -1], NULL, timeZone);
  }

  /** {inheritDoc}. */
  SQLString SelectResultSet::getString(const SQLString& columnLabel) {
    return getString(findColumn(columnLabel));
  }


  SQLString SelectResultSet::zeroFillingIfNeeded(const SQLString& value, ColumnInformation* columnInformation)
  {
    if (columnInformation.isZeroFill()) {
      SQLString zeroAppendStr;
      int64_t zeroToAdd= columnInformation.getDisplaySize()-value.size();
      while (zeroToAdd-->0) {
        zeroAppendStr.append("0");
      }
      return zeroAppendStr.append(value);
    }
    return value;
  }

  /** {inheritDoc}. */  std::istream* SelectResultSet::getBinaryStream(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    return new ByteArrayInputStream(row->buf, row->pos, row->getLengthMaxFieldSize());
  }

  /** {inheritDoc}. */  std::istream* SelectResultSet::getBinaryStream(const SQLString& columnLabel) {
    return getBinaryStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */  int32_t SelectResultSet::getInt(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalInt(columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */  int32_t SelectResultSet::getInt(const SQLString& columnLabel) {
    return getInt(findColumn(columnLabel));
  }

  /** {inheritDoc}. */  int64_t SelectResultSet::getLong(const SQLString& columnLabel) {
    return getLong(findColumn(columnLabel));
  }

  /** {inheritDoc}. */  int64_t SelectResultSet::getLong(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalLong(columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */  float SelectResultSet::getFloat(const SQLString& columnLabel) {
    return getFloat(findColumn(columnLabel));
  }

  /** {inheritDoc}. */  float SelectResultSet::getFloat(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalFloat(columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  long double SelectResultSet::getDouble(const SQLString& columnLabel) {
    return getDouble(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  long double SelectResultSet::getDouble(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalDouble(columnsInformation[columnIndex -1]);
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /** {inheritDoc}. */
  BigDecimal SelectResultSet::getBigDecimal(const SQLString& columnLabel, int32_t scale) {
    return getBigDecimal(findColumn(columnLabel), scale);
  }

  /** {inheritDoc}. */
  BigDecimal SelectResultSet::getBigDecimal(int32_t columnIndex, int32_t scale) {
    checkObjectRange(columnIndex);
    return row->getInternalBigDecimal(columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  BigDecimal SelectResultSet::getBigDecimal(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalBigDecimal(columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  BigDecimal SelectResultSet::getBigDecimal(const SQLString& columnLabel) {
    return getBigDecimal(findColumn(columnLabel));
  }
#endif
#ifdef MAYBE_IN_BETA
  /** {inheritDoc}. */  SQLString SelectResultSet::getBytes(const SQLString& columnLabel) {
    return getBytes(findColumn(columnLabel));
  }
  /** {inheritDoc}. */  SQLString SelectResultSet::getBytes(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    char* data= new char[row->getLengthMaxFieldSize()];
    System.arraycopy(row->buf, row->pos, data, 0, row->getLengthMaxFieldSize());
    return data;
  }


  /** {inheritDoc}. */
  Date* SelectResultSet::getDate(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalDate(columnsInformation[columnIndex -1], NULL, timeZone);
  }

  /** {inheritDoc}. */
  Date* SelectResultSet::getDate(const SQLString& columnLabel) {
    return getDate(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Time* SelectResultSet::getTime(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalTime(columnsInformation[columnIndex -1], NULL, timeZone);
  }

  /** {inheritDoc}. */  Time SelectResultSet::getTime(const SQLString& columnLabel) {
    return getTime(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSet::getTimestamp(const SQLString& columnLabel) {
    return getTimestamp(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSet::getTimestamp(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalTimestamp(columnsInformation[columnIndex -1], NULL, timeZone);
  }
#endif

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /** {inheritDoc}. */
  Date* SelectResultSet::getDate(int32_t columnIndex, Calendar& cal) {
    checkObjectRange(columnIndex);
    return row->getInternalDate(columnsInformation[columnIndex -1], cal, timeZone);
  }

  /** {inheritDoc}. */
  Date* SelectResultSet::getDate(const SQLString& columnLabel, Calendar& cal) {
    return getDate(findColumn(columnLabel), cal);
  }

  /** {inheritDoc}. */
  Time* SelectResultSet::getTime(int32_t columnIndex, Calendar& cal) {
    checkObjectRange(columnIndex);
    return row->getInternalTime(columnsInformation[columnIndex -1], cal, timeZone);
  }

  /** {inheritDoc}. */
  Time* SelectResultSet::getTime(const SQLString& columnLabel, Calendar& cal) {
    return getTime(findColumn(columnLabel), cal);
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSet::getTimestamp(int32_t columnIndex, Calendar& cal) {
    checkObjectRange(columnIndex);
    return row->getInternalTimestamp(columnsInformation[columnIndex -1], cal, timeZone);
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSet::getTimestamp(const SQLString& columnLabel, Calendar& cal) {
    return getTimestamp(findColumn(columnLabel), cal);
  }
#endif

  /** {inheritDoc}. */
  SQLString SelectResultSet::getCursorName() {
    throw ExceptionMapper::getFeatureNotSupportedException("Cursors not supported");
  }

  /** {inheritDoc}. */
  sql::ResultSetMetaData* SelectResultSet::getMetaData() {
    return new MariaDbResultSetMetaData(columnsInformation, options, forceAlias);
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED

  /** {inheritDoc}. */
  sql::Object* SelectResultSet::getObject(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalObject(columnsInformation[columnIndex -1], timeZone);
  }

  /** {inheritDoc}. */
  sql::Object* SelectResultSet::getObject(const SQLString& columnLabel) {
    return getObject(findColumn(columnLabel));
  }

  template <class T>T getObject(int32_t columnIndex, Classtemplate <class T>type) {
    if (type.empty() == true) {
      throw SQLException("Class type cannot be NULL");
    }
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    ColumnInformation col= columnsInformation[columnIndex -1];

    if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalString(col, NULL, timeZone);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)static_cast<int32_t>(row->getInternalInt(col));

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)static_cast<int64_t>(row->getInternalLong(col));

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)(Short)row->getInternalShort(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)(Double)row->getInternalDouble(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)(Float)row->getInternalFloat(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)(Byte)row->getInternalByte(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      char* data= new char[row->getLengthMaxFieldSize()];
      System.arraycopy(row->buf, row->pos, data, 0, row->getLengthMaxFieldSize());
      return (T)data;

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalDate(col, NULL, timeZone);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalTime(col, NULL, timeZone);

    }
    else if ((type.compare(SQLString.class) == 0)||((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalTimestamp(col, NULL, timeZone);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)(Boolean)row->getInternalBoolean(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      SKIP; calendar= SKIP; .getInstance(timeZone);
      Timestamp timestamp= row->getInternalTimestamp(col, NULL, timeZone);
      if (timestamp.empty() == true) {
        return NULL;
      }
      calendar.setTimeInMillis(timestamp.getTime());
      return type.cast(calendar);

    }
    else if ((type.compare(SQLString.class) == 0)||((type.compare(SQLString.class) == 0)) {
      return (T)new MariaDbClob(row->buf, row->pos, row->getLengthMaxFieldSize());

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)new ByteArrayInputStream(row->buf, row->pos, row->getLengthMaxFieldSize());

    }
    else if ((type.compare(SQLString.class) == 0)) {
      SQLString value= row->getInternalString(col, NULL, timeZone);
      if (value.empty() == true) {
        return NULL;
      }
      return (T)new StringReader(value);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalBigDecimal(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalBigInteger(col);
    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalBigDecimal(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      ZonedDateTime zonedDateTime =
        row->getInternalZonedDateTime(col, LocalDateTime.class, timeZone);
      return zonedDateTime.empty() == true
        ? NULL
        : type.cast(zonedDateTime.withZoneSameInstant(ZoneId.systemDefault()).toLocalDateTime());

    }
    else if ((type.compare(SQLString.class) == 0)) {
      ZonedDateTime zonedDateTime =
        row->getInternalZonedDateTime(col, ZonedDateTime.class, timeZone);
      if (zonedDateTime.empty() == true) {
        return NULL;
      }
      return type.cast(row->getInternalZonedDateTime(col, ZonedDateTime.class, timeZone));

    }
    else if ((type.compare(SQLString.class) == 0)) {
      ZonedDateTime tmpZonedDateTime =
        row->getInternalZonedDateTime(col, OffsetDateTime.class, timeZone);
      return tmpZonedDateTime.empty() == true ? NULL : type.cast(tmpZonedDateTime.toOffsetDateTime());

    }
    else if ((type.compare(SQLString.class) == 0)) {
      LocalDate localDate= row->getInternalLocalDate(col, timeZone);
      if (localDate.empty() == true) {
        return NULL;
      }
      return type.cast(localDate);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      LocalDate localDate= row->getInternalLocalDate(col, timeZone);
      if (localDate.empty() == true) {
        return NULL;
      }
      return type.cast(localDate);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      LocalTime localTime= row->getInternalLocalTime(col, timeZone);
      if (localTime.empty() == true) {
        return NULL;
      }
      return type.cast(localTime);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      OffsetTime offsetTime= row->getInternalOffsetTime(col, timeZone);
      if (offsetTime.empty() == true) {
        return NULL;
      }
      return type.cast(offsetTime);
    }
    throw ExceptionMapper::getFeatureNotSupportedException(
      "Type class '"+type.getName()+"' is not supported");
  }

  /** {inheritDoc}. */
  int32_t SelectResultSet::findColumn(const SQLString& columnLabel) {
    return columnNameMap.getIndex(columnLabel)+1;
  }

  /** {inheritDoc}. */
  std::istringstream& SelectResultSet::getCharacterStream(const SQLString& columnLabel) {
    return getCharacterStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  std::istringstream& SelectResultSet::getCharacterStream(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    SQLString value= row->getInternalString(columnsInformation[columnIndex -1], NULL, timeZone);
    if (value.empty() == true) {
      return NULL;
    }
    return new StringReader(value);
  }

  /** {inheritDoc}. */
  std::istringstream& SelectResultSet::getNCharacterStream(int32_t columnIndex) {
    return getCharacterStream(columnIndex);
  }

  /** {inheritDoc}. */
  std::istringstream& SelectResultSet::getNCharacterStream(const SQLString& columnLabel) {
    return getCharacterStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Ref* SelectResultSet::getRef(int32_t columnIndex) {

    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  Ref* SelectResultSet::getRef(const SQLString& columnLabel) {
    throw ExceptionMapper::getFeatureNotSupportedException("Getting REFs not supported");
  }

  /** {inheritDoc}. */
  Blob* SelectResultSet::getBlob(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    return new MariaDbBlob(row->buf, row->pos, row->length);
  }

  /** {inheritDoc}. */
  Blob* SelectResultSet::getBlob(const SQLString& columnLabel) {
    return getBlob(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Clob* SelectResultSet::getClob(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    return new MariaDbClob(row->buf, row->pos, row->length);
  }

  /** {inheritDoc}. */
  Clob* SelectResultSet::getClob(const SQLString& columnLabel) {
    return getClob(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  sql::Array* SelectResultSet::getArray(int32_t columnIndex) {
    throw ExceptionMapper::getFeatureNotSupportedException("Arrays are not supported");
  }

  /** {inheritDoc}. */
  sql::Array* SelectResultSet::getArray(const SQLString& columnLabel) {
    return getArray(findColumn(columnLabel));
  }


  URL* SelectResultSet::getURL(int32_t columnIndex)
  {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    try {
      return new URL(row->getInternalString(columnsInformation[columnIndex -1], NULL, timeZone));
    }
    catch (MalformedURLException& e) {
      throw ExceptionMapper::getSqlException("Could not parse as URL");
    }
  }

  URL* SelectResultSet::getURL(const SQLString& columnLabel) {
    return getURL(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  RowId* SelectResultSet::getRowId(int32_t columnIndex) {
    throw ExceptionMapper::getFeatureNotSupportedException("RowIDs not supported");
  }

  /** {inheritDoc}. */
  RowId* SelectResultSet::getRowId(const SQLString& columnLabel) {
    throw ExceptionMapper::getFeatureNotSupportedException("RowIDs not supported");
  }

  /** {inheritDoc}. */
  NClob* SelectResultSet::getNClob(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return NULL;
    }
    return new MariaDbClob(row->buf, row->pos, row->length);
  }

  /** {inheritDoc}. */
  NClob* SelectResultSet::getNClob(const SQLString& columnLabel) {
    return getNClob(findColumn(columnLabel));
  }

  SQLXML* SelectResultSet::getSQLXML(int32_t columnIndex) {
    throw ExceptionMapper::getFeatureNotSupportedException("SQLXML not supported");
  }

  SQLXML* SelectResultSet::getSQLXML(const SQLString& columnLabel) {
    throw ExceptionMapper::getFeatureNotSupportedException("SQLXML not supported");
  }

  /** {inheritDoc}. */  SQLString SelectResultSet::getNString(int32_t columnIndex) {
    return getString(columnIndex);
  }

  /** {inheritDoc}. */
  SQLString SelectResultSet::getNString(const SQLString& columnLabel) {
    return getString(findColumn(columnLabel));
  }
#endif

  /** {inheritDoc}. */
  bool SelectResultSet::getBoolean(int32_t index) {
    checkObjectRange(index);
    return row->getInternalBoolean(columnsInformation[index -1]);
  }

  /** {inheritDoc}. */
  bool SelectResultSet::getBoolean(const SQLString& columnLabel) {
    return getBoolean(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  int8_t SelectResultSet::getByte(int32_t index) {
    checkObjectRange(index);
    return row->getInternalByte(columnsInformation[index -1]);
  }

  /** {inheritDoc}. */
  int8_t SelectResultSet::getByte(const SQLString& columnLabel) {
    return getByte(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  short SelectResultSet::getShort(int32_t index) {
    checkObjectRange(index);
    return row->getInternalShort(columnsInformation[index -1]);
  }

  /** {inheritDoc}. */
  short SelectResultSet::getShort(const SQLString& columnLabel) {
    return getShort(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  bool SelectResultSet::rowUpdated() {
    throw ExceptionMapper::getFeatureNotSupportedException(
      "Detecting row updates are not supported");
  }

  /** {inheritDoc}. */
  bool SelectResultSet::rowInserted() {
    throw ExceptionMapper::getFeatureNotSupportedException("Detecting inserts are not supported");
  }

  /** {inheritDoc}. */
  bool SelectResultSet::rowDeleted() {
    throw ExceptionMapper::getFeatureNotSupportedException("Row deletes are not supported");
  }

  /** {inheritDoc}. */
  void SelectResultSet::insertRow() {
    throw ExceptionMapper::getFeatureNotSupportedException(
      "insertRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }

  /** {inheritDoc}. */
  void SelectResultSet::deleteRow() {
    throw ExceptionMapper::getFeatureNotSupportedException(
      "deleteRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }

  /** {inheritDoc}. */
  void SelectResultSet::refreshRow() {
    throw ExceptionMapper::getFeatureNotSupportedException(
      "refreshRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }


#ifdef RS_UPDATE_FUNCTIONALITY_IMPLEMENTED
  /** {inheritDoc}. */
  void SelectResultSet::cancelRowUpdates() {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::moveToInsertRow() {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::moveToCurrentRow() {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNull(int32_t columnIndex) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNull(const SQLString& columnLabel) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateBoolean(int32_t columnIndex, bool _bool) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBoolean(const SQLString& columnLabel, bool value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateByte(int32_t columnIndex, char value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateByte(const SQLString& columnLabel, char value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateShort(int32_t columnIndex, short value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateShort(const SQLString& columnLabel, short value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateInt(int32_t columnIndex, int32_t value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateInt(const SQLString& columnLabel, int32_t value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateFloat(int32_t columnIndex, float value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateFloat(const SQLString& columnLabel, float value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateDouble(int32_t columnIndex, double value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateDouble(const SQLString& columnLabel, double value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBigDecimal(int32_t columnIndex, BigDecimal value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBigDecimal(const SQLString& columnLabel, BigDecimal value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateString(int32_t columnIndex, const SQLString& value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateString(const SQLString& columnLabel, const SQLString& value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBytes(int32_t columnIndex, std::string& value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBytes(const SQLString& columnLabel, std::string& value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateDate(int32_t columnIndex, Date date) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateDate(const SQLString& columnLabel, Date value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateTime(int32_t columnIndex, Time time) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateTime(const SQLString& columnLabel, Time value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateTimestamp(int32_t columnIndex, Timestamp timeStamp) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateTimestamp(const SQLString& columnLabel, Timestamp value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateAsciiStream(int32_t columnIndex, std::istream* inputStream, int32_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateAsciiStream(const SQLString& columnLabel, std::istream* inputStream) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateAsciiStream(const SQLString& columnLabel, std::istream* value, int32_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateAsciiStream(int32_t columnIndex, std::istream* inputStream, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateAsciiStream(const SQLString& columnLabel, std::istream* inputStream, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateAsciiStream(int32_t columnIndex, std::istream* inputStream) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBinaryStream(int32_t columnIndex, std::istream* inputStream, int32_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBinaryStream(int32_t columnIndex, std::istream* inputStream, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBinaryStream(const SQLString& columnLabel, std::istream* value, int32_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBinaryStream(const SQLString& columnLabel, std::istream* inputStream, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBinaryStream(int32_t columnIndex, std::istream* inputStream) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBinaryStream(const SQLString& columnLabel, std::istream* inputStream) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateCharacterStream(int32_t columnIndex, std::istringstream& value, int32_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateCharacterStream(int32_t columnIndex, std::istringstream& value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateCharacterStream(const SQLString& columnLabel, std::istringstream& reader, int32_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateCharacterStream(int32_t columnIndex, std::istringstream& value, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateCharacterStream(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateCharacterStream(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateObject(int32_t columnIndex, sql::Object* value, int32_t scaleOrLength) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateObject(int32_t columnIndex, sql::Object* value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateObject(const SQLString& columnLabel, sql::Object* value, int32_t scaleOrLength) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateObject(const SQLString& columnLabel, sql::Object* value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateLong(const SQLString& columnLabel, int64_t value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateLong(int32_t columnIndex, int64_t value) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateRow() {
    throw ExceptionMapper::getFeatureNotSupportedException(
      "updateRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateRef(int32_t columnIndex, Ref& ref) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateRef(const SQLString& columnLabel, Ref& ref) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBlob(int32_t columnIndex, Blob& blob) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBlob(const SQLString& columnLabel, Blob& blob) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBlob(int32_t columnIndex, std::istream* inputStream) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBlob(const SQLString& columnLabel, std::istream* inputStream) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBlob(int32_t columnIndex, std::istream* inputStream, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateBlob(const SQLString& columnLabel, std::istream* inputStream, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateClob(int32_t columnIndex, Clob& clob) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateClob(const SQLString& columnLabel, Clob& clob) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateClob(int32_t columnIndex, std::istringstream& reader, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateClob(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateClob(int32_t columnIndex, std::istringstream& reader) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateClob(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateArray(int32_t columnIndex, sql::Array& array) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateArray(const SQLString& columnLabel, sql::Array& array) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateRowId(int32_t columnIndex, RowId& rowId) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateRowId(const SQLString& columnLabel, RowId& rowId) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNString(int32_t columnIndex, const SQLString& nstring) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNString(const SQLString& columnLabel, const SQLString& nstring) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNClob(int32_t columnIndex, NClob& nclob) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNClob(const SQLString& columnLabel, NClob& nclob) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNClob(int32_t columnIndex, std::istringstream& reader) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNClob(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNClob(int32_t columnIndex, std::istringstream& reader, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNClob(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  void SelectResultSet::updateSQLXML(int32_t columnIndex, SQLXML& xmlObject) {
    throw ExceptionMapper::getFeatureNotSupportedException("SQLXML not supported");
  }

  void SelectResultSet::updateSQLXML(const SQLString& columnLabel, SQLXML& xmlObject) {
    throw ExceptionMapper::getFeatureNotSupportedException("SQLXML not supported");
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateNCharacterStream(int32_t columnIndex, std::istringstream& value, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateNCharacterStream(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSet::updateNCharacterStream(int32_t columnIndex, std::istringstream& reader) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSet::updateNCharacterStream(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionMapper::getFeatureNotSupportedException(NOT_UPDATABLE_ERROR);
  }
#endif

  /** {inheritDoc}. */
  int32_t SelectResultSet::getHoldability() {
    return ResultSet::HOLD_CURSORS_OVER_COMMIT;
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /** {inheritDoc}. */
  bool SelectResultSet::isWrapperFor() {
    return iface.isInstance(this);
  }
#endif

  /** Force metadata getTableName to return table alias, not original table name. */
  void SelectResultSet::setForceTableAlias() {
    this->forceAlias= true;
  }

  void SelectResultSet::rangeCheck(const SQLString& className, int64_t minValue, int64_t maxValue, int64_t value, ColumnInformation* columnInfo) {
    if (value < minValue || value > maxValue) {
      throw SQLException(
        "Out of range value for column '"
        +columnInfo.getName()
        +"' : value "
        + std::to_string(value)
        +" is not in "
        + className
        +" range",
        "22003",
        1264);
    }
  }

  int32_t SelectResultSet::getRowPointer() {
    return rowPointer;
  }

  void SelectResultSet::setRowPointer(int32_t pointer) {
    rowPointer= pointer;
  }

  int32_t SelectResultSet::getDataSize() {
    return dataSize;
  }

  bool SelectResultSet::isBinaryEncoded() {
    return row->isBinaryEncoded();
  }
}
}
