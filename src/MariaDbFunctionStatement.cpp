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


#include <mutex>

#include "MariaDbFunctionStatement.h"
#include "ServerSidePreparedStatement.h"
#include "ClientSidePreparedStatement.h"
#include "CallableParameterMetaData.h"
#include "MariaDbConnection.h"
#include "CallParameter.h"
#include "Results.h"

namespace sql
{
namespace mariadb
{

  /**
    * Specific implementation of CallableStatement to handle function call, represent by call like
    * {?= call procedure-name[(arg1,arg2, ...)]}.
    *
    * @param connection current connection
    * @param databaseName database name
    * @param procedureName function name
    * @param arguments function args
    * @param resultSetType a result set type; one of <code>ResultSet.TYPE_FORWARD_ONLY</code>, <code>
    *     ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @throws SQLException exception
    */
  MariaDbFunctionStatement::MariaDbFunctionStatement(
    MariaDbConnection* _connection,
    const SQLString& _databaseName, const SQLString& _functionName, const SQLString& arguments,
    int32_t resultSetType,
    int32_t resultSetConcurrency,
    Shared::ExceptionFactory& exptnFactory)
    : connection(_connection),
      stmt(new ClientSidePreparedStatement(
      _connection,
      "SELECT "+ _functionName +(arguments.empty() ? "()" : arguments),
      resultSetType,
      resultSetConcurrency, Statement::NO_GENERATED_KEYS, exptnFactory)),
      parameterMetadata(nullptr),
      databaseName(_databaseName),
      functionName(_functionName)
  {
    initFunctionData(stmt->getParameterCount() + 1);
  }

  SelectResultSet* MariaDbFunctionStatement::getResult()
  {
    if (!outputResultSet) {
      throw SQLException("No output result");
    }
    return outputResultSet;
  }

  SelectResultSet * MariaDbFunctionStatement::getOutputResult()
  {
    if (!outputResultSet /*== nullptr*/) {
      if (stmt->getFetchSize() != 0) {
        Shared::Results& results= getResults();
        results->loadFully(false, connection->getProtocol().get());
        outputResultSet= results->getCallableResultSet();
        if (outputResultSet) {
          outputResultSet->next();
          return outputResultSet;
        }
      }
      throw SQLException("There is no output result");
    }
    return outputResultSet;
  }

  MariaDbFunctionStatement::MariaDbFunctionStatement(const MariaDbFunctionStatement &other, MariaDbConnection* _connection)
    : connection(_connection)
    , stmt(other.stmt->clone(_connection))
    , parameterMetadata(other.parameterMetadata)
    , outputResultSet(nullptr)
    , params(other.params)
  {
  }

  /**
    * Clone statement.
    *
    * @param connection connection
    * @return Clone statement.
    * @throws CloneNotSupportedException if any error occur.
    */
  MariaDbFunctionStatement* MariaDbFunctionStatement::clone(MariaDbConnection* connection)
  {
    MariaDbFunctionStatement* clone= new MariaDbFunctionStatement(*this, connection);

    return clone;
  }

  int32_t MariaDbFunctionStatement::executeUpdate()
  {
    std::lock_guard<std::mutex> localScopeLock(*connection->lock);
    auto& results= getResults();

    stmt->execute();

    retrieveOutputResult();

    if (results && results->getResultSet()) {
      return 0;
    }
    return getUpdateCount();
  }

  void MariaDbFunctionStatement::retrieveOutputResult()
  {
    auto& results= getResults();
    //TODO: release or get here?
    outputResultSet= results->getResultSet();
    if (outputResultSet) {
      outputResultSet->next();
    }
  }

  int32_t MariaDbFunctionStatement::indexToOutputIndex(uint32_t parameterIndex)
  {
    return parameterIndex;
  }

  int32_t MariaDbFunctionStatement::nameToOutputIndex(const SQLString & parameterName)
  {
    for (uint32_t i= 0; i < parameterMetadata->getParameterCount(); i++) {
      const SQLString& name= parameterMetadata->getParameterName(i+1);
      if (!name.empty() && equalsIgnoreCase(name, parameterName)) {
        return i;
      }
    }
    throw SQLException("there is no parameter with the name "+parameterName);
  }

