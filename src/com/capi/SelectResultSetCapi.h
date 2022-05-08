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


#ifndef _SELECTRESULTSETCAPI_H_
#define _SELECTRESULTSETCAPI_H_

#include <exception>
#include <vector>

// Should go before Consts
#include "com/capi/ColumnDefinitionCapi.h"

#include "Consts.h"

#include "SelectResultSet.h"

#include "MariaDbStatement.h"

#include "ResultSet.hpp"
#include "ColumnType.h"
#include "com/ColumnNameMap.h"
#include "io/StandardPacketInputStream.h"

#include "jdbccompat.hpp"


namespace sql
{
namespace mariadb
{
class TimeZone;
class ServerPrepareResult;
struct memBuf;

namespace capi
{
#include "mysql.h"

class SelectResultSetCapi : public SelectResultSet
{

  TimeZone* timeZone;
  Shared::Options options;
  std::vector<Shared::ColumnDefinition> columnsInformation;
  int32_t columnInformationLength;
  bool noBackslashEscapes;
  std::map<int32_t, std::unique_ptr<memBuf>> blobBuffer;

  Protocol* protocol;
  bool isEof;
  bool callableResult;
  /* Shared? */
  MariaDbStatement* statement;
  Unique::RowProtocol row;

  MYSQL *capiConnHandle;
  MYSQL_STMT *capiStmtHandle;

  int32_t dataFetchTime;
  bool streaming;

  /*std::unique_ptr<*/
  std::vector<std::vector<sql::bytes>> data;
  std::size_t dataSize; //Should go after data

  int32_t fetchSize;
  int32_t resultSetScrollType;
  int32_t rowPointer;

  std::unique_ptr<ColumnNameMap> columnNameMap;

  int32_t lastRowPointer; /*-1*/
  bool isClosedFlag;
  bool eofDeprecated;
  Shared::mutex lock;
  bool forceAlias;

public:

  SelectResultSetCapi(
    Results* results,
    Protocol* protocol,
    ServerPrepareResult* pr,
    bool callableResult,
    bool eofDeprecated);

  SelectResultSetCapi(
    Results* results,
    Protocol* protocol,
    MYSQL* connection,
    bool eofDeprecated);

  SelectResultSetCapi(
    std::vector<Shared::ColumnDefinition>& columnInformation,
    /*std::unique_ptr<*/std::vector<std::vector<sql::bytes>>& resultSet,
    Protocol* protocol,
    int32_t resultSetScrollType);

  bool isFullyLoaded() const;

private:
  void fetchAllResults();

  const char* getErrMessage();
  const char* getSqlState();
  uint32_t getErrNo();
  uint32_t warningCount();
public:
  void fetchRemaining();

private:
  void handleIoException(std::exception& ioe);
  void nextStreamingValue();
  void addStreamingValue();
  bool readNextValue();

protected:
  std::vector<sql::bytes>& getCurrentRowData();
  void updateRowData(std::vector<sql::bytes>& rawData);
  void deleteCurrentRowData();
  void addRowData(std::vector<sql::bytes>& rawData);

private:
  void growDataArray();

public:
  void abort();
  void close();

private:
  void resetVariables();
public:
  bool fetchNext();
  bool next();
private:
  void resetRow();
  void checkObjectRange(int32_t position);
public:
  SQLWarning* getWarnings();
  void clearWarnings();
  bool isBeforeFirst();
  bool isAfterLast();
  bool isFirst();
  bool isLast();
  void beforeFirst();
  void afterLast();
  bool first();
  bool last();
  int32_t getRow();
  bool absolute(int32_t row);
  bool relative(int32_t rows);
  bool previous();
  int32_t getFetchDirection();
  void setFetchDirection(int32_t direction);
  int32_t getFetchSize();
  void setFetchSize(int32_t fetchSize);
  int32_t getType();
  int32_t getConcurrency();
private:
  void checkClose();
public:
  bool isCallableResult();
  bool isClosed() const;
  MariaDbStatement* getStatement();
  void setStatement(MariaDbStatement* statement);
  bool wasNull();

