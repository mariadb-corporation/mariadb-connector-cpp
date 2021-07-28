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


#include "MariaDbProcedureStatement.h"

#include "SelectResultSet.h"
#include "parameters/ParameterHolder.h"

#include "ServerSidePreparedStatement.h"
#include "CallableParameterMetaData.h"
#include "Results.h"
#include "Parameters.h"

namespace sql
{
namespace mariadb
{
  /**
    * Specific implementation of CallableStatement to handle function call, represent by call like
    * {?= call procedure-name[(arg1,arg2, ...)]}.
    *
    * @param query query
    * @param connection current connection
    * @param procedureName procedure name
    * @param database database
    * @param resultSetType a result set type; one of <code>ResultSet.TYPE_FORWARD_ONLY</code>, <code>
    *     ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @throws SQLException exception
    */
  MariaDbProcedureStatement::MariaDbProcedureStatement(
    const SQLString& query,
    MariaDbConnection* _connection, const SQLString& _procedureName, const SQLString& _database,
    int32_t resultSetType,
    int32_t resultSetConcurrency,
    Shared::ExceptionFactory& factory)
    : outputResultSet(nullptr),
      connection(_connection),
      parameterMetadata(nullptr),
      stmt(new ServerSidePreparedStatement(_connection, query, resultSetType, resultSetConcurrency, Statement::NO_GENERATED_KEYS, factory)),
      hasInOutParameters(false),
      database(_database),
      procedureName(_procedureName)
  {
    //super(connection, query, resultSetType, resultSetConcurrency);
    setParamsAccordingToSetArguments();
    setParametersVariables();
  }

  /**
  * Constructor for getter/setter of callableStatement.
  *
  * @param connection current connection
  * @param sql query
  * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
  *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
  *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
  * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
  *     or <code>ResultSet.CONCUR_UPDATABLE</code>
  * @throws SQLException is prepareStatement connection throw any error
  */
  MariaDbProcedureStatement::MariaDbProcedureStatement(
    MariaDbConnection* _connection, const SQLString& sql, int32_t resultSetScrollType, int32_t resultSetConcurrency, Shared::ExceptionFactory& factory)
    : connection(_connection),
      stmt(new ServerSidePreparedStatement(_connection, sql, resultSetScrollType, resultSetConcurrency, Statement::NO_GENERATED_KEYS, factory))
  {
  }

  MariaDbProcedureStatement::MariaDbProcedureStatement(MariaDbConnection * conn)
    : connection(conn)
  {
  }

  void MariaDbProcedureStatement::setParamsAccordingToSetArguments()
  {
    int32_t parameterCount= stmt->getParameterCount();

    params.reserve(parameterCount);
    for (int32_t index= 0; index < parameterCount; index++) {
      params.emplace_back();
    }
  }

  void MariaDbProcedureStatement::setInputOutputParameterMap()
  {
    if (outputParameterMapper.empty()) {
      outputParameterMapper.reserve(params.size());
      int32_t currentOutputMapper= 1;

      for (size_t index= 0; index < params.size(); index++) {
        outputParameterMapper[index]= (params[index].isOutput() ? currentOutputMapper++ : -1);
      }
    }
  }

  SelectResultSet* MariaDbProcedureStatement::getOutputResult()
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

  /**
    * Clone statement.
    *
    * @param connection connection
    * @return Clone statement.
    * @throws CloneNotSupportedException if any error occur.
    */
  MariaDbProcedureStatement* MariaDbProcedureStatement::clone(MariaDbConnection* connection)
  {
    MariaDbProcedureStatement* clone= new MariaDbProcedureStatement(connection);
    clone->outputResultSet= nullptr;
    clone->stmt.reset(stmt->clone(connection));

    clone->params= params;
    clone->parameterMetadata= parameterMetadata;
    clone->hasInOutParameters= hasInOutParameters;
    clone->outputParameterMapper= outputParameterMapper;

    return clone;
  }

  void MariaDbProcedureStatement::retrieveOutputResult()
  {
    Shared::Results& results= getResults();

    outputResultSet= results->getCallableResultSet();
    if (outputResultSet != nullptr) {
      outputResultSet->next();
    }
  }

  void MariaDbProcedureStatement::setParameter(int32_t parameterIndex, ParameterHolder& holder)
  {
    params[parameterIndex - 1].setInput(true);
    stmt->setParameter(parameterIndex, &holder);
  }

  bool MariaDbProcedureStatement::execute()
  {
    Shared::Results& results= getResults();

    validAllParameters();
    stmt->executeInternal(stmt->getFetchSize());
    retrieveOutputResult();

    return results && results->getResultSet();
  }

  /**
    * Valid that all parameters are set.
    *
    * @throws SQLException if set parameters is not right
    */
  void MariaDbProcedureStatement::validAllParameters()
  {
    setInputOutputParameterMap();

    for (uint32_t index= 0; index < params.size(); index++) {
      if (!params[index].isInput()) {
        stmt->setParameter(index +1, new NullParameter()); //TODO: check if memory not leaked. looks very much like that
      }
    }
    stmt->validParameters();
  }

  const sql::Ints& MariaDbProcedureStatement::executeBatch()
  {
    if (!hasInOutParameters) {
      return stmt->executeBatch();
    }
    else {
      throw SQLException("executeBatch not permit for procedure with output parameter");
    }
  }