  int32_t MariaDbFunctionStatement::nameToIndex(const SQLString & parameterName)
  {
    readMetadataFromDbIfRequired();

    for (uint32_t i= 1; i <= parameterMetadata->getParameterCount(); i++) {
      const SQLString& name= parameterMetadata->getParameterName(i);

      if (!name.empty() && equalsIgnoreCase(name, parameterName))
      {
        return i;
      }
    }
    throw SQLException("there is no parameter with the name "+parameterName);
  }


  CallParameter& MariaDbFunctionStatement::getParameter(uint32_t index)
  {
    if (index > params.size() || index <= 0) {
      throw SQLException("No parameter with index " + std::to_string(index));
    }
    return params[index -1];
  }

  Shared::Results & MariaDbFunctionStatement::getResults()
  {
    MariaDbStatement* mstmt= static_cast<MariaDbStatement*>(*stmt);
    return mstmt->getInternalResults();
  }

  /**
  * Data initialisation when parameterCount is defined.
  *
  * @param parametersCount number of parameters
  */
  void MariaDbFunctionStatement::initFunctionData(int32_t parametersCount)
  {
    params.reserve(parametersCount);
    for (int32_t i= 0; i < parametersCount; i++) {
      params[i]= CallParameter();
      if (i > 0) {
        params[i].setInput(true);
      }
    }
    // the query was in the form {?=call function()}, so the first parameter is always output
    params[0].setOutput(true);
  }


  void MariaDbFunctionStatement::setParameter(int32_t parameterIndex, ParameterHolder& holder)
  {
    stmt->setParameter(parameterIndex -1, &holder);
  }

  ResultSet* MariaDbFunctionStatement::executeQuery()
  {
    std::lock_guard<std::mutex> localScopeLock(*connection->getProtocol()->getLock());
    auto& results= getResults();

    stmt->execute();
    retrieveOutputResult();
    if (results && results->getResultSet()) {
      return results->releaseResultSet();
    }

    return SelectResultSet::createEmptyResultSet();
  }

  bool MariaDbFunctionStatement::execute()
  {
    std::unique_lock<std::mutex>  localScopeLock(*connection->getProtocol()->getLock());
    auto& results= getResults();
    localScopeLock.unlock();
    stmt->execute();
    retrieveOutputResult();
    return results && results->getResultSet();
  }


  Connection * MariaDbFunctionStatement::getConnection()
  {
    return connection;
  }


  /**
  * Registers the designated output parameter. This version of the method <code>
  * registerOutParameter</code> should be used for a user-defined or <code>REF</code> output
  * parameter. Examples of user-defined types include: <code>STRUCT</code>, <code>DISTINCT</code>,
  * <code>JAVA_OBJECT</code>, and named array types.
  *
  * <p>All OUT parameters must be registered before a stored procedure is executed.
  *
  * <p>For a user-defined parameter, the fully-qualified SQL type name of the parameter should also
  * be given, while a <code>REF</code> parameter requires that the fully-qualified type name of the
  * referenced type be given. A JDBC driver that does not need the type code and type name
  * information may ignore it. To be portable, however, applications should always provide these
  * values for user-defined and <code>REF</code> parameters.
  *
  * <p>Although it is intended for user-defined and <code>REF</code> parameters, this method may be
  * used to register a parameter of any JDBC type. If the parameter does not have a user-defined or
  * <code>REF</code> type, the <i>typeName</i> parameter is ignored.
  *
  * <p><B>Note:</B> When reading the value of an out parameter, you must use the getter method
  * whose Java type corresponds to the parameter's registered SQL type.
  *
  * @param parameterIndex the first parameter is 1, the second is 2,...
  * @param sqlType a value from {@link Types}
  * @param typeName the fully-qualified name of an SQL structured type
  * @throws SQLException if the parameterIndex is not valid; if a database access error occurs or
  *     this method is called on a closed <code>CallableStatement</code>
  * @see Types
  */
  void MariaDbFunctionStatement::registerOutParameter(int32_t parameterIndex, int32_t sqlType, const SQLString& typeName)
  {
    CallParameter& callParameter= getParameter(parameterIndex);

    callParameter.setOutputSqlType(sqlType);
    callParameter.setTypeName(typeName);
    callParameter.setOutput(true);
  }

