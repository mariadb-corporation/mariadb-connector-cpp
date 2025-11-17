/************************************************************************************
   Copyright (C) 2020, 2023 MariaDB Corporation AB

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
#include <algorithm>

#include "ResultSetText.h"
#include "Results.h"
#include "Protocol.h"
#include "ColumnDefinition.h"
#include "interface/Row.h"
#include "Exception.h"
#include "class/BinRow.h"
#include "class/TextRow.h"
#include "interface/PreparedStatement.h"
#include "ResultSetMetaData.h"


namespace mariadb
{
  ResultSetText::ResultSetText(Results* results,
                               Protocol * _protocol,
                               MYSQL* capiConnHandle)
    : ResultSet(_protocol, results),
      capiConnHandle(capiConnHandle)
  {
    MYSQL_RES* textNativeResults= nullptr;
    if (fetchSize == 0) {
      data.reserve(10);
      textNativeResults= mysql_store_result(capiConnHandle);

      if (textNativeResults == nullptr && mysql_errno(capiConnHandle) != 0) {
        throw 1;
      }
      dataSize= static_cast<size_t>(textNativeResults != nullptr ? mysql_num_rows(textNativeResults) : 0);
      streaming= false;
      resetVariables();
    }
    else {

      protocol->setActiveStreamingResult(results);

      data.reserve(std::max(10, fetchSize)); // Same
      textNativeResults= mysql_use_result(capiConnHandle);

      streaming= true;
    }
    uint32_t fieldCnt= mysql_field_count(capiConnHandle);

    columnsInformation.reserve(fieldCnt);

    for (size_t i= 0; i < fieldCnt; ++i) {
      columnsInformation.emplace_back(mysql_fetch_field(textNativeResults));
    }
    row= new TextRow(textNativeResults);

    columnInformationLength= static_cast<int32_t>(columnsInformation.size());

    /*if (streaming) {
      nextStreamingValue();
    }*/
  }

  /**
    * Create filled result-set.
    *
    * @param columnInformation column information
    * @param resultSet result-set data
    * @param protocol current protocol
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet::TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    */
  ResultSetText::ResultSetText(
    std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    Protocol * _protocol,
    int32_t resultSetScrollType)
    : ResultSet(_protocol, columnInformation, resultSet, resultSetScrollType)
  {
  }


  ResultSetText::ResultSetText(
    const MYSQL_FIELD* field,
    std::vector<std::vector<mariadb::bytes_view>>& rs,
    Protocol * _protocol,
    int32_t rsScrollType)
    : ResultSet(_protocol, field, rs, rsScrollType)
  {
  }

  ResultSetText::~ResultSetText()
  {
    if (!isFullyLoaded()) {
      try
      {
        // It can throw
        flushPendingServerResults();
      }
      catch (...)//(SQLException&)
      {
        // eating exception
      }
    }
    checkOut();
  }

  /**
    * Indicate if result-set is still streaming results from server.
    *
    * @return true if streaming is finished
    */
  bool ResultSetText::isFullyLoaded() const {
    // result-set is fully loaded when reaching EOF packet.
    return isEof;
  }

  // Reading everything w/out caching
  void ResultSetText::flushPendingServerResults()
  {
    dataSize= 0;
    while (readNextValue()) {
    }
    ++dataFetchTime;
  }


  void ResultSetText::cacheCompleteLocally()
  {
    // Fetching and caching locally if streaming. Otherwise it is cached in the Row object
    if (fetchSize > 0) {
      fetchRemaining();
    }
  }


  const char* ResultSetText::getErrMessage()
  {
    if (capiConnHandle != nullptr)
    {
      return mysql_error(capiConnHandle);
    }
    return "";
  }


  const char * ResultSetText::getSqlState()
  {
    if (capiConnHandle != nullptr)
    {
      return mysql_error(capiConnHandle);
    }
    return "HY000";
  }

  uint32_t ResultSetText::getErrNo()
  {
    if (capiConnHandle != nullptr)
    {
      return mysql_errno(capiConnHandle);
    }
    return 0;
  }

  uint32_t ResultSetText::warningCount()
  {
    if (capiConnHandle != nullptr)
    {
      return mysql_warning_count(capiConnHandle);
    }
    return 0;
  }

  /**
    * When protocol has a current Streaming result (this) fetch all to permit another query is
    * executing. The lock should be acquired before calling this method
    *
    * @throws SQLException if any error occur
    */
  void ResultSetText::fetchRemaining() {
    if (!isEof) {
      try {
        lastRowPointer= -1;
        // Making copy of the current row in C/C in local cache.
        if (!isEof && dataSize > 0 && fetchSize == 1) {
          // We need to grow the array till current size. Its main purpose is to create room for newly fetched
          // fetched row, so it grows till dataSize + 1. But we need to space for already fetched(from server)
          // row. Thus fooling growDataArray by decrementing dataSize
          --dataSize;
          growDataArray();
          // Since index of the last row is smaller from dataSize by 1, we have correct index
          row->cacheCurrentRow(data[dataSize], columnsInformation.size());
          // If we were at some position - it becomes 0, if we were not - then we are not
          if (rowPointer > 0) {
            rowPointer= 0;
            resetRow();
          }
          ++dataSize;
        }
        while (!isEof) {
          addStreamingValue(true);
        }
      }
      catch (SQLException& queryException) {
        throw queryException;
      }
      ++dataFetchTime;
    }
  }


  /**
    * Read next value.
    *
    * @return true if have a new value
    * @throws IOException exception
    * @throws SQLException exception
    */
  bool ResultSetText::readNextValue(bool cacheLocally)
  {
    switch (row->fetchNext()) {
    case MYSQL_DATA_TRUNCATED: {
      protocol->removeActiveStreamingResult();
      protocol->removeHasMoreResults();
      break;
    }
    case 1: {
      // mysql_fetch_row may return NULL if there are no more rows, but also possible an error, that in case of result use, detected on first fetch
      if (capiConnHandle != nullptr && mysql_errno(capiConnHandle) != 0) {
        throw SQLException(getErrMessage(), "HY000", getErrNo());;
      }
      // else we are falling thru to MYSQL_NO_DATA
    }
    case MYSQL_NO_DATA: {
      uint32_t serverStatus= protocol->getServerStatus();
      //protocol->setHasWarnings(warningCount() > 0);

      if ((serverStatus & SERVER_MORE_RESULTS_EXIST) == 0) {
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
  std::vector<mariadb::bytes_view>& ResultSetText::getCurrentRowData() {
    return data[rowPointer];
  }

  /**
    * Update row's raw bytes. in case of row update, refresh the data. (format must correspond to
    * current resultset binary/text row encryption)
    *
    * @param rawData new row's raw data.
    */
  void ResultSetText::updateRowData(std::vector<mariadb::bytes_view>& rawData)
  {
    data[rowPointer]= rawData;
    row->resetRow(data[rowPointer]);
  }

  /**
    * Delete current data. Position cursor to the previous row->
    *
    * @throws SQLException if previous() fail.
    */
  void ResultSetText::deleteCurrentRowData() {

    data.erase(data.begin()+lastRowPointer);
    dataSize--;
    lastRowPointer= -1;
    previous();
  }

  void ResultSetText::addRowData(std::vector<mariadb::bytes_view>& rawData) {
    if (dataSize + 1 >= data.size()) {
      growDataArray();
    }
    data[dataSize]= rawData;
    rowPointer= static_cast<int32_t>(dataSize);
    ++dataSize;
  }

  /** Grow data array. */
  void ResultSetText::growDataArray() {
    std::size_t curSize= data.size();

    if (data.capacity() < curSize + 1) {
      std::size_t newCapacity= static_cast<std::size_t>(curSize + (curSize >> 1));

      if (newCapacity > MAX_ARRAY_SIZE) {
        newCapacity= static_cast<std::size_t>(MAX_ARRAY_SIZE);
      }

      data.reserve(newCapacity);
    }
    for (std::size_t i= curSize; i < dataSize + 1; ++i) {
      data.push_back({});
    }
    data[dataSize].reserve(columnsInformation.size());
  }

  /**
    * Connection.abort() has been called, abort result-set.
    *
    * @throws SQLException exception
    */
  void ResultSetText::abort() {
    isClosedFlag= true;
    resetVariables();

    for (auto& row : data) {
      row.clear();
    }

    if (statement != nullptr) {
      //statement->checkCloseOnCompletion(this);
      statement= nullptr;
    }
  }

  /** Close resultSet. */
  void ResultSetText::close() {
    realClose(false);
  }


  bool ResultSetText::isBeforeFirst() const {
    checkClose();
    return (dataFetchTime >0) ? rowPointer == -1 && dataSize > 0 : rowPointer == -1;
  }


  bool ResultSetText::isAfterLast() {
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

  bool ResultSetText::isFirst() const {
    checkClose();
    return /*dataFetchTime == 1 && */rowPointer == 0 && dataSize > 0;
  }

  bool ResultSetText::isLast() {
    checkClose();
    if (static_cast<std::size_t>(rowPointer + 1) < dataSize) {
      return false;
    }
    else if (isEof) {
      return rowPointer == dataSize -1 && dataSize >0;
    }
    else {
      // when streaming and not having read all results,
      // must read next packet to know if next packet is an EOF packet or some additional data
      try {
        if (!isEof) {
          addStreamingValue();
        }
      }
      catch (std::exception& ioe) {
        handleIoException(ioe);
      }

      if (isEof) {

        return rowPointer == dataSize -1 &&dataSize >0;
      }

      return false;
    }
  }

  void ResultSetText::beforeFirst() {
    checkClose();

    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    rowPointer= -1;
  }

  void ResultSetText::afterLast() {
    checkClose();
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize);
  }

  bool ResultSetText::first() {
    checkClose();

    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }

    rowPointer= 0;
    return dataSize > 0;
  }

  bool ResultSetText::last() {
    checkClose();
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize) - 1;
    return dataSize > 0;
  }

  int64_t ResultSetText::getRow() {
    checkClose();
    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      return 0;
    }
    return rowPointer + 1;
  }

  bool ResultSetText::absolute(int64_t rowPos) {
    checkClose();

    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }

    if (rowPos >= 0 && static_cast<uint32_t>(rowPos) <= dataSize) {
      rowPointer= static_cast<int32_t>(rowPos - 1);
      return true;
    }
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }

    if (rowPos >= 0) {

      if (static_cast<uint32_t>(rowPos) <= dataSize) {
        rowPointer= static_cast<int32_t>(rowPos - 1);
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


  bool ResultSetText::relative(int64_t rows) {
    checkClose();
    if (streaming &&resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    int32_t newPos= static_cast<int32_t>(rowPointer + rows);
    if (newPos <= -1) {
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

  bool ResultSetText::previous() {
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


  int32_t ResultSetText::getFetchSize() const {
    return this->fetchSize;
  }

  void ResultSetText::setFetchSize(int32_t fetchSize) {
    if (streaming &&fetchSize == 0) {
      try {

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

  int32_t ResultSetText::getType()  const {
    return resultSetScrollType;
  }

  void ResultSetText::checkClose() const {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
  }

  bool ResultSetText::isCallableResult() const {
    return false;
  }

  bool ResultSetText::isClosed() const {
    return isClosedFlag;
  }


  PreparedStatement* ResultSetText::getStatement() {
    return statement;
  }


  /** {inheritDoc}. */
  bool ResultSetText::wasNull() const {
    return row->wasNull();
  }

  /** {inheritDoc}. */
  SQLString ResultSetText::getString(int32_t columnIndex) const
  {
    checkObjectRange(columnIndex);
    return std::move(row->getInternalString(&columnsInformation[columnIndex - 1]));
  }


  SQLString ResultSetText::zeroFillingIfNeeded(const SQLString& value, const ColumnDefinition* columnInformation)
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
  std::istream* ResultSetText::getBinaryStream(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    blobBuffer[columnIndex].reset(new memBuf(const_cast<char*>(row->fieldBuf.arr) + row->pos, const_cast<char*>(row->fieldBuf.arr) + row->pos + row->getLengthMaxFieldSize()));
    return new std::istream(blobBuffer[columnIndex].get());
  }


  /** {inheritDoc}. */
  int32_t ResultSetText::getInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalInt(&columnsInformation[columnIndex -1]);
  }


  /** {inheritDoc}. */
  int64_t ResultSetText::getLong(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalLong(&columnsInformation[columnIndex -1]);
  }


  uint64_t ResultSetText::getUInt64(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return static_cast<uint64_t>(row->getInternalULong(&columnsInformation[columnIndex -1]));
  }


  uint32_t ResultSetText::getUInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);

    const ColumnDefinition* columnInfo= &columnsInformation[columnIndex - 1];
    int64_t value= row->getInternalLong(columnInfo);

    row->rangeCheck("uint32_t", 0, UINT32_MAX, value, columnInfo);

    return static_cast<uint32_t>(value);
  }


  /** {inheritDoc}. */
  float ResultSetText::getFloat(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalFloat(&columnsInformation[columnIndex -1]);
  }


  /** {inheritDoc}. */
  long double ResultSetText::getDouble(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalDouble(&columnsInformation[columnIndex -1]);
  }


  /** {inheritDoc}. */
  ResultSetMetaData* ResultSetText::getMetaData() const {
    return new ResultSetMetaData(columnsInformation, forceAlias);
  }


  /** {inheritDoc}. */
  bool ResultSetText::getBoolean(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalBoolean(&columnsInformation[static_cast<std::size_t>(index) -1]);
  }

  /** {inheritDoc}. */
  int8_t ResultSetText::getByte(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalByte(&columnsInformation[static_cast<std::size_t>(index) - 1]);
  }


  /** {inheritDoc}. */
  short ResultSetText::getShort(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalShort(&columnsInformation[static_cast<std::size_t>(index) - 1]);
  }


  std::size_t ResultSetText::rowsCount() const
  {
    return dataSize;
  }


  /** Force metadata getTableName to return table alias, not original table name. */
  void ResultSetText::setForceTableAlias() {
    this->forceAlias= true;
  }


  void ResultSetText::rangeCheck(const SQLString& className, int64_t minValue, int64_t maxValue, int64_t value, const ColumnDefinition* columnInfo) {
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


  int32_t ResultSetText::getRowPointer() {
    return rowPointer;
  }

  void ResultSetText::setRowPointer(int32_t pointer) {
    rowPointer= pointer;
  }

  void ResultSetText::checkOut()
  {
    if (statement != nullptr && statement->getInternalResults()) {
      statement->getInternalResults()->checkOut(this);
    }
  }

  std::size_t ResultSetText::getDataSize() {
    return dataSize;
  }


  bool ResultSetText::isBinaryEncoded() {
    return row->isBinaryEncoded();
  }


  void ResultSetText::realClose(bool noLock)
  {
    isClosedFlag= true;
    if (!isEof) {

      try {
        while (!isEof) {
          dataSize= 0; // to avoid storing data
          readNextValue();
        }
      }
      catch (SQLException& queryException) {
        throw queryException;
      }
      catch (std::runtime_error & ioe) {
        resetVariables();
        handleIoException(ioe);
      }
    }

    checkOut();
    resetVariables();

    data.clear();

    if (statement != nullptr) {
      statement= nullptr;
    }
  }


  void ResultSetText::bind(MYSQL_BIND* bind)
  {
    resultBind= bind;
  }

  bool ResultSetText::get(MYSQL_BIND* bind, uint32_t colIdx0based, uint64_t offset)
  {
    checkObjectRange(colIdx0based + 1);
    return getCached(bind, colIdx0based, offset);
  }

  bool ResultSetText::get()
  {
    return fillBuffers(resultBind);
  }

} // namespace mariadb
