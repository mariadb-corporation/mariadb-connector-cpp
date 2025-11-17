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


#ifndef _RESULTSETCBIN_H_
#define _RESULTSETCBIN_H_

#include <exception>
#include <vector>
#include <map>

#include "ColumnDefinition.h"
#include "ResultSet.h"


namespace mariadb
{
class ServerPrepareResult;
struct memBuf;

extern "C"
{
  // C/C's callback running param column callbacks 
  my_bool* defaultParamCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
  // C/C's callback running callback for the row, and then indivivual param column callbacks
  my_bool* withRowCheckCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
  // Result callback
  void defaultResultCallback(void* data, uint32_t column, unsigned char **row);
}

class ResultSetBin : public ResultSet
{
  friend void defaultResultCallback(void* data, uint32_t column, unsigned char **row);

  bool callableResult= false;
  MYSQL_STMT* capiStmtHandle;
  std::unique_ptr<MYSQL_BIND[]> resultBind;
  std::vector<std::unique_ptr<int8_t[]>> cache;

  std::map<std::size_t, ResultCodec*> resultCodec;
  // For NULL value C/C may call back even for those columns which we haven't marked as dummy. Thus atm we either always need a codec for each column
  // or have one special codec. That makes sense since we don't won't to transcode all possible type combinations.
  ResultCodec* nullResultCodec= nullptr;
  void* callbackData= nullptr;
  bool reBound= false;

public:

  ResultSetBin(
    Results* results,
    Protocol* guard,
    ServerPrepareResult* pr,
    bool callable= false);

  ~ResultSetBin();

  bool isFullyLoaded() const override;
  void fetchRemaining() override;

private:
  void flushPendingServerResults();
  void cacheCompleteLocally() override;

  const char* getErrMessage();
  const char* getSqlState();
  uint32_t getErrNo();
  uint32_t warningCount();

private:
  bool readNextValue(bool cacheLocally= false) override;

protected:
  std::vector<mariadb::bytes_view>& getCurrentRowData() override;

  void updateRowData(std::vector<mariadb::bytes_view>& rawData) override;
  void deleteCurrentRowData() override;
  void addRowData(std::vector<mariadb::bytes_view>& rawData) override;

private:
  void growDataArray(bool complete= false);

public:
  void abort() override;
  //void close();
  //bool fetchNext();
  //SQLWarning* getWarnings();
  //void clearWarnings();
  bool isBeforeFirst() const;
  bool isAfterLast() override;
  bool isFirst() const;
  bool isLast() override;
  void beforeFirst() override;
  void afterLast() override;
  bool first() override;
  bool last() override;
  int64_t getRow() override;
  bool absolute(int64_t row) override;
  bool relative(int64_t rows) override;
  bool previous() override;
  int32_t getFetchSize() const;
  void    setFetchSize(int32_t fetchSize);
  int32_t getType() const;
//  int32_t getConcurrency() const;

private:
  void checkClose() const;

public:
  bool isCallableResult() const override;
  bool isClosed() const;
  bool wasNull() const;

  SQLString getString(int32_t columnIndex) const;

private:
  SQLString zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation);

public:
  std::istream* getBinaryStream(int32_t columnIndex) const override;
  int32_t       getInt(int32_t columnIndex) const override;
  int64_t       getLong(int32_t columnIndex) const override;
  uint64_t      getUInt64(int32_t columnIndex) const override;
  uint32_t      getUInt(int32_t columnIndex) const override;
  float         getFloat(int32_t columnIndex) const override;
  long double   getDouble(int32_t columnIndex) const override;
  bool          getBoolean(int32_t index) const override;
  int8_t        getByte(int32_t index) const override;
  int16_t       getShort(int32_t index) const override;

  ResultSetMetaData* getMetaData() const override;
  /*Blob* getBlob(int32_t columnIndex) const;*/

  std::size_t rowsCount() const override;

  void setForceTableAlias() override;

private:
  void rangeCheck(const SQLString& className,int64_t minValue,int64_t maxValue,int64_t value,
    ColumnDefinition* columnInfo);

protected:
  void setRowPointer(int32_t pointer) override;

public:
  int32_t     getRowPointer() override;
  std::size_t getDataSize() override;
  bool isBinaryEncoded() override;
  void realClose(bool noLock= true);
  void bind(MYSQL_BIND* bind) override;
  bool get(MYSQL_BIND* bind, uint32_t colIdx0based, uint64_t offset) override;
  bool get() override;
  bool setResultCallback(ResultCodec* callback, uint32_t column) override;
  bool setCallbackData(void* data) override;
};

}
#endif