  bool isNull(int32_t columnIndex);
  bool isNull(const SQLString& columnLabel);
  SQLString getString(int32_t columnIndex);
  SQLString getString(const SQLString& columnLabel);
private:
  SQLString zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation);
public:
  std::istream* getBinaryStream(int32_t columnIndex);
  std::istream* getBinaryStream(const SQLString& columnLabel);
  int32_t getInt(int32_t columnIndex);
  int32_t getInt(const SQLString& columnLabel);
  int64_t getLong(const SQLString& columnLabel);
  int64_t getLong(int32_t columnIndex);
  int64_t getInt64(const SQLString& columnLabel)  { return getLong(columnLabel); }
  int64_t getInt64(int32_t columnIndex)           { return getLong(columnIndex); }
  uint64_t getUInt64(const SQLString& columnLabel);
  uint64_t getUInt64(int32_t columnIndex);
  uint32_t getUInt(const SQLString& columnLabel);
  uint32_t getUInt(int32_t columnIndex);
  float getFloat(const SQLString& columnLabel);
  float getFloat(int32_t columnIndex);
  long double getDouble(const SQLString& columnLabel);
  long double getDouble(int32_t columnIndex);
  bool getBoolean(int32_t index);
  bool getBoolean(const SQLString& columnLabel);
  int8_t getByte(int32_t index);
  int8_t getByte(const SQLString& columnLabel);
  int16_t getShort(int32_t index);
  int16_t getShort(const SQLString& columnLabel);

  int32_t findColumn(const SQLString& columnLabel);
  SQLString getCursorName();
  int32_t getHoldability();
  sql::ResultSetMetaData* getMetaData();
  Blob* getBlob(int32_t columnIndex);
  Blob* getBlob(const SQLString& columnLabel);

#ifdef MAYBE_IN_NEXTVERSION

  /* Still... maybe SQLString is better handler for this */
  sql::bytes* getBytes(const SQLString& columnLabel);
  sql::bytes* getBytes(int32_t columnIndex);
  Timestamp* getTimestamp(const SQLString& columnLabel);
  Timestamp* getTimestamp(int32_t columnIndex);
  Time* getTime(int32_t columnIndex);
  Time* getTime(const SQLString& columnLabel);
  std::istream* getAsciiStream(const SQLString& columnLabel);
  std::istream* getAsciiStream(int32_t columnIndex);
  SQLString getNString(int32_t columnIndex);
  SQLString getNString(const SQLString& columnLabel);
#endif

  RowId* getRowId(int32_t columnIndex);
  RowId* getRowId(const SQLString& columnLabel);

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  BigDecimal getBigDecimal(const SQLString& columnLabel,int32_t scale);
  BigDecimal getBigDecimal(int32_t columnIndex,int32_t scale);
  BigDecimal getBigDecimal(int32_t columnIndex);
  BigDecimal getBigDecimal(const SQLString& columnLabel);
  Date* getDate(int32_t columnIndex, Calendar& cal);
  Date* getDate(const SQLString& columnLabel, Calendar& cal);
  Time* getTime(int32_t columnIndex, Calendar& cal);
  Time* getTime(const SQLString& columnLabel, Calendar& cal);
  Timestamp* getTimestamp(int32_t columnIndex, Calendar& cal);
  Timestamp* getTimestamp(const SQLString& columnLabel, Calendar& cal);