  void MariaDbFunctionStatement::registerOutParameter(int32_t parameterIndex, int32_t sqlType)
  {
    registerOutParameter(parameterIndex, sqlType, -1);
  }

  void MariaDbFunctionStatement::registerOutParameter(int32_t parameterIndex, int32_t sqlType, int32_t scale)
  {
    CallParameter& callParameter= getParameter(parameterIndex);

    callParameter.setOutput(true);
    callParameter.setOutputSqlType(sqlType);
    callParameter.setScale(scale);
  }

  void MariaDbFunctionStatement::registerOutParameter(const SQLString& parameterName, int32_t sqlType)
  {
    registerOutParameter(nameToIndex(parameterName), sqlType);
  }

  void MariaDbFunctionStatement::registerOutParameter(const SQLString& parameterName, int32_t sqlType, int32_t scale)
  {
    registerOutParameter(nameToIndex(parameterName), sqlType, scale);
  }

  void MariaDbFunctionStatement::registerOutParameter(const SQLString& parameterName, int32_t sqlType, const SQLString& typeName)
  {
    registerOutParameter(nameToIndex(parameterName), sqlType, typeName);
  }


  void MariaDbFunctionStatement::readMetadataFromDbIfRequired() {
    if (!parameterMetadata) {
      parameterMetadata.reset(connection->getInternalParameterMetaData(functionName, databaseName, true));
    }
  }

  ParameterMetaData * MariaDbFunctionStatement::getParameterMetaData()
  {
    readMetadataFromDbIfRequired();
    return parameterMetadata.get();;
  }


  bool MariaDbFunctionStatement::wasNull()
  {
    return getOutputResult()->wasNull();
  }

  SQLString MariaDbFunctionStatement::getString(int32_t parameterIndex)
  {
    return getOutputResult()->getString(indexToOutputIndex(parameterIndex));
  }

  SQLString MariaDbFunctionStatement::getString(const SQLString& parameterName)
  {
    return getOutputResult()->getString(nameToOutputIndex(parameterName));
  }

  bool MariaDbFunctionStatement::getBoolean(int32_t parameterIndex)
  {
    return getOutputResult()->getBoolean(indexToOutputIndex(parameterIndex));
  }

  bool MariaDbFunctionStatement::getBoolean(const SQLString& parameterName)
  {
    return getOutputResult()->getBoolean(nameToOutputIndex(parameterName));
  }

  int8_t MariaDbFunctionStatement::getByte(int32_t parameterIndex)
  {
    return getOutputResult()->getByte(indexToOutputIndex(parameterIndex));
  }

  int8_t MariaDbFunctionStatement::getByte(const SQLString& parameterName)
  {
    return getOutputResult()->getByte(nameToOutputIndex(parameterName));
  }

  int16_t MariaDbFunctionStatement::getShort(int32_t parameterIndex)
  {
    return getOutputResult()->getShort(indexToOutputIndex(parameterIndex));
  }

  int16_t MariaDbFunctionStatement::getShort(const SQLString& parameterName)
  {
    return getOutputResult()->getShort(nameToOutputIndex(parameterName));
  }

  int32_t MariaDbFunctionStatement::getInt(const SQLString& parameterName)
  {
    return getOutputResult()->getInt(nameToOutputIndex(parameterName));
  }

  int32_t MariaDbFunctionStatement::getInt(int32_t parameterIndex)
  {
    return getOutputResult()->getInt(indexToOutputIndex(parameterIndex));
  }

  int64_t MariaDbFunctionStatement::getLong(const SQLString& parameterName)
  {
    return getOutputResult()->getLong(nameToOutputIndex(parameterName));
  }

  int64_t MariaDbFunctionStatement::getLong(int32_t parameterIndex)
  {
    return getOutputResult()->getLong(indexToOutputIndex(parameterIndex));
  }

  float MariaDbFunctionStatement::getFloat(const SQLString& parameterName)
  {
    return getOutputResult()->getFloat(nameToOutputIndex(parameterName));
  }

  float MariaDbFunctionStatement::getFloat(int32_t parameterIndex)
  {
    return getOutputResult()->getFloat(indexToOutputIndex(parameterIndex));
  }