  /********************** Former CallableProcedureStatement methods *******************************/

  /** Set in/out parameters value. */
  void MariaDbProcedureStatement::setParametersVariables()
  {
    hasInOutParameters= false;

    for (CallParameter& param : params) {
      if (param.isOutput() && param.isInput()) {
        hasInOutParameters= true;
        break;
      }
    }
  }

  void MariaDbProcedureStatement::readMetadataFromDbIfRequired() {
    if (!parameterMetadata) {
      parameterMetadata.reset(connection->getInternalParameterMetaData(procedureName, database, false));
    }
  }

  ParameterMetaData* MariaDbProcedureStatement::getParameterMetaData()
  {
    readMetadataFromDbIfRequired();
    return parameterMetadata.get();
  }

  /**
  * Convert parameter name to parameter index in the query.
  *
  * @param parameterName name
  * @return index
  * @throws SQLException exception
  */
  int32_t MariaDbProcedureStatement::nameToIndex(const SQLString& parameterName)
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

  /**
  * Convert parameter name to output parameter index in the query.
  *
  * @param parameterName name
  * @return index
  * @throws SQLException exception
  */
  int32_t MariaDbProcedureStatement::nameToOutputIndex(const SQLString& parameterName)
  {
    readMetadataFromDbIfRequired();
    for (uint32_t i= 0; i <parameterMetadata->getParameterCount(); i++) {
      const SQLString& name= parameterMetadata->getParameterName(i + 1);
      if (!name.empty() && equalsIgnoreCase(name, parameterName)) {
        if (outputParameterMapper[i] ==-1) {

          throw SQLException(
            "Parameter '"
            + parameterName
            + "' is not declared as output parameter with method registerOutParameter");
        }
        return outputParameterMapper[i];
      }
    }
    throw SQLException("there is no parameter with the name "+parameterName);
  }

  /**
  * Convert parameter index to corresponding outputIndex.
  *
  * @param parameterIndex index
  * @return index
  * @throws SQLException exception
  */
  int32_t MariaDbProcedureStatement::indexToOutputIndex(uint32_t parameterIndex)
  {
    try {
      if (outputParameterMapper[parameterIndex -1] ==-1) {

        throw SQLException(
          "Parameter in index '"
          + std::to_string(parameterIndex)
          +"' is not declared as output parameter with method registerOutParameter");
      }
      return outputParameterMapper[parameterIndex -1];
    }
    catch (std::exception& /*arrayIndexOutOfBoundsException*/) {
      if (parameterIndex <1) {
        throw SQLException("Index "+ std::to_string(parameterIndex) +" must at minimum be 1");
      }
      throw SQLException(
        "Index value '"+ std::to_string(parameterIndex) +"' is incorrect. Maximum value is " + std::to_string(params.size()));
    }
  }

  bool MariaDbProcedureStatement::wasNull()
  {
    return getOutputResult()->wasNull();
  }

  SQLString MariaDbProcedureStatement::getString(int32_t parameterIndex)
  {
    return getOutputResult()->getString(indexToOutputIndex(parameterIndex));
  }

  SQLString MariaDbProcedureStatement::getString(const SQLString& parameterName)
  {
    return getOutputResult()->getString(nameToOutputIndex(parameterName));
  }

  bool MariaDbProcedureStatement::getBoolean(int32_t parameterIndex)
  {
    return getOutputResult()->getBoolean(indexToOutputIndex(parameterIndex));
  }

  bool MariaDbProcedureStatement::getBoolean(const SQLString& parameterName)
  {
    return getOutputResult()->getBoolean(nameToOutputIndex(parameterName));
  }

  int8_t MariaDbProcedureStatement::getByte(int32_t parameterIndex)
  {
    return getOutputResult()->getByte(indexToOutputIndex(parameterIndex));
  }

  int8_t MariaDbProcedureStatement::getByte(const SQLString& parameterName)
  {
    return getOutputResult()->getByte(nameToOutputIndex(parameterName));
  }

  int16_t MariaDbProcedureStatement::getShort(int32_t parameterIndex)
  {
    return getOutputResult()->getShort(indexToOutputIndex(parameterIndex));
  }

  int16_t MariaDbProcedureStatement::getShort(const SQLString& parameterName)
  {
    return getOutputResult()->getShort(nameToOutputIndex(parameterName));
  }

  int32_t MariaDbProcedureStatement::getInt(const SQLString& parameterName)
  {
    return getOutputResult()->getInt(nameToOutputIndex(parameterName));
  }

  int32_t MariaDbProcedureStatement::getInt(int32_t parameterIndex)
  {
    return getOutputResult()->getInt(indexToOutputIndex(parameterIndex));
  }

  int64_t MariaDbProcedureStatement::getLong(const SQLString& parameterName)
  {
    return getOutputResult()->getLong(nameToOutputIndex(parameterName));
  }

  int64_t MariaDbProcedureStatement::getLong(int32_t parameterIndex)
  {
    return getOutputResult()->getLong(indexToOutputIndex(parameterIndex));
  }

  float MariaDbProcedureStatement::getFloat(const SQLString& parameterName)
  {
    return getOutputResult()->getFloat(nameToOutputIndex(parameterName));
  }

