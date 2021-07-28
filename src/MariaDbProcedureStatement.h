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


#ifndef _MARIADBPROCEDURESTATEMENT_H_
#define _MARIADBPROCEDURESTATEMENT_H_

#include "Consts.h"

#include "ParameterMetaData.hpp"
#include "CallParameter.h"

namespace sql
{
namespace mariadb
{
class SelectResultSet;
class ParameterHolder;
class MariaDbConnection;

namespace capi
{
  class SelectResultSet;
}
class MariaDbProcedureStatement  : public CloneableCallableStatement
{
  SelectResultSet* outputResultSet;
  std::vector<CallParameter> params;
  std::vector<int32_t> outputParameterMapper;
  MariaDbConnection* connection;
  Shared::CallableParameterMetaData parameterMetadata;
  bool hasInOutParameters;
  Unique::ServerSidePreparedStatement stmt;
  SQLString database;
  SQLString procedureName;

public:
   MariaDbProcedureStatement(
    const SQLString& query,
    MariaDbConnection* connection, const SQLString& procedureName, const SQLString& database,
    int32_t resultSetType,
    int32_t resultSetConcurrency,
    Shared::ExceptionFactory& factory);
   MariaDbProcedureStatement(
     MariaDbConnection* connection, const SQLString& sql, int32_t resultSetScrollType, int32_t resultSetConcurrency, Shared::ExceptionFactory& factory);
   MariaDbProcedureStatement(MariaDbConnection* conn);
   // Just to have where to put a breakpoint
   ~MariaDbProcedureStatement() {}

private:
  void setParamsAccordingToSetArguments();
  void setInputOutputParameterMap();

protected:
  SelectResultSet* getOutputResult();

public:
  MariaDbProcedureStatement* clone(MariaDbConnection* connection);

private:
  void retrieveOutputResult();

public:
  void setParameter(int32_t parameterIndex, ParameterHolder& holder);
  bool execute();

private:
  void validAllParameters();
  int32_t nameToIndex(const SQLString& parameterName);
  int32_t nameToOutputIndex(const SQLString& parameterName);
  int32_t indexToOutputIndex(uint32_t parameterIndex);
  CallParameter& getParameter(uint32_t index);
  Shared::Results& getResults();
  void readMetadataFromDbIfRequired();

public:
  const sql::Ints& executeBatch();
  const sql::Longs& executeLargeBatch();
  void setParametersVariables();
  ParameterMetaData* getParameterMetaData();

  bool wasNull();
  SQLString getString(int32_t parameterIndex);
  SQLString getString(const SQLString& parameterName);
  bool getBoolean(int32_t parameterIndex);
  bool getBoolean(const SQLString& parameterName);
  int8_t getByte(int32_t parameterIndex);
  int8_t getByte(const SQLString& parameterName);
  int16_t getShort(int32_t parameterIndex);
  int16_t getShort(const SQLString& parameterName);
  int32_t getInt(const SQLString& parameterName);
  int32_t getInt(int32_t parameterIndex);
  int64_t getLong(const SQLString& parameterName);
  int64_t getLong(int32_t parameterIndex);
  float getFloat(const SQLString& parameterName);
  float getFloat(int32_t parameterIndex);
  long double getDouble(int32_t parameterIndex);
  long double getDouble(const SQLString& parameterName);
  Blob* getBlob(int32_t parameterIndex);
  Blob* getBlob(const SQLString& parameterName);
  sql::bytes* getBytes(const SQLString& parameterName);
  sql::bytes* getBytes(int32_t parameterIndex);
  Date getDate(int32_t parameterIndex);
  Date getDate(const SQLString& parameterName);

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  BigDecimal* getBigDecimal(int32_t parameterIndex, int32_t scale);
  BigDecimal* getBigDecimal(int32_t parameterIndex);
  BigDecimal* getBigDecimal(const SQLString& parameterName);
  Date* getDate(const SQLString& parameterName, Calendar& cal);
  Date* getDate(int32_t parameterIndex, Calendar& cal);
  Time* getTime(int32_t parameterIndex, Calendar& cal);
  Time* getTime(const SQLString& parameterName, Calendar& cal);
  Timestamp* getTimestamp(int32_t parameterIndex, Calendar& cal);
  Timestamp* getTimestamp(const SQLString& parameterName, Calendar& cal);

  Time* getTime(const SQLString& parameterName);
  Time getTime(int32_t parameterIndex);
  Timestamp* getTimestamp(int32_t parameterIndex);
  Timestamp* getTimestamp(const SQLString& parameterName);

