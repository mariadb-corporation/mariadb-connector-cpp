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
#include <sstream>

#include "SelectResultSetBin.h"
#include "Results.h"

#include "MariaDbResultSetMetaData.h"

#include "Protocol.h"
#include "ColumnDefinitionCapi.h"
#include "ExceptionFactory.h"
#include "SqlStates.h"
#include "com/RowProtocol.h"
#include "protocol/capi/BinRowProtocolCapi.h"
#include "protocol/capi/TextRowProtocolCapi.h"
#include "util/ServerPrepareResult.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
  /**
    * Create Streaming resultSet.
    *
    * @param results results
    * @param protocol current protocol
    * @param spr ServerPrepareResult
    * @param callableResult is it from a callableStatement ?
    * @param eofDeprecated is EOF deprecated
    */
  SelectResultSetBin::SelectResultSetBin(Results* results,
                                         Protocol* protocol,
                                         ServerPrepareResult* spr,
                                         bool callableResult,
                                         bool eofDeprecated)
    : SelectResultSet(results->getFetchSize()),
      options(protocol->getOptions()),
      columnsInformation(spr->getColumns()),
      columnInformationLength(static_cast<int32_t>(columnsInformation.size())),
      noBackslashEscapes(protocol->noBackslashEscapes()),
      protocol(protocol),
      callableResult(callableResult),
      statement(results->getStatement()),
      capiStmtHandle(spr->getStatementId()),
      dataSize(0),
      resultSetScrollType(results->getResultSetScrollType()),
      columnNameMap(new ColumnNameMap(columnsInformation)),
      isClosedFlag(false),
      eofDeprecated(eofDeprecated),
      forceAlias(false)
  {
    if (fetchSize == 0 || callableResult) {
      data.reserve(10);//= new char[10]; // This has to be array of arrays. Need to decide what to use for its representation
      if (mysql_stmt_store_result(capiStmtHandle)) {
        throwStmtError(capiStmtHandle);
      }
      dataSize= static_cast<std::size_t>(mysql_stmt_num_rows(capiStmtHandle));
      streaming= false;
      resetVariables();
      row.reset(new capi::BinRowProtocolCapi(columnsInformation, columnInformationLength, results->getMaxFieldSize(), options, capiStmtHandle));
    }
    else {
      lock= protocol->getLock();

      protocol->setActiveStreamingResult(statement->getInternalResults());

      protocol->removeHasMoreResults();
      data.reserve(std::max(10, fetchSize)); // Same
      row.reset(new capi::BinRowProtocolCapi(columnsInformation, columnInformationLength, results->getMaxFieldSize(), options, capiStmtHandle));
      nextStreamingValue();
      streaming= true;
    }
  }


  SelectResultSetBin::~SelectResultSetBin()
  {
    if (!isFullyLoaded()) {
      //close();
      fetchAllResults();
    }
    checkOut();
  }

  /**
    * Indicate if result-set is still streaming results from server.
    *
    * @return true if streaming is finished
    */
  bool SelectResultSetBin::isFullyLoaded() const {
    // result-set is fully loaded when reaching EOF packet.
    return isEof;
  }


  void SelectResultSetBin::fetchAllResults()
  {
    dataSize = 0;
    while (readNextValue()) {
    }
    ++dataFetchTime;
  }

  const char * SelectResultSetBin::getErrMessage()
  {
    return mysql_stmt_error(capiStmtHandle);
  }


  const char * SelectResultSetBin::getSqlState()
  {
    return mysql_stmt_error(capiStmtHandle);
  }

  uint32_t SelectResultSetBin::getErrNo()
  {
    return mysql_stmt_errno(capiStmtHandle);
  }

  uint32_t SelectResultSetBin::warningCount()
  {
    return mysql_stmt_warning_count(capiStmtHandle);
  }

  /**
    * When protocol has a current Streaming result (this) fetch all to permit another query is
    * executing. The lock should be acquired before calling this method
    *
    * @throws SQLException if any error occur
    */
  void SelectResultSetBin::fetchRemaining() {
    if (!isEof) {
      try {
        lastRowPointer = -1;
        if (!isEof && dataSize > 0 && fetchSize == 1) {
          // We need to grow the array till current size. Its main purpose is to create room for newly fetched
          // fetched row, so it grows till dataSize + 1. But we need to space for already fetched(from server)
          // row. Thus fooling growDataArray by decrementing dataSize
          --dataSize;
          growDataArray();
          // Since index of the last row is smaller from dataSize by 1, we have correct index
          row->cacheCurrentRow(data[dataSize], columnsInformation.size());
          rowPointer = 0;
          resetRow();
          ++dataSize;
        }
        /*if (mysql_stmt_store_result(capiStmtHandle) != 0) {
          throwStmtError(capiStmtHandle);
        }*/
        while (!isEof) {
          addStreamingValue(true);
        }
      }
      catch (SQLException & queryException) {
        ExceptionFactory::INSTANCE.create(queryException).Throw();
      }
      catch (std::exception & ioe) {
        handleIoException(ioe);
      }
      dataFetchTime++;
    }
  }

  void SelectResultSetBin::handleIoException(std::exception& ioe) const
  {
    ExceptionFactory::INSTANCE.create(
        "Server has closed the connection. \n"
        "Please check net_read_timeout/net_write_timeout/wait_timeout server variables. "
        "If result set contain huge amount of data, Server expects client to"
        " read off the result set relatively fast. "
        "In this case, please consider increasing net_read_timeout session variable"
        " / processing your result set faster (check Streaming result sets documentation for more information)",
        CONNECTION_EXCEPTION.getSqlState(), &ioe).Throw();
  }

  /**
    * This permit to replace current stream results by next ones.
    *
    * @throws IOException if socket exception occur
    * @throws SQLException if server return an unexpected error
    */
  void SelectResultSetBin::nextStreamingValue() {
    lastRowPointer= -1;

    if (resultSetScrollType == TYPE_FORWARD_ONLY) {
      dataSize= 0;
    }

    addStreamingValue(fetchSize > 1);
  }


  /**
    * Read next value.
    *
    * @return true if have a new value
    * @throws IOException exception
    * @throws SQLException exception
    */
  bool SelectResultSetBin::readNextValue(bool cacheLocally)
  {
    switch (row->fetchNext()) {
    case 1: {
      SQLString err("Internal error: most probably fetch on not yet executed statment handle. ");
      unsigned int nativeErrno = getErrNo();
      err.append(getErrMessage());
      throw SQLException(err, "HY000", nativeErrno );
    }
    case MYSQL_DATA_TRUNCATED: {
      /*protocol->removeActiveStreamingResult();
      protocol->removeHasMoreResults();*/
      protocol->setHasWarnings(true);
      break;

      /*resetVariables();
      throw *ExceptionFactory::INSTANCE.create(
        getErrMessage(),
        getSqlState(),
        getErrNo(),
        nullptr,
        false);*/
    }

    case MYSQL_NO_DATA: {
      uint32_t serverStatus;
      uint32_t warnings;

      if (!eofDeprecated) {
        protocol->readEofPacket();
        warnings= warningCount();
        serverStatus= protocol->getServerStatus();

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
        // protocol->readOkPacket()?
        serverStatus= protocol->getServerStatus();
        warnings= warningCount();;
        callableResult= (serverStatus & PS_OUT_PARAMETERS)!=0;
      }
      protocol->setServerStatus(serverStatus);
      protocol->setHasWarnings(warnings > 0);

      if ((serverStatus & MORE_RESULTS_EXISTS) == 0) {
        protocol->removeActiveStreamingResult();
      }
      resetVariables();
      return false;
    }
    }

    if (cacheLocally) {
      if (dataSize + 1 >= data.size()) {
        growDataArray();
      }
      row->cacheCurrentRow(data[dataSize], columnsInformation.size());
    }
    ++dataSize;
    return true;
  }

  /**
    * Get current row's raw bytes.
    *
    * @return row's raw bytes
    */
  std::vector<sql::bytes>& SelectResultSetBin::getCurrentRowData() {
    return data[rowPointer];
  }

  /**
    * Update row's raw bytes. in case of row update, refresh the data. (format must correspond to
    * current resultset binary/text row encryption)
    *
    * @param rawData new row's raw data.
    */
  void SelectResultSetBin::updateRowData(std::vector<sql::bytes>& rawData)
  {
    data[rowPointer]= rawData;
    row->resetRow(data[rowPointer]);
  }

  /**
    * Delete current data. Position cursor to the previous row->
    *
    * @throws SQLException if previous() fail.
    */
  void SelectResultSetBin::deleteCurrentRowData() {

    data.erase(data.begin()+lastRowPointer);
    dataSize--;
    lastRowPointer= -1;
    previous();
  }

  void SelectResultSetBin::addRowData(std::vector<sql::bytes>& rawData) {
    if (dataSize +1 >= data.size()) {
      growDataArray();
    }
    data[dataSize]= rawData;
    rowPointer= static_cast<int32_t>(dataSize);
    ++dataSize;
  }

  /*int32_t SelectResultSetBin::skipLengthEncodedValue(std::string& buf, int32_t pos) {
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
  }*/

  /** Grow data array. */
  void SelectResultSetBin::growDataArray() {
    std::size_t curSize = data.size();

    if (data.capacity() < curSize + 1) {
      uint64_t newCapacity = static_cast<uint64_t>(curSize + (curSize >> 1));

      if (newCapacity > MAX_ARRAY_SIZE) {
        newCapacity = MAX_ARRAY_SIZE;
      }

      data.reserve(newCapacity);
    }
    for (std::size_t i = curSize; i < dataSize + 1; ++i) {
      data.push_back({});
    }
    data[dataSize].reserve(columnsInformation.size());
  }

  /**
    * Connection.abort() has been called, abort result-set.
    *
    * @throws SQLException exception
    */
  void SelectResultSetBin::abort() {
    isClosedFlag= true;
    resetVariables();

    for (auto& row : data) {
      row.clear();
    }

    if (statement != nullptr) {
      statement->checkCloseOnCompletion(this);
      statement= nullptr;
    }
  }

  /** Close resultSet. */
  void SelectResultSetBin::close() {
    isClosedFlag= true;
    if (!isEof) {
      std::unique_lock<std::mutex> localScopeLock(*lock);
      try {
        while (!isEof) {
          dataSize= 0; // to avoid storing data
          readNextValue();
        }
      }
      catch (SQLException& queryException) {
        ExceptionFactory::INSTANCE.create(queryException).Throw();
      }
      catch (std::runtime_error& ioe) {
        resetVariables();
        handleIoException(ioe);
      }
    }

    checkOut();
    resetVariables();

    data.clear();

    if (statement != nullptr) {
      statement->checkCloseOnCompletion(this);
      statement= nullptr;
    }
  }


  void SelectResultSetBin::resetVariables() {
    protocol= nullptr;
    isEof= true;
  }


  bool SelectResultSetBin::fetchNext()
  {
    ++rowPointer;
    if (data.size() > 0) {
      row->resetRow(data[rowPointer]);
    }
    else {
      if (row->fetchNext() == MYSQL_NO_DATA) {
        return false;
      }
    }
    lastRowPointer= rowPointer;
    return true;
  }

  bool SelectResultSetBin::next()
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
        std::lock_guard<std::mutex> localScopeLock(*lock);
        try {
          if (!isEof) {
            nextStreamingValue();
          }
        }
        catch (std::exception& ioe) {
          handleIoException(ioe);
        }

        if (resultSetScrollType == TYPE_FORWARD_ONLY) {

          rowPointer= 0;
          return dataSize > 0;
        }
        else {
          rowPointer++;
          return dataSize > static_cast<std::size_t>(rowPointer);
        }
      }

      rowPointer= static_cast<int32_t>(dataSize);
      return false;
    }
  }

  // It has to be const, because it's called by getters, and properties it changes are mutable
  void SelectResultSetBin::resetRow() const
  {
    if (data.size() > rowPointer) {
      row->resetRow(const_cast<std::vector<sql::bytes> &>(data[rowPointer]));
    }
    else {
      if (rowPointer != lastRowPointer + 1) {
        row->installCursorAtPosition(rowPointer);
      }
      if (!streaming) {
        row->fetchNext();
      }
    }
    lastRowPointer= rowPointer;
  }


  void SelectResultSetBin::checkObjectRange(int32_t position) const {
    if (rowPointer < 0) {
      throw SQLDataException("Current position is before the first row", "22023");
    }

    if (static_cast<uint32_t>(rowPointer) >= dataSize) {
      throw SQLDataException("Current position is after the last row", "22023");
    }

    if (position <= 0 || position > columnInformationLength) {
      throw IllegalArgumentException("No such column: " + std::to_string(position), "22023");
    }

    if (lastRowPointer != rowPointer) {
      resetRow();
    }
    row->setPosition(position - 1);
  }

  SQLWarning* SelectResultSetBin::getWarnings() {
    if (this->statement == nullptr) {
      return nullptr;
    }
    return this->statement->getWarnings();
  }

  void SelectResultSetBin::clearWarnings() {
    if (this->statement != nullptr) {
      this->statement->clearWarnings();
    }
  }

  bool SelectResultSetBin::isBeforeFirst() const {
    checkClose();
    return (dataFetchTime >0) ? rowPointer == -1 && dataSize > 0 : rowPointer == -1;
  }

  bool SelectResultSetBin::isAfterLast() {
    checkClose();
    if (rowPointer < 0 || static_cast<std::size_t>(rowPointer) < dataSize) {
      // has remaining results
      return false;
    }
    else {
      
      if (streaming && !isEof)
      {
      // has to read more result to know if it's finished or not
      // (next packet may be new data or an EOF packet indicating that there is no more data)
        std::lock_guard<std::mutex> localScopeLock(*lock);
        try {
          // this time, fetch is added even for streaming forward type only to keep current pointer
          // row.
          if (!isEof) {
            addStreamingValue();
          }
        }
        catch (std::exception& ioe) {
          handleIoException(ioe);
        }

        return dataSize == rowPointer;
      }
      // has read all data and pointer is after last result
      // so result would have to always to be true,
      // but when result contain no row at all jdbc say that must return false
      return dataSize >0 ||dataFetchTime >1;
    }
  }

  bool SelectResultSetBin::isFirst() const {
    checkClose();
    return /*dataFetchTime == 1 && */rowPointer == 0 && dataSize > 0;
  }

  bool SelectResultSetBin::isLast() {
    checkClose();
    if (static_cast<std::size_t>(rowPointer + 1) < dataSize) {
      return false;
    }
    else if (isEof) {
      return rowPointer == dataSize - 1 && dataSize > 0;
    }
    else {
      // when streaming and not having read all results,
      // must read next packet to know if next packet is an EOF packet or some additional data
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {
        if (!isEof) {
          addStreamingValue();
        }
      }
      catch (std::exception& ioe) {
        handleIoException(ioe);
      }

      if (isEof) {

        return rowPointer == dataSize - 1 && dataSize > 0;
      }

      return false;
    }
  }

  void SelectResultSetBin::beforeFirst() {
    checkClose();

    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    rowPointer= -1;
  }

  void SelectResultSetBin::afterLast() {
    checkClose();
    if (!isEof) {
      //SelectResultSet objects only have lock if streaming
      std::lock_guard<std::mutex> localScopeLock(*lock);
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize);
  }

  bool SelectResultSetBin::first() {
    checkClose();

    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }

    rowPointer= 0;
    return dataSize > 0;
  }

  bool SelectResultSetBin::last() {
    checkClose();
    if (!isEof) {
      //SelectResultSet objects only have lock if streaming
      std::lock_guard<std::mutex> localScopeLock(*lock);
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize) - 1;
    return dataSize > 0;
  }

  int32_t SelectResultSetBin::getRow() {
    checkClose();
    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      return 0;
    }
    return rowPointer + 1;
  }

  bool SelectResultSetBin::absolute(int32_t rowPos) {
    checkClose();

    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }

    if (rowPos >= 0 && static_cast<uint32_t>(rowPos) <= dataSize) {
      rowPointer= rowPos - 1;
      return true;
    }
    if (!isEof) {
      //SelectResultSet objects only have lock if streaming
      std::lock_guard<std::mutex> localScopeLock(*lock);
      fetchRemaining();
    }
    if (rowPos >= 0) {

      if (static_cast<uint32_t>(rowPos) <= dataSize) {
        rowPointer= rowPos - 1;
        return true;
      }
      rowPointer= static_cast<int32_t>(dataSize);
      return false;
    }
    else {

      // Need to cast, or otherwise the result would be size_t -> always not negative
      if (static_cast<int64_t>(dataSize) + rowPos >= 0) {
        rowPointer= static_cast<int32_t>(dataSize + rowPos);
        return true;
      }
      rowPointer= -1;
      return false;
    }
  }


  bool SelectResultSetBin::relative(int32_t rows) {
    checkClose();
    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    int32_t newPos= rowPointer + rows;
    if (newPos <=-1) {
      rowPointer= -1;
      return false;
    }
    else if (static_cast<uint32_t>(newPos) >= dataSize) {
      rowPointer= static_cast<int32_t>(dataSize);
      return false;
    }
    else {
      rowPointer= newPos;
      return true;
    }
  }

  bool SelectResultSetBin::previous() {
    checkClose();
    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    if (rowPointer > -1) {
      --rowPointer;
      return rowPointer != -1;
    }
    return false;
  }

  int32_t SelectResultSetBin::getFetchDirection() const {
    return FETCH_UNKNOWN;
  }

  void SelectResultSetBin::setFetchDirection(int32_t direction) {
    if (direction == FETCH_REVERSE) {
      throw SQLException(
        "Invalid operation. Allowed direction are ResultSet::FETCH_FORWARD and ResultSet::FETCH_UNKNOWN");
    }
  }

  int32_t SelectResultSetBin::getFetchSize() const {
    return this->fetchSize;
  }

  void SelectResultSetBin::setFetchSize(int32_t fetchSize) {
    if (streaming && fetchSize == 0) {
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {
        // fetch all results
        while (!isEof) {
          addStreamingValue();
        }
      }
      catch (std::exception& ioe) {
        handleIoException(ioe);
      }
      streaming= dataFetchTime == 1;
    }
    this->fetchSize= fetchSize;
  }

  int32_t SelectResultSetBin::getType()  const {
    return resultSetScrollType;
  }

  int32_t SelectResultSetBin::getConcurrency() const {
    return CONCUR_READ_ONLY;
  }

  void SelectResultSetBin::checkClose() const {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
  }

  bool SelectResultSetBin::isCallableResult() const {
    return callableResult;
  }

  bool SelectResultSetBin::isClosed() const {
    return isClosedFlag;
  }

  MariaDbStatement* SelectResultSetBin::getStatement() {
    return statement;
  }

  void SelectResultSetBin::setStatement(MariaDbStatement* statement)
  {
    this->statement= statement;
  }

  /** {inheritDoc}. */
  bool SelectResultSetBin::wasNull() const {
    return row->wasNull();
  }

  bool SelectResultSetBin::isNull(int32_t columnIndex) const
  {
    checkObjectRange(columnIndex);
    return row->lastValueWasNull();
  }

  bool SelectResultSetBin::isNull(const SQLString & columnLabel) const
  {
    return isNull(findColumn(columnLabel));
  }