  sql::Object* getObject(int32_t columnIndex);
  sql::Object* getObject(const SQLString& columnLabel);
  std::istringstream* getCharacterStream(const SQLString& columnLabel);
  std::istringstream* getCharacterStream(int32_t columnIndex);
  std::istringstream* getNCharacterStream(int32_t columnIndex);
  std::istringstream* getNCharacterStream(const SQLString& columnLabel);
  Ref* getRef(int32_t columnIndex);
  Ref* getRef(const SQLString& columnLabel);
  Clob* getClob(int32_t columnIndex);
  Clob* getClob(const SQLString& columnLabel);
  sql::Array* getArray(int32_t columnIndex);
  sql::Array* getArray(const SQLString& columnLabel);
  URL* getURL(int32_t columnIndex);
  URL* getURL(const SQLString& columnLabel);

  NClob* getNClob(int32_t columnIndex);
  NClob* getNClob(const SQLString& columnLabel);
  SQLXML* getSQLXML(int32_t columnIndex);
  SQLXML* getSQLXML(const SQLString& columnLabel);
#endif

  bool rowUpdated();
  bool rowInserted();
  bool rowDeleted();
  void insertRow();
  void deleteRow();
  void refreshRow();
  void moveToInsertRow();
  void moveToCurrentRow();
  void cancelRowUpdates();

  std::size_t rowsCount();

#ifdef RS_UPDATE_FUNCTIONALITY_IMPLEMENTED
  void updateNull(int32_t columnIndex);
  void updateNull(const SQLString& columnLabel);
  void updateBoolean(int32_t columnIndex, bool _bool);
  void updateBoolean(const SQLString& columnLabel,bool value);
  void updateByte(int32_t columnIndex,char value);
  void updateByte(const SQLString& columnLabel,char value);
  void updateShort(int32_t columnIndex,short value);
  void updateShort(const SQLString& columnLabel,short value);
  void updateInt(int32_t columnIndex,int32_t value);
  void updateInt(const SQLString& columnLabel,int32_t value);
  void updateFloat(int32_t columnIndex,float value);
  void updateFloat(const SQLString& columnLabel,float value);
  void updateDouble(int32_t columnIndex,double value);
  void updateDouble(const SQLString& columnLabel,double value);
  void updateBigDecimal(int32_t columnIndex,BigDecimal value);
  void updateBigDecimal(const SQLString& columnLabel,BigDecimal value);
  void updateString(int32_t columnIndex, const SQLString& value);
  void updateString(const SQLString& columnLabel, const SQLString& value);
  void updateBytes(int32_t columnIndex,std::string& value);
  void updateBytes(const SQLString& columnLabel,std::string& value);
  void updateDate(int32_t columnIndex,Date date);
  void updateDate(const SQLString& columnLabel,Date value);
  void updateTime(int32_t columnIndex,Time time);
  void updateTime(const SQLString& columnLabel,Time value);
  void updateTimestamp(int32_t columnIndex,Timestamp timeStamp);
  void updateTimestamp(const SQLString& columnLabel,Timestamp value);
  void updateAsciiStream(int32_t columnIndex,std::istream* inputStream,int32_t length);
  void updateAsciiStream(const SQLString& columnLabel,std::istream* inputStream);
  void updateAsciiStream(const SQLString& columnLabel,std::istream* value,int32_t length);
  void updateAsciiStream(int32_t columnIndex,std::istream* inputStream,int64_t length);
  void updateAsciiStream(const SQLString& columnLabel,std::istream* inputStream,int64_t length);
  void updateAsciiStream(int32_t columnIndex,std::istream* inputStream);
  void updateBinaryStream(int32_t columnIndex,std::istream* inputStream,int32_t length);
  void updateBinaryStream(int32_t columnIndex,std::istream* inputStream,int64_t length);
  void updateBinaryStream(const SQLString& columnLabel,std::istream* value,int32_t length);
  void updateBinaryStream(const SQLString& columnLabel,std::istream* inputStream,int64_t length);
  void updateBinaryStream(int32_t columnIndex,std::istream* inputStream);
  void updateBinaryStream(const SQLString& columnLabel,std::istream* inputStream);
  void updateCharacterStream(int32_t columnIndex,std::istringstream& value,int32_t length);
  void updateCharacterStream(int32_t columnIndex,std::istringstream& value);
  void updateCharacterStream(const SQLString& columnLabel,std::istringstream& reader,int32_t length);
  void updateCharacterStream(int32_t columnIndex,std::istringstream& value,int64_t length);
  void updateCharacterStream(const SQLString& columnLabel,std::istringstream& reader,int64_t length);
  void updateCharacterStream(const SQLString& columnLabel,std::istringstream& reader);

