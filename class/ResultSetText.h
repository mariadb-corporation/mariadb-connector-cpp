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


#ifndef _SELECTRESULTSETCAPI_H_
#define _SELECTRESULTSETCAPI_H_

#include <exception>
#include <vector>
#include <map>

#include "ColumnDefinition.h"
#include "CArray.h"
#include "ResultSet.h"

#include "mysql.h"


namespace mariadb
{
class PreparedStatement;
struct memBuf;

class ResultSetText : public ResultSet
{
  MYSQL*      capiConnHandle= nullptr;
  MYSQL_BIND* resultBind=     nullptr;

public:
  ResultSetText(
    Results* results,
    Protocol* _protocol,
    MYSQL* connection);

  ResultSetText(
    std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    Protocol * _protocol,
    int32_t resultSetScrollType);

  ResultSetText(
    const MYSQL_FIELD *columnInformation,
    std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    Protocol * _protocol,
    int32_t resultSetScrollType);
  
  ~ResultSetText();

  bool isFullyLoaded() const override;

private:
  void flushPendingServerResults();
  void cacheCompleteLocally() override;

  const char* getErrMessage();
  const char* getSqlState();
  uint32_t getErrNo();
  uint32_t warningCount();

public:
  void fetchRemaining() override;

private:
  bool readNextValue(bool cacheLocally= false) override;

protected:
  std::vector<mariadb::bytes_view>& getCurrentRowData() override;
  void updateRowData(std::vector<mariadb::bytes_view>& rawData) override;
  void deleteCurrentRowData() override;
  void addRowData(std::vector<mariadb::bytes_view>& rawData) override;

private:
  void growDataArray();

public:
  void abort() override;
  void close();

public:
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
  void setFetchSize(int32_t fetchSize);
  int32_t getType() const;

private:
  void checkClose() const;
public:
  bool isCallableResult() const override;
  bool isClosed() const;
  bool wasNull() const;

  SQLString getString(int32_t columnIndex) const;

private:
  SQLString zeroFillingIfNeeded(const SQLString& value, const ColumnDefinition* columnInformation);

public:
  std::istream* getBinaryStream(int32_t columnIndex) const override;
  int32_t getInt(int32_t columnIndex) const override;
  int64_t getLong(int32_t columnIndex) const override;
  uint64_t getUInt64(int32_t columnIndex) const override;
  uint32_t getUInt(int32_t columnIndex) const override;
  float   getFloat(int32_t columnIndex) const override;
  bool    getBoolean(int32_t index) const override;
  int8_t  getByte(int32_t index) const override;
  int16_t getShort(int32_t index) const override;
  long double getDouble(int32_t columnIndex) const override;

  ResultSetMetaData* getMetaData() const override;
  PreparedStatement* getStatement();
   /*Blob* getBlob(int32_t columnIndex) const;*/

  std::size_t rowsCount() const override;

  void setForceTableAlias() override;

private:
  void rangeCheck(const SQLString& className,int64_t minValue,int64_t maxValue,int64_t value,
    const ColumnDefinition* columnInfo);
  void setRowPointer(int32_t pointer) override;
  void checkOut();

public:
  int32_t     getRowPointer() override;
  std::size_t getDataSize() override;
  bool isBinaryEncoded() override;
  void realClose(bool noLock= true);

  void bind(MYSQL_BIND* bind) override;
  bool get(MYSQL_BIND* bind, uint32_t column0basedIdx, uint64_t offset) override;
  bool get() override;
  bool setResultCallback(ResultCodec* callback, uint32_t column) override { return true; }
  bool setCallbackData(void* data) override { return true; }
};

} // namespace mariadb
#endif