  /*sql::Object* getObject(int32_t parameterIndex);
  sql::Object* getObject(const SQLString& parameterName);
  template <class T >T getObject(int32_t parameterIndex,Classtemplate <class T >type);
  template <class T >T getObject(const SQLString& parameterName,Classtemplate <class T >type);*/
  Ref* getRef(int32_t parameterIndex);
  Ref* getRef(const SQLString& parameterName);
  Clob* getClob(const SQLString& parameterName);
  Clob* getClob(int32_t parameterIndex);
  sql::Array* getArray(const SQLString& parameterName);
  sql::Array* getArray(int32_t parameterIndex);
  URL* getURL(int32_t parameterIndex);
  URL* getURL(const SQLString& parameterName);
  RowId* getRowId(int32_t parameterIndex);
  RowId* getRowId(const SQLString& parameterName);
  NClob* getNClob(int32_t parameterIndex);
  NClob* getNClob(const SQLString& parameterName);
  SQLXML* getSQLXML(int32_t parameterIndex);
  SQLXML* getSQLXML(const SQLString& parameterName);
  std::istringstream* getNCharacterStream(int32_t parameterIndex);
  std::istringstream* getNCharacterStream(const SQLString& parameterName);
#endif
#ifdef MAYBE_IN_NEXTVERSION
  SQLString getNString(int32_t parameterIndex);
  SQLString getNString(const SQLString& parameterName);
#endif
  std::istringstream* getCharacterStream(int32_t parameterIndex);
  std::istringstream* getCharacterStream(const SQLString& parameterName);
  void registerOutParameter(int32_t parameterIndex, int32_t sqlType, const SQLString& typeName);
  void registerOutParameter(int32_t parameterIndex, int32_t sqlType);
  void registerOutParameter(int32_t parameterIndex, int32_t sqlType, int32_t scale);
  void registerOutParameter(const SQLString& parameterName, int32_t sqlType);
  void registerOutParameter(const SQLString& parameterName, int32_t sqlType, int32_t scale);
  void registerOutParameter(const SQLString& parameterName, int32_t sqlType, const SQLString& typeName);

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  void registerOutParameter(int32_t parameterIndex, SQLType* sqlType);
  void registerOutParameter(int32_t parameterIndex, SQLType* sqlType, int32_t scale);
  void registerOutParameter(int32_t parameterIndex, SQLType* sqlType, const SQLString& typeName);
  void registerOutParameter(const SQLString& parameterName, SQLType* sqlType);
  void registerOutParameter(const SQLString& parameterName, SQLType* sqlType, int32_t scale);
  void registerOutParameter(const SQLString& parameterName, SQLType* sqlType, const SQLString& typeName);
  void setSQLXML(const SQLString& parameterName, const SQLXML& xmlObject);
  void setRowId(const SQLString& parameterName, const RowId& rowid);
  void setNCharacterStream(const SQLString& parameterName, std::istringstream& value, int64_t length);
  void setNCharacterStream(const SQLString& parameterName, std::istringstream& value);
  void setNClob(const SQLString& parameterName, const NClob& value);
  void setClob(const SQLString& parameterName, const Clob& clob);
  void setNClob(const SQLString& parameterName, const std::istringstream& reader, int64_t length);
  void setNClob(const SQLString& parameterName, const std::istringstream& reader);
  void setBlob(const SQLString& parameterName, const Blob& blob);
#endif

  void setClob(const SQLString& parameterName, const std::istringstream* reader, int64_t length);
  void setClob(const SQLString& parameterName, const std::istringstream* reader);
  void setBlob(const SQLString& parameterName, const std::istream* inputStream, int64_t length);
  void setBlob(const SQLString& parameterName, const std::istream* inputStream);

#ifdef MAYBE_IN_NEXTVERSION
  void setNString(const SQLString& parameterName, const SQLString& value);
  void setAsciiStream(const SQLString& parameterName, std::istream& inputStream, int64_t length);
  void setAsciiStream(const SQLString& parameterName, std::istream& inputStream, int32_t length);
  void setAsciiStream(const SQLString& parameterName, std::istream& inputStream);
#endif
  void setBinaryStream(const SQLString& parameterName, const std::istream& inputStream, int64_t length);
  void setBinaryStream(const SQLString& parameterName, const std::istream& inputStream);
  void setBinaryStream(const SQLString& parameterName, const std::istream& inputStream, int32_t length);
  void setCharacterStream(const SQLString& parameterName, const std::istringstream& reader, int64_t length);
  void setCharacterStream(const SQLString& parameterName, const std::istringstream& reader);
  void setCharacterStream(const SQLString& parameterName, const std::istringstream& reader, int32_t length);

  void setNull(const SQLString& parameterName, int32_t sqlType);
  void setNull(const SQLString& parameterName, int32_t sqlType, const SQLString& typeName);
  void setBoolean(const SQLString& parameterName, bool boolValue);
  void setByte(const SQLString& parameterName, char byteValue);
  void setShort(const SQLString& parameterName, int16_t shortValue);
  void setInt(const SQLString& parameterName, int32_t intValue);
  void setLong(const SQLString& parameterName, int64_t longValue);
  void setInt64(const SQLString& parameterName, int64_t longValue) { setLong(parameterName, longValue); }
  void setFloat(const SQLString& parameterName, float floatValue);
  void setDouble(const SQLString& parameterName, double doubleValue);
  void setBigDecimal(const SQLString& parameterName, const BigDecimal& bigDecimal);
  void setString(const SQLString& parameterName, const SQLString& stringValue);
  void setBytes(const SQLString& parameterName, sql::bytes* bytes);