  long double MariaDbFunctionStatement::getDouble(int32_t parameterIndex)
  {
    return getOutputResult()->getDouble(indexToOutputIndex(parameterIndex));
  }

  long double MariaDbFunctionStatement::getDouble(const SQLString& parameterName)
  {
    return getOutputResult()->getDouble(nameToOutputIndex(parameterName));
  }

#ifdef MAYBE_IN_NEXTVERSION
  sql::bytes* MariaDbFunctionStatement::getBytes(const SQLString& parameterName)
  {
    return getOutputResult()->getBytes(nameToOutputIndex(parameterName));
  }

  sql::bytes* MariaDbFunctionStatement::getBytes(int32_t parameterIndex)
  {
    return getOutputResult()->getBytes(indexToOutputIndex(parameterIndex));
  }
#endif

  /******************** Calls forwarding to CSPS *********************/
  uint32_t MariaDbFunctionStatement::getMaxFieldSize() {
    return stmt->getMaxFieldSize();
  }

  const sql::Ints& MariaDbFunctionStatement::executeBatch() {
    return stmt->executeBatch();
  }

  bool MariaDbFunctionStatement::execute(const sql::SQLString &sql, const sql::SQLString *colNames)
  {
    return stmt->execute(sql, colNames);
  }


  bool MariaDbFunctionStatement::execute(const sql::SQLString &sql, int32_t *colIdxs)
  {
    return stmt->execute(sql, colIdxs);
  }


  bool MariaDbFunctionStatement::execute(const SQLString& sql)
  {
    return stmt->execute(sql);
  }


  bool MariaDbFunctionStatement::execute(const SQLString& sql, int32_t autoGeneratedKeys)
  {
    return stmt->execute(sql, autoGeneratedKeys);
  }


  ResultSet* MariaDbFunctionStatement::executeQuery(const SQLString& sql)
  {
    return dynamic_cast<Statement*>(stmt.get())->executeQuery(sql);
  }


  int64_t MariaDbFunctionStatement::executeLargeUpdate(const SQLString& sql)
  {
    return stmt->executeLargeUpdate(sql);
  }


  int64_t MariaDbFunctionStatement::executeLargeUpdate(const SQLString& sql, int32_t autoGeneratedKeys)
  {
    return stmt->executeLargeUpdate(sql, autoGeneratedKeys);
  }


  int64_t MariaDbFunctionStatement::executeLargeUpdate(const SQLString& sql, int32_t* columnIndexes)
  {
    return stmt->executeLargeUpdate(sql, columnIndexes);
  }


  int64_t MariaDbFunctionStatement::executeLargeUpdate(const SQLString& sql, const SQLString* columnNames)
  {
    return stmt->executeLargeUpdate(sql, columnNames);
  }


  int32_t MariaDbFunctionStatement::executeUpdate(const SQLString& sql)
  {
    return stmt->executeUpdate(sql);
  }


  int32_t MariaDbFunctionStatement::executeUpdate(const SQLString& sql, int32_t autoGeneratedKeys)
  {
    return stmt->executeUpdate(sql, autoGeneratedKeys);
  }


  int32_t MariaDbFunctionStatement::executeUpdate(const SQLString& sql, int32_t* columnIndexes)
  {
    return stmt->executeUpdate(sql, columnIndexes);
  }


  int32_t MariaDbFunctionStatement::executeUpdate(const SQLString& sql, const SQLString* columnNames)
  {
    return stmt->executeUpdate(sql, columnNames);
  }


  void MariaDbFunctionStatement::setMaxFieldSize(uint32_t max)
  {
    stmt->setMaxFieldSize(max);
  }


  int32_t MariaDbFunctionStatement::getMaxRows()
  {
    return stmt->getMaxRows();
  }


  void MariaDbFunctionStatement::setMaxRows(int32_t max)
  {
    stmt->setMaxRows(max);
  }


  int64_t MariaDbFunctionStatement::getLargeMaxRows()
  {
    return stmt->getLargeMaxRows();
  }


  void MariaDbFunctionStatement::setLargeMaxRows(int64_t max)
  {
    return stmt->setLargeMaxRows(max);
  }