  float MariaDbProcedureStatement::getFloat(int32_t parameterIndex)
  {
    return getOutputResult()->getFloat(indexToOutputIndex(parameterIndex));
  }

  long double MariaDbProcedureStatement::getDouble(int32_t parameterIndex)
  {
    return getOutputResult()->getDouble(indexToOutputIndex(parameterIndex));
  }

  long double MariaDbProcedureStatement::getDouble(const SQLString& parameterName)
  {
    return getOutputResult()->getDouble(nameToOutputIndex(parameterName));
  }

#ifdef MAYBE_IN_NEXTVERSION
  sql::bytes* MariaDbProcedureStatement::getBytes(const SQLString& parameterName)
  {
    return getOutputResult()->getBytes(nameToOutputIndex(parameterName));
  }

  sql::bytes* MariaDbProcedureStatement::getBytes(int32_t parameterIndex)
  {
    return getOutputResult()->getBytes(indexToOutputIndex(parameterIndex));
  }
#endif

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  BigDecimal* MariaDbProcedureStatement::getBigDecimal(int32_t parameterIndex, int32_t scale)
  {
    return getOutputResult()->getBigDecimal(indexToOutputIndex(parameterIndex));
  }

  BigDecimal* MariaDbProcedureStatement::getBigDecimal(int32_t parameterIndex)
  {
    return getOutputResult()->getBigDecimal(indexToOutputIndex(parameterIndex));
  }

  BigDecimal* MariaDbProcedureStatement::getBigDecimal(const SQLString& parameterName)
  {
    return getOutputResult()->getBigDecimal(nameToOutputIndex(parameterName));
  }
  Date MariaDbProcedureStatement::getDate(const SQLString& parameterName, Calendar* cal)
  {
    return getOutputResult()->getDate(nameToOutputIndex(parameterName), cal);
  }

  Date* MariaDbProcedureStatement::getDate(int32_t parameterIndex, Calendar* cal)
  {
    return getOutputResult()->getDate(parameterIndex, cal);
  }

  Time* MariaDbProcedureStatement::getTime(int32_t parameterIndex, Calendar* cal)
  {
    return getOutputResult()->getTime(indexToOutputIndex(parameterIndex), cal);
  }

  void MariaDbProcedureStatement::setDate(const SQLString& parameterName, const Date& date, Calendar* cal)
  {
    setDate(nameToIndex(parameterName), date, cal);
  }

  void MariaDbProcedureStatement::setTime(const SQLString& parameterName, const Time time, Calendar* cal)
  {
    setTime(nameToIndex(parameterName), time, cal);
  }
  Time MariaDbProcedureStatement::getTime(const SQLString& parameterName, Calendar* cal)
  {
    return getOutputResult()->getTime(nameToOutputIndex(parameterName), cal);
  }
  void MariaDbProcedureStatement::setTimestamp(const SQLString& parameterName, const Timestamp& timestamp, Calendar* cal)
  {
    setTimestamp(nameToIndex(parameterName), timestamp, cal);
  }

  Timestamp* MariaDbProcedureStatement::getTimestamp(int32_t parameterIndex, Calendar* cal)
  {
    return getOutputResult()->getTimestamp(indexToOutputIndex(parameterIndex), cal);
  }

  Timestamp* MariaDbProcedureStatement::getTimestamp(const SQLString& parameterName, Calendar* cal)
  {
    return getOutputResult()->getTimestamp(nameToOutputIndex(parameterName), cal);
  }


  sql::Object* MariaDbProcedureStatement::getObject(int32_t parameterIndex)
  {
    std::type_info classType =
      typeid(ColumnType)FromJavaType(getParameter(parameterIndex).getOutputSqlType());
    if (classType/*.empty() != true*/) {
      return getOutputResult()->getObject(indexToOutputIndex(parameterIndex), classType);
    }
    return getOutputResult()->getObject(indexToOutputIndex(parameterIndex));
  }

  sql::Object* MariaDbProcedureStatement::getObject(const SQLString& parameterName)
  {
    int32_t index= nameToIndex(parameterName);
    Class< ? >classType= typeid(ColumnType)FromJavaType(getParameter(index).getOutputSqlType());
    if (classType/*.empty() != true*/) {
      return getOutputResult()->getObject(indexToOutputIndex(index), classType);
    }
    return getOutputResult()->getObject(indexToOutputIndex(index));
  }

  template <class T>T getObject(int32_t parameterIndex, Classtemplate <class T>type)
  {
    return getOutputResult()->getObject(indexToOutputIndex(parameterIndex), type);
  }

  template <class T>T getObject(const SQLString& parameterName, Classtemplate <class T>type)
  {
    return getOutputResult()->getObject(nameToOutputIndex(parameterName), type);
  }

  Ref* MariaDbProcedureStatement::getRef(int32_t parameterIndex)
  {
    return getOutputResult()->getRef(indexToOutputIndex(parameterIndex));
  }

  Ref* MariaDbProcedureStatement::getRef(const SQLString& parameterName)
  {
    return getOutputResult()->getRef(nameToOutputIndex(parameterName));
  }

  sql::Array* MariaDbProcedureStatement::getArray(const SQLString& parameterName)
  {
    return getOutputResult()->getArray(nameToOutputIndex(parameterName));
  }

  sql::Array* MariaDbProcedureStatement::getArray(int32_t parameterIndex)
  {
    return getOutputResult()->getArray(indexToOutputIndex(parameterIndex));
  }