  void setNull(int32_t parameterIndex, int32_t sqlType);
  //void setNull(int32_t parameterIndex, const ColumnType& mariadbType);
  void setNull(int32_t parameterIndex, int32_t sqlType, const SQLString& typeName);
  void setBlob(int32_t parameterIndex, std::istream* inputStream, const int64_t length);
  void setBlob(int32_t parameterIndex, std::istream* inputStream);

  void setBoolean(int32_t parameterIndex, bool value);
  void setByte(int32_t parameterIndex, int8_t byte);
  void setShort(int32_t parameterIndex, int16_t value);
  void setString(int32_t parameterIndex, const SQLString& str);
  void setBytes(int32_t parameterIndex, sql::bytes* bytes);
  void setInt(int32_t column, int32_t value);
  void setLong(int32_t parameterIndex, int64_t value);
  void setInt64(int32_t parameterIndex, int64_t value) { setLong(parameterIndex, value); }
  void setUInt64(int32_t parameterIdx, uint64_t longValue);
  void setUInt(int32_t parameterIndex, uint32_t value);
  void setFloat(int32_t parameterIndex, float value);
  void setDouble(int32_t parameterIndex, double value);
  void setDateTime(int32_t parameterIndex, const SQLString& dt);
  void setBigInt(int32_t parameterIndex, const SQLString& value);

  /* Forwarding to stmt to implement Statement's part of interface */
  bool execute(const sql::SQLString& sql, const sql::SQLString* colNames);
  bool execute(const sql::SQLString& sql, int32_t* colIdxs);
  bool execute(const SQLString& sql);
  bool execute(const SQLString& sql, int32_t autoGeneratedKeys);
  int32_t executeUpdate(const SQLString& sql);
  int32_t executeUpdate(const SQLString& sql, int32_t autoGeneratedKeys);
  int32_t executeUpdate(const SQLString& sql, int32_t* columnIndexes);
  int32_t executeUpdate(const SQLString& sql, const SQLString* columnNames);
  int64_t executeLargeUpdate(const SQLString& sql);
  int64_t executeLargeUpdate(const SQLString& sql, int32_t autoGeneratedKeys);
  int64_t executeLargeUpdate(const SQLString& sql, int32_t* columnIndexes);
  int64_t executeLargeUpdate(const SQLString& sql, const SQLString* columnNames);
  ResultSet* executeQuery();
  ResultSet* executeQuery(const SQLString& sql);

  uint32_t getMaxFieldSize();
  void setMaxFieldSize(uint32_t max);
  int32_t getMaxRows();
  void setMaxRows(int32_t max);
  int64_t getLargeMaxRows();
  void setLargeMaxRows(int64_t max);
  void setEscapeProcessing(bool enable);
  int32_t getQueryTimeout();
  void setQueryTimeout(int32_t seconds);
  void cancel();
  SQLWarning* getWarnings();
  void clearWarnings();
  void setCursorName(const SQLString& name);
  Connection* getConnection();
  ResultSet* getGeneratedKeys();
  int32_t getResultSetHoldability();
  bool isClosed();
  bool isPoolable();
  void setPoolable(bool poolable);
  ResultSet* getResultSet();
  int32_t getUpdateCount();
  int64_t getLargeUpdateCount();
  bool getMoreResults();
  bool getMoreResults(int32_t current);
  int32_t getFetchDirection();
  void setFetchDirection(int32_t direction);
  int32_t getFetchSize();
  void setFetchSize(int32_t rows);
  int32_t getResultSetConcurrency();
  int32_t getResultSetType();
  void closeOnCompletion();
  bool isCloseOnCompletion();
  void addBatch();
  void addBatch(const SQLString& sql);
  void clearBatch();
  void close();
  int32_t executeUpdate();
  int64_t executeLargeUpdate();
  Statement* setResultSetType(int32_t rsType);
  void clearParameters();

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  void setURL(const SQLString& parameterName, URL& url);
  void setDate(const SQLString& parameterName, Date& date);
  void setTime(const SQLString& parameterName, Time& time);
  void setTimestamp(const SQLString& parameterName, Timestamp& timestamp);

  void setDate(const SQLString& parameterName, Date& date, Calendar& cal);
  void setTime(const SQLString& parameterName, Time& time, Calendar& cal);
  void setTimestamp(const SQLString& parameterName, Timestamp& timestamp, Calendar& cal);
  void setObject(const SQLString& parameterName,sql::Object* obj,int32_t targetSqlType,int32_t scale);
  void setObject(const SQLString& parameterName,sql::Object* obj,int32_t targetSqlType);
  void setObject(const SQLString& parameterName,sql::Object* obj);
  void setObject(const SQLString& parameterName,sql::Object* obj,SQLType* targetSqlType,int32_t scaleOrLength);
  void setObject(const SQLString& parameterName,sql::Object* obj,SQLType* targetSqlType);
#endif

  };
}
}
#endif