  void updateObject(int32_t columnIndex,sql::Object* value,int32_t scaleOrLength);
  void updateObject(int32_t columnIndex,sql::Object* value);
  void updateObject(const SQLString& columnLabel,sql::Object* value,int32_t scaleOrLength);
  void updateObject(const SQLString& columnLabel,sql::Object* value);
  void updateLong(const SQLString& columnLabel,int64_t value);
  void updateLong(int32_t columnIndex,int64_t value);
  void updateRow();
  void updateRef(int32_t columnIndex, Ref& ref);
  void updateRef(const SQLString& columnLabel, Ref& ref);
  void updateBlob(int32_t columnIndex,Blob& blob);
  void updateBlob(const SQLString& columnLabel,Blob& blob);
  void updateBlob(int32_t columnIndex,std::istream* inputStream);
  void updateBlob(const SQLString& columnLabel,std::istream* inputStream);
  void updateBlob(int32_t columnIndex,std::istream* inputStream,int64_t length);
  void updateBlob(const SQLString& columnLabel,std::istream* inputStream,int64_t length);
  void updateClob(int32_t columnIndex,Clob& clob);
  void updateClob(const SQLString& columnLabel,Clob& clob);
  void updateClob(int32_t columnIndex,std::istringstream& reader,int64_t length);
  void updateClob(const SQLString& columnLabel,std::istringstream& reader,int64_t length);
  void updateClob(int32_t columnIndex,std::istringstream& reader);
  void updateClob(const SQLString& columnLabel,std::istringstream& reader);
  void updateArray(int32_t columnIndex,sql::Array& array);
  void updateArray(const SQLString& columnLabel,sql::Array& array);
  void updateRowId(int32_t columnIndex, RowId& rowId);
  void updateRowId(const SQLString& columnLabel, RowId& rowId);
  void updateNString(int32_t columnIndex, const SQLString& nstring);
  void updateNString(const SQLString& columnLabel, const SQLString& nstring);
  void updateNClob(int32_t columnIndex,NClob& nclob);
  void updateNClob(const SQLString& columnLabel,NClob& nclob);
  void updateNClob(int32_t columnIndex,std::istringstream& reader);
  void updateNClob(const SQLString& columnLabel,std::istringstream& reader);
  void updateNClob(int32_t columnIndex,std::istringstream& reader,int64_t length);
  void updateNClob(const SQLString& columnLabel,std::istringstream& reader,int64_t length);
  void updateSQLXML(int32_t columnIndex,SQLXML& xmlObject);
  void updateSQLXML(const SQLString& columnLabel,SQLXML& xmlObject);
  void updateNCharacterStream(int32_t columnIndex,std::istringstream& value,int64_t length);
  void updateNCharacterStream(const SQLString& columnLabel,std::istringstream& reader,int64_t length);
  void updateNCharacterStream(int32_t columnIndex,std::istringstream& reader);
  void updateNCharacterStream(const SQLString& columnLabel,std::istringstream& reader);
#endif

      //public:  bool isWrapperFor();
  void setForceTableAlias();
private:
  void rangeCheck(const SQLString& className,int64_t minValue,int64_t maxValue,int64_t value, ColumnDefinition* columnInfo);
public:
  int32_t getRowPointer();
protected:
  void setRowPointer(int32_t pointer);
public:
  std::size_t getDataSize();
  bool isBinaryEncoded();
  };

}
}
}
#endif