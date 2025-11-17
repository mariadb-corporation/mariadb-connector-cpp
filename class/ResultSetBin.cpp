/************************************************************************************
   Copyright (C) 2022,2023 MariaDB Corporation AB

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

#include "ResultSetBin.h"
#include "Results.h"

#include "ColumnDefinition.h"
#include "Row.h"
#include "BinRow.h"
#include "TextRow.h"
#include "ServerPrepareResult.h"
#include "Exception.h"
#include "PreparedStatement.h"
#include "ResultSetMetaData.h"
#include "Protocol.h"

#ifdef max
# undef max
#endif // max

namespace mariadb
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
  ResultSetBin::ResultSetBin(Results* results,
                             Protocol* guard,
                             ServerPrepareResult* spr,
                             bool callable)
    : ResultSet(guard, results, spr->getColumns()),
      callableResult(callable),
      capiStmtHandle(spr->getStatementId()),
      resultBind(nullptr),
      cache(mysql_stmt_field_count(spr->getStatementId()))
  {
    if (fetchSize == 0 || callableResult) {
      data.reserve(callable ? 1 : 10);
      if (mysql_stmt_store_result(capiStmtHandle)) {
        throw 1;
      }
      dataSize= static_cast<std::size_t>(mysql_stmt_num_rows(capiStmtHandle));
      resetVariables();
      row= new BinRow(columnsInformation, columnInformationLength, capiStmtHandle);
    }
    else {
      protocol->setActiveStreamingResult(results);
      data.reserve(std::max(10, fetchSize));
      row= new BinRow(columnsInformation, columnInformationLength, capiStmtHandle);
      streaming= true;
    }
  }


  ResultSetBin::~ResultSetBin()
  {
    if (!isFullyLoaded()) {
      try
      {
        // It can throw
        flushPendingServerResults();
      }
      catch (...)//(SQLException&)
      {
        1;
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
  bool ResultSetBin::isFullyLoaded() const {
    // result-set is fully loaded when reaching EOF packet.
    return isEof;
  }


  void ResultSetBin::flushPendingServerResults()
  {
    dataSize= 0;
    while (readNextValue()) {
    }
    ++dataFetchTime;
  }


  void ResultSetBin::cacheCompleteLocally()
  {
    if (data.size()) {
      // we have already it cached
      return;
    }
    auto preservedPosition= rowPointer;
    // fetchRemaining does remaining stream
    if (streaming) {
      fetchRemaining();
    }
    else {
      if (rowPointer > -1) {
        beforeFirst();
        // after beforeFirst rowPointer is supposed to be -1, i'd say
        row->installCursorAtPosition(rowPointer > -1 ? rowPointer : 0);
        lastRowPointer= -1;
      }
      growDataArray(true);

      BinRow *br= dynamic_cast<BinRow*>(row);
      // Probably is better to make a copy
      MYSQL_BIND *bind= br->getDefaultBind();

      for (std::size_t i= 0; i < cache.size(); ++i)
      {
        cache[i].reset(new int8_t[bind[i].buffer_length * dataSize]);
        bind[i].buffer= cache[i].get();
      }
      std::size_t rowNr= 0;
      mysql_stmt_bind_result(capiStmtHandle, bind);
      while (br->fetchNext() != MYSQL_NO_DATA) {
        // It's mor correct to call cacheCurrentRow in Row object, but let's do to more optimally
        //br->cacheCurrentRow(data[rowNr], columnsInformation.size());
        auto& rowDataCache= data[rowNr];
        rowDataCache.clear();
        for (std::size_t i= 0; i < cache.size(); ++i)
        {
          auto& b= bind[i];
          if (b.is_null_value != '\0') {
              rowDataCache.emplace_back();
            }
            else {
            rowDataCache.emplace_back(static_cast<char*>(b.buffer), b.length && *b.length > 0 ? *b.length : b.buffer_length);
            }
          b.buffer= ((char*)bind[i].buffer + bind[i].buffer_length);
        }
        mysql_stmt_bind_result(capiStmtHandle, bind);
        ++rowNr;
      }
      rowPointer= preservedPosition;
    }
  }


  const char* ResultSetBin::getErrMessage()
  {
    return mysql_stmt_error(capiStmtHandle);
  }


  const char * ResultSetBin::getSqlState()
  {
    return mysql_stmt_error(capiStmtHandle);
  }


  uint32_t ResultSetBin::getErrNo()
  {
    return mysql_stmt_errno(capiStmtHandle);
  }


  uint32_t ResultSetBin::warningCount()
  {
    return mysql_stmt_warning_count(capiStmtHandle);
  }

  /**
    * When protocol has a current Streaming result (this) fetch all to permit another query is
    * executing. The lock should be acquired before calling this method
    *
    * @throws SQLException if any error occur
    */
  void ResultSetBin::fetchRemaining() {
    if (!isEof) {
      try {
        lastRowPointer= -1;
        if (!isEof && dataSize > 0 && fetchSize == 1) {
          // We need to grow the array till current size. Its main purpose is to create room for newly fetched
          // fetched row, so it grows till dataSize + 1. But we need to space for already fetched(from server)
          // row. Thus fooling growDataArray by decrementing dataSize
          --dataSize;
          growDataArray();
          // Since index of the last row is smaller from dataSize by 1, we have correct index
          row->cacheCurrentRow(data[dataSize], columnsInformation.size());
          rowPointer= 0;
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
        throw queryException;
      }
      catch (std::exception & ioe) {
        handleIoException(ioe);
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
  bool ResultSetBin::readNextValue(bool cacheLocally)
  {
    switch (row->fetchNext()) {
    case 1: {
      SQLString err("Internal error: most probably fetch on not yet executed statment handle. ");
      err.append(getErrMessage());
      throw SQLException(err, "HY000", getErrNo());
    }
    case MYSQL_DATA_TRUNCATED: {
      break;
    }

    case MYSQL_NO_DATA: {
      uint32_t serverStatus= protocol->getServerStatus();

      if (callableResult) {
        serverStatus|= SERVER_MORE_RESULTS_EXIST;
      }
      else {
        callableResult= (serverStatus & SERVER_PS_OUT_PARAMS) != 0;
      }

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
  std::vector<mariadb::bytes_view>& ResultSetBin::getCurrentRowData() {
    return data[rowPointer];
  }

  /**
    * Update row's raw bytes. in case of row update, refresh the data. (format must correspond to
    * current resultset binary/text row encryption)
    *
    * @param rawData new row's raw data.
    */
  void ResultSetBin::updateRowData(std::vector<mariadb::bytes_view>& rawData)
  {
    data[rowPointer]= rawData;
    row->resetRow(data[rowPointer]);
  }

  /**
    * Delete current data. Position cursor to the previous row->
    *
    * @throws SQLException if previous() fail.
    */
  void ResultSetBin::deleteCurrentRowData() {

    data.erase(data.begin()+lastRowPointer);
    dataSize--;
    lastRowPointer= -1;
    previous();
  }

  void ResultSetBin::addRowData(std::vector<mariadb::bytes_view>& rawData) {
    if (dataSize +1 >= data.size()) {
      growDataArray();
    }
    data[dataSize]= rawData;
    rowPointer= static_cast<int32_t>(dataSize);
    ++dataSize;
  }

  /*int32_t ResultSetBin::skipLengthEncodedValue(std::string& buf, int32_t pos) {
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
  void ResultSetBin::growDataArray(bool complete) {
    std::size_t curSize= data.size(), newSize= curSize + 1;
    if (complete) {
      newSize= dataSize;
    }

    if (data.capacity() < newSize) {
      std::size_t newCapacity= complete ? newSize : static_cast<std::size_t>(curSize + (curSize >> 1));

      // I don't remember what is MAX_ARRAY_SIZE is about. it might be irrelevant for C/ODBC and C/C++
      if (!complete && newCapacity > MAX_ARRAY_SIZE) {
        newCapacity= static_cast<std::size_t>(MAX_ARRAY_SIZE);
      }

      data.reserve(newCapacity);
    }
    for (std::size_t i=  curSize; i < newSize; ++i) {
      data.push_back({});
      data.back().reserve(columnsInformation.size());
    }
    //data[dataSize].reserve(columnsInformation.size());
  }

  /**
    * Connection.abort() has been called, abort result-set.
    *
    * @throws SQLException exception
    */
  void ResultSetBin::abort() {
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


  bool ResultSetBin::isBeforeFirst() const {
    checkClose();
    return (dataFetchTime >0) ? rowPointer == -1 && dataSize > 0 : rowPointer == -1;
  }


  bool ResultSetBin::isAfterLast() {
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

  bool ResultSetBin::isFirst() const {
    checkClose();
    return /*dataFetchTime == 1 && */rowPointer == 0 && dataSize > 0;
  }

  bool ResultSetBin::isLast() {
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

  void ResultSetBin::beforeFirst() {
    checkClose();

    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    rowPointer= -1;
  }

  void ResultSetBin::afterLast() {
    checkClose();
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize);
  }

  bool ResultSetBin::first() {
    checkClose();

    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }

    rowPointer= 0;
    return dataSize > 0;
  }

  bool ResultSetBin::last() {
    checkClose();
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize) - 1;
    return dataSize > 0;
  }

  /* Returns current 1-based position in the resulset */
  int64_t ResultSetBin::getRow() {
    checkClose();
    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      return 0;
    }
    return rowPointer + 1;
  }

  bool ResultSetBin::absolute(int64_t rowPos) {
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


  bool ResultSetBin::relative(int64_t rows) {
    checkClose();
    if (streaming && resultSetScrollType == TYPE_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type TYPE_FORWARD_ONLY");
    }
    int32_t newPos= static_cast<int32_t>(rowPointer + rows);
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

  bool ResultSetBin::previous() {
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


  int32_t ResultSetBin::getFetchSize() const {
    return this->fetchSize;
  }

  void ResultSetBin::setFetchSize(int32_t fetchSize) {
    if (streaming && fetchSize == 0) {
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

  int32_t ResultSetBin::getType()  const {
    return resultSetScrollType;
  }

  void ResultSetBin::checkClose() const {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
  }

  bool ResultSetBin::isCallableResult() const {
    return callableResult;
  }

  bool ResultSetBin::isClosed() const {
    return isClosedFlag;
  }

  /** {inheritDoc}. */
  bool ResultSetBin::wasNull() const {
    return row->wasNull();
  }

  /** {inheritDoc}. */
  SQLString ResultSetBin::getString(int32_t columnIndex) const
  {
    checkObjectRange(columnIndex);
    return std::move(row->getInternalString(&columnsInformation[columnIndex - 1]));
  }


  SQLString ResultSetBin::zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation)
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
  std::istream* ResultSetBin::getBinaryStream(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    blobBuffer[columnIndex].reset(new memBuf(const_cast<char*>(row->fieldBuf.arr) + row->pos, const_cast<char*>(row->fieldBuf.arr) + row->pos + row->getLengthMaxFieldSize()));
    return new std::istream(blobBuffer[columnIndex].get());
  }

  /** {inheritDoc}. */
  int32_t ResultSetBin::getInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalInt(&columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  int64_t ResultSetBin::getLong(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalLong(&columnsInformation[columnIndex -1]);
  }


  uint64_t ResultSetBin::getUInt64(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return static_cast<uint64_t>(row->getInternalULong(&columnsInformation[columnIndex -1]));
  }


  uint32_t ResultSetBin::getUInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);

    const ColumnDefinition* columnInfo= &columnsInformation[columnIndex - 1];
    int64_t value= row->getInternalLong(columnInfo);

    row->rangeCheck("uint32_t", 0, UINT32_MAX, value, columnInfo);

    return static_cast<uint32_t>(value);
  }

  /** {inheritDoc}. */
  float ResultSetBin::getFloat(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalFloat(&columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  long double ResultSetBin::getDouble(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalDouble(&columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  ResultSetMetaData* ResultSetBin::getMetaData() const {
    return new ResultSetMetaData(columnsInformation, forceAlias);
  }

  ///** {inheritDoc}. */
  //Blob* ResultSetBin::getBlob(int32_t columnIndex) const {
  //  return getBinaryStream(columnIndex);
  //}

  /** {inheritDoc}. */
  bool ResultSetBin::getBoolean(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalBoolean(&columnsInformation[static_cast<std::size_t>(index) -1]);
  }

  /** {inheritDoc}. */
  int8_t ResultSetBin::getByte(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalByte(&columnsInformation[static_cast<std::size_t>(index) - 1]);
  }

  /** {inheritDoc}. */
  short ResultSetBin::getShort(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalShort(&columnsInformation[static_cast<std::size_t>(index) - 1]);
  }

 
  std::size_t ResultSetBin::rowsCount() const
  {
    return dataSize;
  }

  /** Force metadata getTableName to return table alias, not original table name. */
  void ResultSetBin::setForceTableAlias() {
    this->forceAlias= true;
  }

  void ResultSetBin::rangeCheck(const SQLString& className, int64_t minValue, int64_t maxValue, int64_t value, ColumnDefinition* columnInfo) {
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

  int32_t ResultSetBin::getRowPointer() {
    return rowPointer;
  }

  void ResultSetBin::setRowPointer(int32_t pointer) {
    rowPointer= pointer;
  }


  std::size_t ResultSetBin::getDataSize() {
    return dataSize;
  }


  bool ResultSetBin::isBinaryEncoded() {
    return row->isBinaryEncoded();
  }


  void ResultSetBin::realClose(bool noLock)
  {
    isClosedFlag= true;
    if (!isEof) {
      try {
        while (!isEof) {
          dataSize= 0; // to avoid storing data
          readNextValue();
        }
      }
      catch (SQLException & queryException) {
        resetVariables();
        throw queryException;
      }
    }

    checkOut();
    resetVariables();

    data.clear();

    if (statement != nullptr) {
      statement= nullptr;
    }
  }

  /*{{{ ResultSetBin::bind
   * Binding MYSQL_BIND structures to columns. Should be called *after* setting callbacks.
   * (this constraint can be removed if really needed)
   */
  void ResultSetBin::bind(MYSQL_BIND* bind)
  {
    //mysql_stmt_bind_result(capiStmtHandle, bind);
    resultBind.reset(new MYSQL_BIND[columnInformationLength]());
    std::memcpy(resultBind.get(), bind, columnInformationLength*sizeof(MYSQL_BIND));
    if (!resultCodec.empty()) {
      for (const auto& it : resultCodec) {
        resultBind[it.first].flags|= MADB_BIND_DUMMY;
      }
    }
    if (dataSize > 0 || streaming) {
      mysql_stmt_bind_result(capiStmtHandle, resultBind.get());
      reBound= true; //We need to force fetch. Otherwise fetch_column may fail
    }
  }


  bool ResultSetBin::get(MYSQL_BIND* bind, uint32_t colIdx0based, uint64_t offset)
  {
    checkObjectRange(colIdx0based + 1);
    // If cached result - write to buffers with own means, otherwise let c/c do it
    if (data.size()) {
      return getCached(bind, colIdx0based, offset);
    }
    else {
      return mysql_stmt_fetch_column(capiStmtHandle, bind, colIdx0based, static_cast<unsigned long>(offset)) != 0;
    }
  }

  /* Method like C/C's fetch functions - moves to next record and fills bound buffers with values */
  bool ResultSetBin::get()
  {
    bool truncations= false;
    if (resultBind) {
      // Ugly - we don't want resetRow to call mysql_stmt_fetch. Maybe it just never should,
      // but it is like it is, and now is not the right time to change that.
      if (lastRowPointer != rowPointer || (reBound && !streaming)) {
        resetRow();
        reBound= false;
      }
      if (resultCodec.empty()) {
        for (int32_t i= 0; i < columnInformationLength; ++i) {
          MYSQL_BIND* bind= resultBind.get() + i;
          if (bind->error == nullptr) {
            bind->error= &bind->error_value;
          }
          get(bind, i, 0);
          if (*bind->error) {
            truncations= true;
          }
        }
      }
      else {
        lastRowPointer= rowPointer;
        return false;
      }
    }
    return truncations;
  }


  bool ResultSetBin::setResultCallback(ResultCodec* callback, uint32_t column)
  {
    if (column == uint32_t(-1)) {
      if (mysql_stmt_attr_set(capiStmtHandle, STMT_ATTR_CB_USER_DATA, callback ? (void*)this : nullptr)) {
        return true;
      }
      nullResultCodec= callback;
      return mysql_stmt_attr_set(capiStmtHandle, STMT_ATTR_CB_RESULT, (const void*)defaultResultCallback);
    }
    if (column >= static_cast<uint32_t>(columnInformationLength)) {
      throw SQLException("No such column: " + std::to_string(column + 1), "22023");
    }

    resultCodec[column]= callback;
    if (resultCodec.size() == 1 && nullResultCodec == nullptr) {
      mysql_stmt_attr_set(capiStmtHandle, STMT_ATTR_CB_USER_DATA, (void*)this);
      return mysql_stmt_attr_set(capiStmtHandle, STMT_ATTR_CB_RESULT, (const void*)defaultResultCallback);
    }
    return false;
  }


  void defaultResultCallback(void* data, uint32_t column, unsigned char **row)
  {
    // We can't let the callback to throw - we have to intercept, as otherwise we are guaranteed to have
    // the protocol broken
    try
    {
      // Assuming so far, that Null value codec is always present. In this project it's the case.
      ResultSetBin *rs= reinterpret_cast<ResultSetBin*>(data);
      if (row == nullptr) {
        (*rs->nullResultCodec)(rs->callbackData, column, nullptr, NULL_LENGTH);
      }

      const auto& it= rs->resultCodec.find(column);

      if (it != rs->resultCodec.end()) {
        auto length= mysql_net_field_length(row);
        (*it->second)(rs->callbackData, column, *row, length);
        *row+= length;
      }
      //else if ()
    }
    catch (...)
    {
      // Just eating exception
    }
  }


  bool ResultSetBin::setCallbackData(void * data)
  {
    callbackData= data;
    // if C/C does not support callbacks 
    return mysql_stmt_attr_set(capiStmtHandle, STMT_ATTR_CB_USER_DATA, (void*)this);
  }

} // namespace mariadb