  URL* MariaDbProcedureStatement::getURL(int32_t parameterIndex)
  {
    return getOutputResult()->getURL(indexToOutputIndex(parameterIndex));
  }

  URL* MariaDbProcedureStatement::getURL(const SQLString& parameterName)
  {
    return getOutputResult()->getURL(nameToOutputIndex(parameterName));
  }
#endif

#ifdef MAYBE_IN_NEXTVERSION
  Blob* MariaDbProcedureStatement::getBlob(int32_t parameterIndex)
  {
    return getOutputResult()->getBlob(parameterIndex);
  }

  Blob* MariaDbProcedureStatement::getBlob(const SQLString& parameterName)
  {
    return getOutputResult()->getBlob(nameToOutputIndex(parameterName));
  }



  Date* MariaDbProcedureStatement::getDate(int32_t parameterIndex)
  {
    return getOutputResult()->getDate(indexToOutputIndex(parameterIndex));
  }

  Date* MariaDbProcedureStatement::getDate(const SQLString& parameterName)
  {
    return getOutputResult()->getDate(nameToOutputIndex(parameterName));
  }

  Clob* MariaDbProcedureStatement::getClob(const SQLString& parameterName)
  {
    return getOutputResult()->getClob(nameToOutputIndex(parameterName));
  }

  Clob* MariaDbProcedureStatement::getClob(int32_t parameterIndex)
  {
    return getOutputResult()->getClob(indexToOutputIndex(parameterIndex));
  }

  NClob* MariaDbProcedureStatement::getNClob(int32_t parameterIndex)
  {
    return getOutputResult()->getNClob(indexToOutputIndex(parameterIndex));
  }

  NClob* MariaDbProcedureStatement::getNClob(const SQLString& parameterName)
  {
    return getOutputResult()->getNClob(nameToOutputIndex(parameterName));
  }

  Time* MariaDbProcedureStatement::getTime(const SQLString& parameterName)
  {
    return getOutputResult()->getTime(nameToOutputIndex(parameterName));
  }

  Time* MariaDbProcedureStatement::getTime(int32_t parameterIndex)
  {
    return getOutputResult()->getTime(indexToOutputIndex(parameterIndex));
  }

  Timestamp* MariaDbProcedureStatement::getTimestamp(int32_t parameterIndex)
  {
    return getOutputResult()->getTimestamp(parameterIndex);
  }

  Timestamp* MariaDbProcedureStatement::getTimestamp(const SQLString& parameterName)
  {
    return getOutputResult()->getTimestamp(nameToOutputIndex(parameterName));
  }

  RowId* MariaDbProcedureStatement::getRowId(int32_t parameterIndex)
  {
    throw ExceptionFactory::INSTANCE.notSupported("RowIDs not supported");
  }

  RowId* MariaDbProcedureStatement::getRowId(const SQLString& parameterName)
  {
    throw ExceptionFactory::INSTANCE.notSupported("RowIDs not supported");
  }


  SQLXML* MariaDbProcedureStatement::getSQLXML(int32_t parameterIndex)
  {
    throw ExceptionFactory::INSTANCE.notSupported("SQLXML& not supported");
  }

  SQLXML* MariaDbProcedureStatement::getSQLXML(const SQLString& parameterName)
  {
    throw ExceptionFactory::INSTANCE.notSupported("SQLXML& not supported");
  }

  std::istringstream* MariaDbProcedureStatement::getNCharacterStream(int32_t parameterIndex)
  {
    return getOutputResult()->getCharacterStream(indexToOutputIndex(parameterIndex));
  }

  std::istringstream* MariaDbProcedureStatement::getNCharacterStream(const SQLString& parameterName)
  {
    return getOutputResult()->getCharacterStream(nameToOutputIndex(parameterName));
  }


  SQLString MariaDbProcedureStatement::getNString(int32_t parameterIndex)
  {
    return getOutputResult()->getString(indexToOutputIndex(parameterIndex));
  }

  SQLString MariaDbProcedureStatement::getNString(const SQLString& parameterName)
  {
    return getOutputResult()->getString(nameToOutputIndex(parameterName));
  }

  std::istringstream* MariaDbProcedureStatement::getCharacterStream(int32_t parameterIndex)
  {
    return getOutputResult()->getCharacterStream(indexToOutputIndex(parameterIndex));
  }

  std::istringstream* MariaDbProcedureStatement::getCharacterStream(const SQLString& parameterName)
  {
    return getOutputResult()->getCharacterStream(nameToOutputIndex(parameterName));
  }
#endif
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
  void MariaDbProcedureStatement::registerOutParameter(int32_t parameterIndex, int32_t sqlType, const SQLString& typeName)
  {
    CallParameter& callParameter= getParameter(parameterIndex);
    callParameter.setOutputSqlType(sqlType);
    callParameter.setTypeName(typeName);
    callParameter.setOutput(true);
  }

  void MariaDbProcedureStatement::registerOutParameter(int32_t parameterIndex, int32_t sqlType)
  {
    registerOutParameter(parameterIndex, sqlType, -1);
  }

  void MariaDbProcedureStatement::registerOutParameter(int32_t parameterIndex, int32_t sqlType, int32_t scale)
  {
    CallParameter& callParameter= getParameter(parameterIndex);
    callParameter.setOutput(true);
    callParameter.setOutputSqlType(sqlType);
    callParameter.setScale(scale);
  }