  void MariaDbFunctionStatement::setEscapeProcessing(bool enable)
  {
    stmt->setEscapeProcessing(enable);
  }



  int32_t MariaDbFunctionStatement::getQueryTimeout()
  {
    return stmt->getQueryTimeout();
  }



  void MariaDbFunctionStatement::setQueryTimeout(int32_t seconds)
  {
    stmt->setQueryTimeout(seconds);
  }



  void MariaDbFunctionStatement::cancel()
  {
    stmt->cancel();
  }



  SQLWarning* MariaDbFunctionStatement::getWarnings()
  {
    return stmt->getWarnings();
  }



  void MariaDbFunctionStatement::clearWarnings()
  {
    stmt->clearWarnings();
  }


  void MariaDbFunctionStatement::setCursorName(const SQLString& name)
  {
    stmt->setCursorName(name);
  }


  ResultSet* MariaDbFunctionStatement::getGeneratedKeys()
  {
    return stmt->getGeneratedKeys();
  }


  int32_t MariaDbFunctionStatement::getResultSetHoldability()
  {
    return stmt->getResultSetHoldability();
  }


  bool MariaDbFunctionStatement::isClosed()
  {
    return stmt->isClosed();
  }


  bool MariaDbFunctionStatement::isPoolable()
  {
    return stmt->isPoolable();
  }


  void MariaDbFunctionStatement::setPoolable(bool poolable)
  {
    stmt->setPoolable(poolable);
  }


  ResultSet* MariaDbFunctionStatement::getResultSet()
  {
    return stmt->getResultSet();
  }


  int32_t MariaDbFunctionStatement::getUpdateCount()
  {
    return stmt->getUpdateCount();
  }


  int64_t MariaDbFunctionStatement::getLargeUpdateCount()
  {
    return stmt->getLargeUpdateCount();
  }


  bool MariaDbFunctionStatement::getMoreResults()
  {
    return stmt->getMoreResults();
  }


  bool MariaDbFunctionStatement::getMoreResults(int32_t current)
  {
    return stmt->getMoreResults(current);
  }


  int32_t MariaDbFunctionStatement::getFetchDirection()
  {
    return stmt->getFetchDirection();
  }


  void MariaDbFunctionStatement::setFetchDirection(int32_t direction)
  {
    stmt->setFetchDirection(direction);
  }


  int32_t MariaDbFunctionStatement::getFetchSize()
  {
    return stmt->getFetchSize();
  }


  void MariaDbFunctionStatement::setFetchSize(int32_t rows)
  {
    stmt->setFetchSize(rows);
  }


  int32_t MariaDbFunctionStatement::getResultSetConcurrency()
  {
    return stmt->getResultSetConcurrency();
  }


  int32_t MariaDbFunctionStatement::getResultSetType()
  {
    return stmt->getResultSetType();
  }


  void MariaDbFunctionStatement::closeOnCompletion()
  {
    stmt->closeOnCompletion();
  }


  bool MariaDbFunctionStatement::isCloseOnCompletion()
  {
    return stmt->isCloseOnCompletion();
  }

  void MariaDbFunctionStatement::addBatch()
  {
    stmt->addBatch();
  }

  void MariaDbFunctionStatement::addBatch(const SQLString& sql)
  {
    stmt->addBatch(sql);
  }


  void MariaDbFunctionStatement::clearBatch()
  {
    stmt->clearBatch();
  }


  void MariaDbFunctionStatement::close()
  {
    stmt->close();
  }


  const sql::Longs& MariaDbFunctionStatement::executeLargeBatch() {
    return stmt->executeLargeBatch();
  }


  int64_t MariaDbFunctionStatement::executeLargeUpdate() {
    return stmt->executeLargeUpdate();
  }

  void MariaDbFunctionStatement::setNull(int32_t parameterIndex, int32_t sqlType) {
    stmt->setNull(parameterIndex, sqlType);
  }
  /*void MariaDbFunctionStatement::setNull(int32_t parameterIndex, const ColumnType& mariadbType) {
  stmt->setNull(parameterIndex, mariadbType);
  }*/
  void MariaDbFunctionStatement::setNull(int32_t parameterIndex, int32_t sqlType, const SQLString& typeName) {
    stmt->setNull(parameterIndex, sqlType, typeName);
  }