#ifdef MAYBE_IN_NEXTVERSION
  /** {inheritDoc}. */
  std::istream* SelectResultSetBin::getAsciiStream(const SQLString& columnLabel) {
    return getAsciiStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  std::istream* SelectResultSetBin::getAsciiStream(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    return new ByteArrayInputStream(
      new SQLString(row->buf, row->pos, row->getLengthMaxFieldSize()).c_str());/*, StandardCharsets.UTF_8*/
  }
#endif

  /** {inheritDoc}. */
  SQLString SelectResultSetBin::getString(int32_t columnIndex) const
  {
    checkObjectRange(columnIndex);

    return std::move(row->getInternalString(columnsInformation[columnIndex - 1].get()));
  }

  /** {inheritDoc}. */
  SQLString SelectResultSetBin::getString(const SQLString& columnLabel) const {
    return getString(findColumn(columnLabel));
  }


  SQLString SelectResultSetBin::zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation)
  {
    if (columnInformation->isZeroFill()) {
      SQLString zeroAppendStr;
      int64_t zeroToAdd= columnInformation->getDisplaySize() - value.size();
      while ((zeroToAdd--) > 0) {
        zeroAppendStr.append("0");
      }
      return zeroAppendStr.append(value);
    }
    return value;
  }

  /** {inheritDoc}. */
  std::istream* SelectResultSetBin::getBinaryStream(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    blobBuffer[columnIndex].reset(new memBuf(row->fieldBuf.arr + row->pos, row->fieldBuf.arr + row->pos + row->getLengthMaxFieldSize()));
    return new std::istream(blobBuffer[columnIndex].get());
  }

  /** {inheritDoc}. */
  std::istream* SelectResultSetBin::getBinaryStream(const SQLString& columnLabel) const {
    return getBinaryStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  int32_t SelectResultSetBin::getInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalInt(columnsInformation[columnIndex -1].get());
  }

  /** {inheritDoc}. */  int32_t SelectResultSetBin::getInt(const SQLString& columnLabel) const {
    return getInt(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  int64_t SelectResultSetBin::getLong(const SQLString& columnLabel) const {
    return getLong(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  int64_t SelectResultSetBin::getLong(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalLong(columnsInformation[columnIndex -1].get());
  }


  uint64_t SelectResultSetBin::getUInt64(const SQLString & columnLabel) const {
    return getUInt64(findColumn(columnLabel));
  }


  uint64_t SelectResultSetBin::getUInt64(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return static_cast<uint64_t>(row->getInternalULong(columnsInformation[columnIndex -1].get()));
  }


  uint32_t SelectResultSetBin::getUInt(const SQLString& columnLabel) const {
    return getUInt(findColumn(columnLabel));
  }


  uint32_t SelectResultSetBin::getUInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);

    ColumnDefinition* columnInfo= columnsInformation[columnIndex - 1].get();
    int64_t value= row->getInternalLong(columnInfo);

    row->rangeCheck("uint32_t", 0, UINT32_MAX, value, columnInfo);

    return static_cast<uint32_t>(value);
  }


  /** {inheritDoc}. */
  float SelectResultSetBin::getFloat(const SQLString& columnLabel) const {
    return getFloat(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  float SelectResultSetBin::getFloat(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalFloat(columnsInformation[columnIndex -1].get());
  }

  /** {inheritDoc}. */
  long double SelectResultSetBin::getDouble(const SQLString& columnLabel) const {
    return getDouble(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  long double SelectResultSetBin::getDouble(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalDouble(columnsInformation[columnIndex -1].get());
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /** {inheritDoc}. */
  BigDecimal SelectResultSetBin::getBigDecimal(const SQLString& columnLabel, int32_t scale) {
    return getBigDecimal(findColumn(columnLabel), scale);
  }

  /** {inheritDoc}. */
  BigDecimal SelectResultSetBin::getBigDecimal(int32_t columnIndex, int32_t scale) {
    checkObjectRange(columnIndex);
    return row->getInternalBigDecimal(columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  BigDecimal SelectResultSetBin::getBigDecimal(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalBigDecimal(columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  BigDecimal SelectResultSetBin::getBigDecimal(const SQLString& columnLabel) {
    return getBigDecimal(findColumn(columnLabel));
  }
#endif
#ifdef MAYBE_IN_NEXTVERSION
  /** {inheritDoc}. */
  SQLString SelectResultSetBin::getBytes(const SQLString& columnLabel) {
    return getBytes(findColumn(columnLabel));
  }
  /** {inheritDoc}. */
  SQLString SelectResultSetBin::getBytes(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    char* data= new char[row->getLengthMaxFieldSize()];
    System.arraycopy(row->buf, row->pos, data, 0, row->getLengthMaxFieldSize());
    return data;
  }


  /** {inheritDoc}. */
  Date* SelectResultSetBin::getDate(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalDate(columnsInformation[columnIndex -1], nullptr, timeZone);
  }

  /** {inheritDoc}. */
  Date* SelectResultSetBin::getDate(const SQLString& columnLabel) {
    return getDate(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Time* SelectResultSetBin::getTime(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalTime(columnsInformation[columnIndex -1], nullptr, timeZone);
  }

  /** {inheritDoc}. */  Time SelectResultSetBin::getTime(const SQLString& columnLabel) {
    return getTime(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSetBin::getTimestamp(const SQLString& columnLabel) {
    return getTimestamp(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSetBin::getTimestamp(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalTimestamp(columnsInformation[columnIndex -1], nullptr, timeZone);
  }
#endif

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /** {inheritDoc}. */
  Date* SelectResultSetBin::getDate(int32_t columnIndex, Calendar& cal) {
    checkObjectRange(columnIndex);
    return row->getInternalDate(columnsInformation[columnIndex -1], cal, timeZone);
  }

  /** {inheritDoc}. */
  Date* SelectResultSetBin::getDate(const SQLString& columnLabel, Calendar& cal) {
    return getDate(findColumn(columnLabel), cal);
  }

  /** {inheritDoc}. */
  Time* SelectResultSetBin::getTime(int32_t columnIndex, Calendar& cal) {
    checkObjectRange(columnIndex);
    return row->getInternalTime(columnsInformation[columnIndex -1], cal, timeZone);
  }

  /** {inheritDoc}. */
  Time* SelectResultSetBin::getTime(const SQLString& columnLabel, Calendar& cal) {
    return getTime(findColumn(columnLabel), cal);
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSetBin::getTimestamp(int32_t columnIndex, Calendar& cal) {
    checkObjectRange(columnIndex);
    return row->getInternalTimestamp(columnsInformation[columnIndex -1], cal, timeZone);
  }

  /** {inheritDoc}. */
  Timestamp* SelectResultSetBin::getTimestamp(const SQLString& columnLabel, Calendar& cal) {
    return getTimestamp(findColumn(columnLabel), cal);
  }
#endif

  /** {inheritDoc}. */
  SQLString SelectResultSetBin::getCursorName() {
    throw ExceptionFactory::INSTANCE.notSupported("Cursors not supported");
  }

  /** {inheritDoc}. */
  sql::ResultSetMetaData* SelectResultSetBin::getMetaData() const {
    return new MariaDbResultSetMetaData(columnsInformation, options, forceAlias);
  }

  /** {inheritDoc}. */
  int32_t SelectResultSetBin::findColumn(const SQLString& columnLabel) const {
    return columnNameMap->getIndex(columnLabel) + 1;
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED

  /** {inheritDoc}. */
  sql::Object* SelectResultSetBin::getObject(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    return row->getInternalObject(columnsInformation[columnIndex -1], timeZone);
  }

  /** {inheritDoc}. */
  sql::Object* SelectResultSetBin::getObject(const SQLString& columnLabel) {
    return getObject(findColumn(columnLabel));
  }

  template <class T>T getObject(int32_t columnIndex, Classtemplate <class T>type) {
    if (type.empty() == true) {
      throw SQLException("Class type cannot be NULL");
    }
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    ColumnDefinition col= columnsInformation[columnIndex -1];

    if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalString(col, nullptr, timeZone);

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
      return (T)row->getInternalDate(col, nullptr, timeZone);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalTime(col, nullptr, timeZone);

    }
    else if ((type.compare(SQLString.class) == 0)||((type.compare(SQLString.class) == 0)) {
      return (T)row->getInternalTimestamp(col, nullptr, timeZone);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      return (T)(Boolean)row->getInternalBoolean(col);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      calendar=  .getInstance(timeZone);
      Timestamp timestamp= row->getInternalTimestamp(col, nullptr, timeZone);
      if (timestamp.empty() == true) {
        return nullptr;
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
      SQLString value= row->getInternalString(col, nullptr, timeZone);
      if (value.empty() == true) {
        return nullptr;
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
        ? nullptr
        : type.cast(zonedDateTime.withZoneSameInstant(ZoneId.systemDefault()).toLocalDateTime());

    }
    else if ((type.compare(SQLString.class) == 0)) {
      ZonedDateTime zonedDateTime =
        row->getInternalZonedDateTime(col, ZonedDateTime.class, timeZone);
      if (zonedDateTime.empty() == true) {
        return nullptr;
      }
      return type.cast(row->getInternalZonedDateTime(col, ZonedDateTime.class, timeZone));

    }
    else if ((type.compare(SQLString.class) == 0)) {
      ZonedDateTime tmpZonedDateTime =
        row->getInternalZonedDateTime(col, OffsetDateTime.class, timeZone);
      return tmpZonedDateTime.empty() == true ? nullptr : type.cast(tmpZonedDateTime.toOffsetDateTime());

    }
    else if ((type.compare(SQLString.class) == 0)) {
      LocalDate localDate= row->getInternalLocalDate(col, timeZone);
      if (localDate.empty() == true) {
        return nullptr;
      }
      return type.cast(localDate);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      LocalDate localDate= row->getInternalLocalDate(col, timeZone);
      if (localDate.empty() == true) {
        return nullptr;
      }
      return type.cast(localDate);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      LocalTime localTime= row->getInternalLocalTime(col, timeZone);
      if (localTime.empty() == true) {
        return nullptr;
      }
      return type.cast(localTime);

    }
    else if ((type.compare(SQLString.class) == 0)) {
      OffsetTime offsetTime= row->getInternalOffsetTime(col, timeZone);
      if (offsetTime.empty() == true) {
        return nullptr;
      }
      return type.cast(offsetTime);
    }
    throw ExceptionFactory::INSTANCE.notSupported(
      "Type class '"+type.getName()+"' is not supported");
  }

  /** {inheritDoc}. */
  std::istringstream* SelectResultSetBin::getCharacterStream(const SQLString& columnLabel) {
    return getCharacterStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  std::istringstream* SelectResultSetBin::getCharacterStream(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    SQLString value= row->getInternalString(columnsInformation[columnIndex -1], nullptr, timeZone);
    if (value.empty() == true) {
      return nullptr;
    }
    return new StringReader(value);
  }

  /** {inheritDoc}. */
  std::istringstream* SelectResultSetBin::getNCharacterStream(int32_t columnIndex) {
    return getCharacterStream(columnIndex);
  }

  /** {inheritDoc}. */
  std::istringstream* SelectResultSetBin::getNCharacterStream(const SQLString& columnLabel) {
    return getCharacterStream(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  Ref* SelectResultSetBin::getRef(int32_t columnIndex) {

    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  Ref* SelectResultSetBin::getRef(const SQLString& columnLabel) {
    throw ExceptionFactory::INSTANCE.notSupported("Getting REFs not supported");
  }

  /** {inheritDoc}. */
  Clob* SelectResultSetBin::getClob(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    return new MariaDbClob(row->buf, row->pos, row->length);
  }

  /** {inheritDoc}. */
  Clob* SelectResultSetBin::getClob(const SQLString& columnLabel) {
    return getClob(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  sql::Array* SelectResultSetBin::getArray(int32_t columnIndex) {
    throw ExceptionFactory::INSTANCE.notSupported("Arrays are not supported");
  }

  /** {inheritDoc}. */
  sql::Array* SelectResultSetBin::getArray(const SQLString& columnLabel) {
    return getArray(findColumn(columnLabel));
  }


  URL* SelectResultSetBin::getURL(int32_t columnIndex)
  {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    try {
      return new URL(row->getInternalString(columnsInformation[columnIndex -1], nullptr, timeZone));
    }
    catch (MalformedURLException& e) {
      throw ExceptionMapper::getSqlException("Could not parse as URL");
    }
  }

  URL* SelectResultSetBin::getURL(const SQLString& columnLabel) {
    return getURL(findColumn(columnLabel));
  }


  /** {inheritDoc}. */
  NClob* SelectResultSetBin::getNClob(int32_t columnIndex) {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    return new MariaDbClob(row->buf, row->pos, row->length);
  }

  /** {inheritDoc}. */
  NClob* SelectResultSetBin::getNClob(const SQLString& columnLabel) {
    return getNClob(findColumn(columnLabel));
  }

  SQLXML* SelectResultSetBin::getSQLXML(int32_t columnIndex) {
    throw ExceptionFactory::INSTANCE.notSupported("SQLXML not supported");
  }

  SQLXML* SelectResultSetBin::getSQLXML(const SQLString& columnLabel) {
    throw ExceptionFactory::INSTANCE.notSupported("SQLXML not supported");
  }

  /** {inheritDoc}. */  SQLString SelectResultSetBin::getNString(int32_t columnIndex) {
    return getString(columnIndex);
  }

  /** {inheritDoc}. */
  SQLString SelectResultSetBin::getNString(const SQLString& columnLabel) {
    return getString(findColumn(columnLabel));
  }
#endif

  /** {inheritDoc}. */
  Blob* SelectResultSetBin::getBlob(int32_t columnIndex) const {
    return getBinaryStream(columnIndex);
  }

  /** {inheritDoc}. */
  Blob* SelectResultSetBin::getBlob(const SQLString& columnLabel) const {
    return getBlob(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  RowId* SelectResultSetBin::getRowId(int32_t columnIndex) const {
    throw ExceptionFactory::INSTANCE.notSupported("RowIDs not supported");
  }

  /** {inheritDoc}. */
  RowId* SelectResultSetBin::getRowId(const SQLString& columnLabel) const {
    throw ExceptionFactory::INSTANCE.notSupported("RowIDs not supported");
  }

  /** {inheritDoc}. */
  bool SelectResultSetBin::getBoolean(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalBoolean(columnsInformation[static_cast<std::size_t>(index) -1].get());
  }

  /** {inheritDoc}. */
  bool SelectResultSetBin::getBoolean(const SQLString& columnLabel) const {
    return getBoolean(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  int8_t SelectResultSetBin::getByte(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalByte(columnsInformation[static_cast<std::size_t>(index) - 1].get());
  }

  /** {inheritDoc}. */
  int8_t SelectResultSetBin::getByte(const SQLString& columnLabel) const {
    return getByte(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  short SelectResultSetBin::getShort(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalShort(columnsInformation[static_cast<std::size_t>(index) - 1].get());
  }

  /** {inheritDoc}. */
  short SelectResultSetBin::getShort(const SQLString& columnLabel) const {
    return getShort(findColumn(columnLabel));
  }

  /** {inheritDoc}. */
  bool SelectResultSetBin::rowUpdated() {
    throw ExceptionFactory::INSTANCE.notSupported(
      "Detecting row updates are not supported");
  }

  /** {inheritDoc}. */
  bool SelectResultSetBin::rowInserted() {
    throw ExceptionFactory::INSTANCE.notSupported("Detecting inserts are not supported");
  }

  /** {inheritDoc}. */
  bool SelectResultSetBin::rowDeleted() {
    throw ExceptionFactory::INSTANCE.notSupported("Row deletes are not supported");
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::insertRow() {
    throw ExceptionFactory::INSTANCE.notSupported(
      "insertRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::deleteRow() {
    throw ExceptionFactory::INSTANCE.notSupported(
      "deleteRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::refreshRow() {
    throw ExceptionFactory::INSTANCE.notSupported(
      "refreshRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::moveToInsertRow() {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::moveToCurrentRow() {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::cancelRowUpdates() {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  std::size_t sql::mariadb::capi::SelectResultSetBin::rowsCount() const
  {
    return dataSize;
  }

#ifdef RS_UPDATE_FUNCTIONALITY_IMPLEMENTED
  /** {inheritDoc}. */
  void SelectResultSetBin::updateNull(int32_t columnIndex) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNull(const SQLString& columnLabel) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateBoolean(int32_t columnIndex, bool _bool) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateBoolean(const SQLString& columnLabel, bool value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateByte(int32_t columnIndex, char value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateByte(const SQLString& columnLabel, char value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateShort(int32_t columnIndex, short value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateShort(const SQLString& columnLabel, short value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateInt(int32_t columnIndex, int32_t value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateInt(const SQLString& columnLabel, int32_t value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateFloat(int32_t columnIndex, float value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateFloat(const SQLString& columnLabel, float value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateDouble(int32_t columnIndex, double value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateDouble(const SQLString& columnLabel, double value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBigDecimal(int32_t columnIndex, BigDecimal value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBigDecimal(const SQLString& columnLabel, BigDecimal value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateString(int32_t columnIndex, const SQLString& value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateString(const SQLString& columnLabel, const SQLString& value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBytes(int32_t columnIndex, std::string& value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBytes(const SQLString& columnLabel, std::string& value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateDate(int32_t columnIndex, Date date) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateDate(const SQLString& columnLabel, Date value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateTime(int32_t columnIndex, Time time) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateTime(const SQLString& columnLabel, Time value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateTimestamp(int32_t columnIndex, Timestamp timeStamp) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateTimestamp(const SQLString& columnLabel, Timestamp value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateAsciiStream(int32_t columnIndex, std::istream* inputStream, int32_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateAsciiStream(const SQLString& columnLabel, std::istream* inputStream) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateAsciiStream(const SQLString& columnLabel, std::istream* value, int32_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateAsciiStream(int32_t columnIndex, std::istream* inputStream, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateAsciiStream(const SQLString& columnLabel, std::istream* inputStream, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateAsciiStream(int32_t columnIndex, std::istream* inputStream) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBinaryStream(int32_t columnIndex, std::istream* inputStream, int32_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBinaryStream(int32_t columnIndex, std::istream* inputStream, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBinaryStream(const SQLString& columnLabel, std::istream* value, int32_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBinaryStream(const SQLString& columnLabel, std::istream* inputStream, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBinaryStream(int32_t columnIndex, std::istream* inputStream) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBinaryStream(const SQLString& columnLabel, std::istream* inputStream) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateCharacterStream(int32_t columnIndex, std::istringstream& value, int32_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateCharacterStream(int32_t columnIndex, std::istringstream& value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateCharacterStream(const SQLString& columnLabel, std::istringstream& reader, int32_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateCharacterStream(int32_t columnIndex, std::istringstream& value, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateCharacterStream(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateCharacterStream(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateObject(int32_t columnIndex, sql::Object* value, int32_t scaleOrLength) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateObject(int32_t columnIndex, sql::Object* value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateObject(const SQLString& columnLabel, sql::Object* value, int32_t scaleOrLength) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateObject(const SQLString& columnLabel, sql::Object* value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateLong(const SQLString& columnLabel, int64_t value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateLong(int32_t columnIndex, int64_t value) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateRow() {
    throw ExceptionFactory::INSTANCE.notSupported(
      "updateRow are not supported when using ResultSet::CONCUR_READ_ONLY");
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateRef(int32_t columnIndex, Ref& ref) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateRef(const SQLString& columnLabel, Ref& ref) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBlob(int32_t columnIndex, Blob& blob) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBlob(const SQLString& columnLabel, Blob& blob) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBlob(int32_t columnIndex, std::istream* inputStream) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBlob(const SQLString& columnLabel, std::istream* inputStream) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBlob(int32_t columnIndex, std::istream* inputStream, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateBlob(const SQLString& columnLabel, std::istream* inputStream, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateClob(int32_t columnIndex, Clob& clob) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateClob(const SQLString& columnLabel, Clob& clob) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateClob(int32_t columnIndex, std::istringstream& reader, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateClob(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateClob(int32_t columnIndex, std::istringstream& reader) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateClob(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateArray(int32_t columnIndex, sql::Array& array) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateArray(const SQLString& columnLabel, sql::Array& array) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateRowId(int32_t columnIndex, RowId& rowId) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateRowId(const SQLString& columnLabel, RowId& rowId) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateNString(int32_t columnIndex, const SQLString& nstring) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateNString(const SQLString& columnLabel, const SQLString& nstring) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateNClob(int32_t columnIndex, NClob& nclob) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateNClob(const SQLString& columnLabel, NClob& nclob) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNClob(int32_t columnIndex, std::istringstream& reader) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNClob(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNClob(int32_t columnIndex, std::istringstream& reader, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */  void SelectResultSetBin::updateNClob(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  void SelectResultSetBin::updateSQLXML(int32_t columnIndex, SQLXML& xmlObject) {
    throw ExceptionFactory::INSTANCE.notSupported("SQLXML not supported");
  }

  void SelectResultSetBin::updateSQLXML(const SQLString& columnLabel, SQLXML& xmlObject) {
    throw ExceptionFactory::INSTANCE.notSupported("SQLXML not supported");
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNCharacterStream(int32_t columnIndex, std::istringstream& value, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNCharacterStream(const SQLString& columnLabel, std::istringstream& reader, int64_t length) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNCharacterStream(int32_t columnIndex, std::istringstream& reader) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }

  /** {inheritDoc}. */
  void SelectResultSetBin::updateNCharacterStream(const SQLString& columnLabel, std::istringstream& reader) {
    throw ExceptionFactory::INSTANCE.notSupported(NOT_UPDATABLE_ERROR);
  }
#endif

  /** {inheritDoc}. */
  int32_t SelectResultSetBin::getHoldability() const {
    return ResultSet::HOLD_CURSORS_OVER_COMMIT;
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /** {inheritDoc}. */
  bool SelectResultSetBin::isWrapperFor() {
    return iface.isInstance(this);
  }
#endif

  /** Force metadata getTableName to return table alias, not original table name. */
  void SelectResultSetBin::setForceTableAlias() {
    this->forceAlias= true;
  }

  void SelectResultSetBin::rangeCheck(const SQLString& className, int64_t minValue, int64_t maxValue, int64_t value, ColumnDefinition* columnInfo) {
    if (value < minValue || value > maxValue) {
      throw SQLException(
        "Out of range value for column '"
        +columnInfo->getName()
        +"' : value "
        + std::to_string(value)
        +" is not in "
        + className
        +" range",
        "22003",
        1264);
    }
  }

  int32_t SelectResultSetBin::getRowPointer() {
    return rowPointer;
  }

  void SelectResultSetBin::setRowPointer(int32_t pointer) {
    rowPointer= pointer;
  }

  void sql::mariadb::capi::SelectResultSetBin::checkOut()
  {
    if (released && statement != nullptr && statement->getInternalResults()) {
      statement->getInternalResults()->checkOut(this);
    }
  }


  std::size_t SelectResultSetBin::getDataSize() {
    return dataSize;
  }


  bool SelectResultSetBin::isBinaryEncoded() {
    return row->isBinaryEncoded();
  }


  void SelectResultSetBin::realClose(bool noLock)
  {
    isClosedFlag = true;
    if (!isEof) {
      if (!noLock) {
        lock->lock();
      }
      try {
        while (!isEof) {
          dataSize = 0; // to avoid storing data
          readNextValue();
        }
      }
      catch (SQLException & queryException) {
        if (!noLock) {
          lock->unlock();
        }
        ExceptionFactory::INSTANCE.create(queryException).Throw();
      }
      catch (std::runtime_error & ioe) {
        if (!noLock) {
          lock->unlock();
        }
        resetVariables();
        handleIoException(ioe);
      }
      if (!noLock) {
        lock->unlock();
      }
    }

    checkOut();
    resetVariables();

    data.clear();

    if (statement != nullptr) {
      statement->checkCloseOnCompletion(this);
      statement= nullptr;
    }
  }
}
}
}