  void MariaDbProcedureStatement::registerOutParameter(const SQLString& parameterName, int32_t sqlType)
  {
    registerOutParameter(nameToIndex(parameterName), sqlType);
  }

  void MariaDbProcedureStatement::registerOutParameter(const SQLString& parameterName, int32_t sqlType, int32_t scale)
  {
    registerOutParameter(nameToIndex(parameterName), sqlType, scale);
  }

  void MariaDbProcedureStatement::registerOutParameter(const SQLString& parameterName, int32_t sqlType, const SQLString& typeName)
  {
    registerOutParameter(nameToIndex(parameterName), sqlType, typeName);
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  void MariaDbProcedureStatement::registerOutParameter(int32_t parameterIndex, SQLType* sqlType)
  {
    registerOutParameter(parameterIndex, sqlType->getVendorTypeNumber());
  }

  void MariaDbProcedureStatement::registerOutParameter(int32_t parameterIndex, SQLType* sqlType, int32_t scale)
  {
    registerOutParameter(parameterIndex, sqlType->getVendorTypeNumber(), scale);
  }

  void MariaDbProcedureStatement::registerOutParameter(int32_t parameterIndex, SQLType* sqlType, const SQLString& typeName)
  {
    registerOutParameter(parameterIndex, sqlType->getVendorTypeNumber(), typeName);
  }

  void MariaDbProcedureStatement::registerOutParameter(const SQLString& parameterName, SQLType* sqlType)
  {
    registerOutParameter(parameterName, sqlType->getVendorTypeNumber());
  }

  void MariaDbProcedureStatement::registerOutParameter(const SQLString& parameterName, SQLType* sqlType, int32_t scale)
  {
    registerOutParameter(parameterName, sqlType->getVendorTypeNumber(), scale);
  }

  void MariaDbProcedureStatement::registerOutParameter(const SQLString& parameterName, SQLType* sqlType, const SQLString& typeName)
  {
    registerOutParameter(parameterName, sqlType->getVendorTypeNumber(), typeName);
  }

  void MariaDbProcedureStatement::setSQLXML(const SQLString& parameterName, const SQLXML& xmlObject)
  {
    throw ExceptionFactory::INSTANCE.notSupported("SQLXML& not supported");
  }

  void MariaDbProcedureStatement::setRowId(const SQLString& parameterName, const RowId& rowid)
  {
    throw ExceptionFactory::INSTANCE.notSupported("RowIDs not supported");
  }

#ifdef MAYBE_IN_NEXTVERSION
  void MariaDbProcedureStatement::setNString(const SQLString& parameterName, const SQLString& value)
  {
    setString(nameToIndex(parameterName), value);
  }
#endif
  void MariaDbProcedureStatement::setNCharacterStream(const SQLString& parameterName, const std::istringstream& value, int64_t length)
  {
    setCharacterStream(nameToIndex(parameterName), value, length);
  }

  void MariaDbProcedureStatement::setNCharacterStream(const SQLString& parameterName, std::istringstream& value)
  {
    setCharacterStream(nameToIndex(parameterName), value);
  }

  void MariaDbProcedureStatement::setNClob(const SQLString& parameterName, NClob& value)
  {
    setClob(nameToIndex(parameterName), value);
  }

  void MariaDbProcedureStatement::setNClob(const SQLString& parameterName, std::istringstream& reader, int64_t length)
  {
    setClob(nameToIndex(parameterName), reader, length);
  }

  void MariaDbProcedureStatement::setNClob(const SQLString& parameterName, std::istringstream& reader)
  {
    setClob(nameToIndex(parameterName), reader);
  }

  void MariaDbProcedureStatement::setClob(const SQLString& parameterName, std::istringstream& reader, int64_t length)
  {
    setClob(nameToIndex(parameterName), reader, length);
  }

  void MariaDbProcedureStatement::setClob(const SQLString& parameterName, Clob& clob)
  {
    setClob(nameToIndex(parameterName), clob);
  }

  void MariaDbProcedureStatement::setClob(const SQLString& parameterName, std::istringstream& reader)
  {
    setClob(nameToIndex(parameterName), reader);
  }

  void MariaDbProcedureStatement::setBlob(const SQLString& parameterName, std::istream* inputStream, int64_t length)
  {
    setBlob(nameToIndex(parameterName), inputStream, length);
  }

  void MariaDbProcedureStatement::setBlob(const SQLString& parameterName, Blob& blob)
  {
    setBlob(nameToIndex(parameterName), blob);
  }

  void MariaDbProcedureStatement::setBlob(const SQLString& parameterName, std::istream* inputStream)
  {
    setBlob(nameToIndex(parameterName), inputStream);
  }

  void MariaDbProcedureStatement::setAsciiStream(const SQLString& parameterName, std::istream* inputStream, int64_t length)
  {
    setAsciiStream(nameToIndex(parameterName), inputStream, length);
  }

  void MariaDbProcedureStatement::setAsciiStream(const SQLString& parameterName, std::istream* inputStream, int32_t length)
  {
    setAsciiStream(nameToIndex(parameterName), inputStream, length);
  }

  void MariaDbProcedureStatement::setAsciiStream(const SQLString& parameterName, std::istream* inputStream)
  {
    setAsciiStream(nameToIndex(parameterName), inputStream);
  }

  void MariaDbProcedureStatement::setURL(const SQLString& parameterName, const URL& url)
  {
    setURL(nameToIndex(parameterName), url);
  }

  void MariaDbProcedureStatement::setDate(const SQLString& parameterName, const Date& date)
  {
    setDate(nameToIndex(parameterName), date);
  }

  void MariaDbProcedureStatement::setTime(const SQLString& parameterName, const Time& time)
  {
    setTime(nameToIndex(parameterName), time);
  }

  void MariaDbProcedureStatement::setTimestamp(const SQLString& parameterName, const Timestamp& timestamp)
  {
    setTimestamp(nameToIndex(parameterName), timestamp);
  }

  void MariaDbProcedureStatement::setObject(const SQLString& parameterName, sql::Object* obj, int32_t targetSqlType, int32_t scale)
  {
    setObject(nameToIndex(parameterName), obj, targetSqlType, scale);
  }

  void MariaDbProcedureStatement::setObject(const SQLString& parameterName, sql::Object* obj, int32_t targetSqlType)
  {
    setObject(nameToIndex(parameterName), obj, targetSqlType);
  }

  void MariaDbProcedureStatement::setObject(const SQLString& parameterName, sql::Object* obj)
  {
    setObject(nameToIndex(parameterName), obj);
  }

  void MariaDbProcedureStatement::setObject(const SQLString& parameterName, sql::Object* obj, SQLType* targetSqlType, int32_t scaleOrLength)
  {
    setObject(nameToIndex(parameterName), obj, targetSqlType->getVendorTypeNumber(), scaleOrLength);
  }

  void MariaDbProcedureStatement::setObject(const SQLString& parameterName, sql::Object* obj, SQLType* targetSqlType)
  {
    setObject(nameToIndex(parameterName), obj, targetSqlType->getVendorTypeNumber());
  }
#endif


  CallParameter& MariaDbProcedureStatement::getParameter(uint32_t index)
  {
    if (index > params.size() || index <= 0) {
      throw SQLException("No parameter with index " + std::to_string(index));
    }
    return params[index -1];
  }

  Shared::Results & MariaDbProcedureStatement::getResults()
  {
    MariaDbStatement* mstmt= static_cast<MariaDbStatement*>(*stmt);
    return mstmt->getInternalResults();
  }


#ifdef MAYBE_IN_NEXTVERSION
  void MariaDbProcedureStatement::setBinaryStream(const SQLString& parameterName, const std::istream& inputStream, int64_t length)
  {
    stmt->setBinaryStream(nameToIndex(parameterName), inputStream, length);
  }

  void MariaDbProcedureStatement::setBinaryStream(const SQLString& parameterName, const std::istream& inputStream)
  {
    stmt->setBinaryStream(nameToIndex(parameterName), inputStream);
  }

  void MariaDbProcedureStatement::setBinaryStream(const SQLString& parameterName, const std::istream& inputStream, int32_t length)
  {
    stmt->setBinaryStream(nameToIndex(parameterName), inputStream, length);
  }

  void MariaDbProcedureStatement::setCharacterStream(const SQLString& parameterName, const std::istringstream& reader, int64_t length)
  {
    stmt->setCharacterStream(nameToIndex(parameterName), reader, length);
  }

  void MariaDbProcedureStatement::setCharacterStream(const SQLString& parameterName, const std::istringstream& reader)
  {
    stmt->setCharacterStream(nameToIndex(parameterName), reader);
  }

  void MariaDbProcedureStatement::setCharacterStream(const SQLString& parameterName, const std::istringstream& reader, int32_t length)
  {
    stmt->setCharacterStream(nameToIndex(parameterName), reader, length);
  }

  void MariaDbProcedureStatement::setBigDecimal(const SQLString& parameterName, const BigDecimal& bigDecimal)
  {
    stmt->setBigDecimal(nameToIndex(parameterName), bigDecimal);
  }
#endif

  void MariaDbProcedureStatement::setNull(const SQLString& parameterName, int32_t sqlType)
  {
    stmt->setNull(nameToIndex(parameterName), sqlType);
  }

  void MariaDbProcedureStatement::setNull(const SQLString& parameterName, int32_t sqlType, const SQLString& typeName)
  {
    stmt->setNull(nameToIndex(parameterName), sqlType, typeName);
  }

  void MariaDbProcedureStatement::setBoolean(const SQLString& parameterName, bool boolValue)
  {
    stmt->setBoolean(nameToIndex(parameterName), boolValue);
  }

  void MariaDbProcedureStatement::setByte(const SQLString& parameterName, char byteValue)
  {
    stmt->setByte(nameToIndex(parameterName), byteValue);
  }


  void MariaDbProcedureStatement::setShort(const SQLString& parameterName, int16_t shortValue) {
    stmt->setShort(nameToIndex(parameterName), shortValue);
  }


  void MariaDbProcedureStatement::setInt(const SQLString& parameterName, int32_t intValue) {
    stmt->setInt(nameToIndex(parameterName), intValue);
  }


  void MariaDbProcedureStatement::setLong(const SQLString& parameterName, int64_t longValue) {
    stmt->setLong(nameToIndex(parameterName), longValue);
  }


  void MariaDbProcedureStatement::setUInt64(int32_t parameterIdx, uint64_t uLongValue) {
    stmt->setUInt64(parameterIdx, uLongValue);
  }


  void MariaDbProcedureStatement::setUInt(int32_t parameterIndex, uint32_t value) {
    stmt->setUInt(parameterIndex, value);
  }


  void MariaDbProcedureStatement::setFloat(const SQLString& parameterName, float floatValue) {
    stmt->setFloat(nameToIndex(parameterName), floatValue);
  }


  void MariaDbProcedureStatement::setDouble(const SQLString& parameterName, double doubleValue) {
    stmt->setDouble(nameToIndex(parameterName), doubleValue);
  }


  void MariaDbProcedureStatement::setBigInt(int32_t parameterIndex, const SQLString& value) {
    stmt->setBigInt(parameterIndex, value);
  }


  void MariaDbProcedureStatement::setString(const SQLString& parameterName, const SQLString& stringValue)
  {
    stmt->setString(nameToIndex(parameterName), stringValue);
  }

  void MariaDbProcedureStatement::setBytes(const SQLString& parameterName, sql::bytes* bytes)
  {
    stmt->setBytes(nameToIndex(parameterName), bytes);
  }

  bool MariaDbProcedureStatement::execute(const sql::SQLString &sql, const sql::SQLString *colNames) {
    return stmt->execute(sql, colNames);
  }
  bool MariaDbProcedureStatement::execute(const sql::SQLString &sql, int32_t *colIdxs) {
    return stmt->execute(sql, colIdxs);
  }
  bool MariaDbProcedureStatement::execute(const SQLString& sql) {
    return stmt->execute(sql);
  }
  bool MariaDbProcedureStatement::execute(const SQLString& sql, int32_t autoGeneratedKeys) {
    return stmt->execute(sql, autoGeneratedKeys);
  }

  int32_t MariaDbProcedureStatement::executeUpdate(const SQLString& sql) {
    return stmt->executeUpdate(sql);
  }
  int32_t MariaDbProcedureStatement::executeUpdate(const SQLString& sql, int32_t autoGeneratedKeys) {
    return stmt->executeUpdate(sql, autoGeneratedKeys);
  }
  int32_t MariaDbProcedureStatement::executeUpdate(const SQLString& sql, int32_t* columnIndexes) {
    return stmt->executeUpdate(sql, columnIndexes);
  }
  int32_t MariaDbProcedureStatement::executeUpdate(const SQLString& sql, const SQLString* columnNames) {
    return stmt->executeUpdate(sql, columnNames);
  }

  int64_t MariaDbProcedureStatement::executeLargeUpdate(const SQLString& sql) { return stmt->executeLargeUpdate(sql); }
  int64_t MariaDbProcedureStatement::executeLargeUpdate(const SQLString& sql, int32_t autoGeneratedKeys) { return stmt->executeLargeUpdate(sql, autoGeneratedKeys); }
  int64_t MariaDbProcedureStatement::executeLargeUpdate(const SQLString& sql, int32_t* columnIndexes) { return stmt->executeLargeUpdate(sql, columnIndexes); }
  int64_t MariaDbProcedureStatement::executeLargeUpdate(const SQLString& sql, const SQLString* columnNames) { return stmt->executeLargeUpdate(sql, columnNames); }

  ResultSet* MariaDbProcedureStatement::executeQuery() {
      return stmt->executeQuery();
    }
  ResultSet* MariaDbProcedureStatement::executeQuery(const SQLString& sql) {
    return dynamic_cast<Statement*>(stmt.get())->executeQuery(sql);
  }

  void MariaDbProcedureStatement::setNull(int32_t parameterIndex, int32_t sqlType) {
    stmt->setNull(parameterIndex, sqlType);
  }

  /*void MariaDbProcedureStatement::setNull(int32_t parameterIndex, const ColumnType& mariadbType) {
    stmt->setNull(parameterIndex, mariadbType);
  }*/

  void MariaDbProcedureStatement::setNull(int32_t parameterIndex, int32_t sqlType, const SQLString& typeName) {
    stmt->setNull(parameterIndex, sqlType, typeName);
  }
  void MariaDbProcedureStatement::setBlob(int32_t parameterIndex, std::istream* inputStream, const int64_t length) {
    stmt->setBlob(parameterIndex, inputStream, length);
  }
  void MariaDbProcedureStatement::setBlob(int32_t parameterIndex, std::istream* inputStream) {
    stmt->setBlob(parameterIndex, inputStream);
  }

  void MariaDbProcedureStatement::setBoolean(int32_t parameterIndex, bool value) {
    stmt->setBoolean(parameterIndex, value);
  }
  void MariaDbProcedureStatement::setByte(int32_t parameterIndex, int8_t byte) {
    stmt->setByte(parameterIndex, byte);
  }
  void MariaDbProcedureStatement::setShort(int32_t parameterIndex, int16_t value) {
    stmt->setShort(parameterIndex, value);
  }
  void MariaDbProcedureStatement::setString(int32_t parameterIndex, const SQLString& str) {
    stmt->setString(parameterIndex, str);
  }
  void MariaDbProcedureStatement::setBytes(int32_t parameterIndex, sql::bytes* bytes) {
    stmt->setBytes(parameterIndex, bytes);
  }
  void MariaDbProcedureStatement::setInt(int32_t column, int32_t value) {
    stmt->setInt(column, value);
  }
  void MariaDbProcedureStatement::setLong(int32_t parameterIndex, int64_t value) {
    stmt->setLong(parameterIndex, value);
  }
  void MariaDbProcedureStatement::setFloat(int32_t parameterIndex, float value) {
    stmt->setFloat(parameterIndex, value);
  }
  void MariaDbProcedureStatement::setDouble(int32_t parameterIndex, double value) {
    stmt->setDouble(parameterIndex, value);
  }

  void MariaDbProcedureStatement::setDateTime(int32_t parameterIndex, const SQLString & dt)
  {
    stmt->setDateTime(parameterIndex, dt);
  }

  uint32_t MariaDbProcedureStatement::getMaxFieldSize() { return stmt->getMaxFieldSize(); }
  void MariaDbProcedureStatement::setMaxFieldSize(uint32_t max) { stmt->setMaxFieldSize(max); }
  int32_t MariaDbProcedureStatement::getMaxRows() { return stmt->getMaxRows(); }
  void MariaDbProcedureStatement::setMaxRows(int32_t max) { stmt->setMaxRows(max); }
  int64_t MariaDbProcedureStatement::getLargeMaxRows() { return stmt->getLargeMaxRows(); }
  void MariaDbProcedureStatement::setLargeMaxRows(int64_t max) { stmt->setLargeMaxRows(max); }
  void MariaDbProcedureStatement::setEscapeProcessing(bool enable) { stmt->setEscapeProcessing(enable); }
  int32_t MariaDbProcedureStatement::getQueryTimeout() { return stmt->getQueryTimeout(); }
  void MariaDbProcedureStatement::setQueryTimeout(int32_t seconds) { stmt->setQueryTimeout(seconds); }
  void MariaDbProcedureStatement::cancel() { stmt->cancel(); }
  SQLWarning* MariaDbProcedureStatement::getWarnings() { return stmt->getWarnings(); }
  void MariaDbProcedureStatement::clearWarnings() { stmt->clearWarnings(); }
  void MariaDbProcedureStatement::setCursorName(const SQLString& name) { stmt->setCursorName(name); }
  Connection* MariaDbProcedureStatement::getConnection() { return stmt->getConnection(); }
  ResultSet* MariaDbProcedureStatement::getGeneratedKeys() { return stmt->getGeneratedKeys(); }
  int32_t MariaDbProcedureStatement::getResultSetHoldability() { return stmt->getResultSetHoldability(); }
  bool MariaDbProcedureStatement::isClosed() { return stmt->isClosed(); }
  bool MariaDbProcedureStatement::isPoolable() { return stmt->isPoolable(); }
  void MariaDbProcedureStatement::setPoolable(bool poolable) { stmt->setPoolable(poolable); }
  ResultSet* MariaDbProcedureStatement::getResultSet() { return stmt->getResultSet(); }
  int32_t MariaDbProcedureStatement::getUpdateCount() { return stmt->getUpdateCount(); }
  int64_t MariaDbProcedureStatement::getLargeUpdateCount() { return stmt->getLargeUpdateCount(); }
  bool MariaDbProcedureStatement::getMoreResults() { return stmt->getMoreResults(); }
  bool MariaDbProcedureStatement::getMoreResults(int32_t current) { return stmt->getMoreResults(current); }
  int32_t MariaDbProcedureStatement::getFetchDirection() { return stmt->getFetchDirection(); }
  void MariaDbProcedureStatement::setFetchDirection(int32_t direction) { stmt->setFetchDirection(direction); }
  int32_t MariaDbProcedureStatement::getFetchSize() { return stmt->getFetchSize(); }
  void MariaDbProcedureStatement::setFetchSize(int32_t rows) { stmt->setFetchSize(rows); }
  int32_t MariaDbProcedureStatement::getResultSetConcurrency() { return stmt->getResultSetConcurrency(); }
  int32_t MariaDbProcedureStatement::getResultSetType() { return stmt->getResultSetType(); }
  void MariaDbProcedureStatement::closeOnCompletion() { stmt->closeOnCompletion(); }
  bool MariaDbProcedureStatement::isCloseOnCompletion() { return stmt->isCloseOnCompletion(); }

  void MariaDbProcedureStatement::addBatch() {
    stmt->addBatch();
  }

  void MariaDbProcedureStatement::addBatch(const SQLString& sql) {
    stmt->addBatch(sql);
  }

  void MariaDbProcedureStatement::clearBatch() {
    stmt->clearBatch();
  }

  void MariaDbProcedureStatement::close() {
    stmt->close();
  }

  const sql::Longs& MariaDbProcedureStatement::executeLargeBatch() {
    return stmt->executeLargeBatch();
  }

  int32_t MariaDbProcedureStatement::executeUpdate() {
      return stmt->executeUpdate();
  }

  int64_t MariaDbProcedureStatement::executeLargeUpdate() {
    return stmt->executeLargeUpdate();
  }

  Statement* MariaDbProcedureStatement::setResultSetType(int32_t rsType) {
    stmt->setResultSetType(rsType);
    return this;
  }

  void MariaDbProcedureStatement::clearParameters() {
    stmt->clearParameters();
  }
}
}