  void MariaDbFunctionStatement::setBlob(int32_t parameterIndex, std::istream* inputStream, const int64_t length) {
    stmt->setBlob(parameterIndex, inputStream, length);
  }


  void MariaDbFunctionStatement::setBlob(int32_t parameterIndex, std::istream* inputStream) {
    stmt->setBlob(parameterIndex, inputStream);
  }


  void MariaDbFunctionStatement::setBoolean(int32_t parameterIndex, bool value) {
    stmt->setBoolean(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setByte(int32_t parameterIndex, int8_t byte) {
    stmt->setByte(parameterIndex, byte);
  }


  void MariaDbFunctionStatement::setShort(int32_t parameterIndex, int16_t value) {
    stmt->setShort(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setString(int32_t parameterIndex, const SQLString& str) {
    stmt->setString(parameterIndex, str);
  }


  void MariaDbFunctionStatement::setBytes(int32_t parameterIndex, sql::bytes* bytes) {
    stmt->setBytes(parameterIndex, bytes);
  }


  void MariaDbFunctionStatement::setInt(int32_t column, int32_t value) {
    stmt->setInt(column, value);
  }


  void MariaDbFunctionStatement::setLong(int32_t parameterIndex, int64_t value) {
    stmt->setLong(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setUInt64(int32_t parameterIndex, uint64_t value) {
    stmt->setUInt64(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setUInt(int32_t parameterIndex, uint32_t value) {
    stmt->setUInt(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setFloat(int32_t parameterIndex, float value) {
    stmt->setFloat(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setDouble(int32_t parameterIndex, double value) {
    stmt->setDouble(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setDateTime(int32_t parameterIndex, const SQLString & dt) {
    stmt->setDateTime(parameterIndex, dt);
  }


  void MariaDbFunctionStatement::setBigInt(int32_t parameterIndex, const SQLString& value) {
    stmt->setBigInt(parameterIndex, value);
  }


  void MariaDbFunctionStatement::setNull(const SQLString& parameterName, int32_t sqlType) {
    stmt->setNull(nameToIndex(parameterName), sqlType);
  }

  void MariaDbFunctionStatement::setNull(const SQLString& parameterName, int32_t sqlType, const SQLString& typeName)
  {
    stmt->setNull(nameToIndex(parameterName), sqlType, typeName);
  }

  void MariaDbFunctionStatement::setBoolean(const SQLString& parameterName, bool boolValue)
  {
    stmt->setBoolean(nameToIndex(parameterName), boolValue);
  }

  void MariaDbFunctionStatement::setByte(const SQLString& parameterName, char byteValue)
  {
    stmt->setByte(nameToIndex(parameterName), byteValue);
  }

  void MariaDbFunctionStatement::setShort(const SQLString& parameterName, int16_t shortValue)
  {
    stmt->setShort(nameToIndex(parameterName), shortValue);
  }

  void MariaDbFunctionStatement::setInt(const SQLString& parameterName, int32_t intValue)
  {
    stmt->setInt(nameToIndex(parameterName), intValue);
  }

  void MariaDbFunctionStatement::setLong(const SQLString& parameterName, int64_t longValue)
  {
    stmt->setLong(nameToIndex(parameterName), longValue);
  }

  void MariaDbFunctionStatement::setFloat(const SQLString& parameterName, float floatValue) {
    stmt->setFloat(nameToIndex(parameterName), floatValue);
  }


  void MariaDbFunctionStatement::setDouble(const SQLString& parameterName, double doubleValue) {
    stmt->setDouble(nameToIndex(parameterName), doubleValue);
  }


  void MariaDbFunctionStatement::setString(const SQLString& parameterName, const SQLString& stringValue) {
    stmt->setString(nameToIndex(parameterName), stringValue);
  }


  void MariaDbFunctionStatement::setBytes(const SQLString& parameterName, sql::bytes* bytes) {
    stmt->setBytes(nameToIndex(parameterName), bytes);
  }


  Statement* MariaDbFunctionStatement::setResultSetType(int32_t rsType) {
    stmt->setResultSetType(rsType);
    return this;
  }

  void MariaDbFunctionStatement::clearParameters() {
    stmt->clearParameters();
  }
}
}
