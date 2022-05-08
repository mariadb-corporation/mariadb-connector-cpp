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


#include "MariaDbConnection.h"

#include "MariaDBWarning.h"
#include "MariaDbSavepoint.h"
#include "ServerSidePreparedStatement.h"
#include "ClientSidePreparedStatement.h"
#include "MariaDbProcedureStatement.h"
#include "MariaDbFunctionStatement.h"
#include "MariaDbDatabaseMetaData.h"
#include "CallableParameterMetaData.h"

#include "logger/LoggerFactory.h"
#include "pool/Pools.h"
#include "util/Utils.h"
#include "jdbccompat.hpp"
#include "ExceptionFactory.h"

namespace sql
{
namespace mariadb
{
  Shared::Logger MariaDbConnection::logger= LoggerFactory::getLogger(typeid(MariaDbConnection));
  /**
    * Pattern to check the correctness of callable statement query string Legal queries, as
    * documented in JDK have the form: {[?=]call[(arg1,..,,argn)]}
    */
  std::regex MariaDbConnection::CALLABLE_STATEMENT_PATTERN(
    "^(\\s*\\{)?\\s*((\\?\\s*=)?(\\s*\\/\\*([^\\*]|\\*[^\\/])*\\*\\/)*\\s*"
    "call(\\s*\\/\\*([^\\*]|\\*[^\\/])*\\*\\/)*\\s*((((`[^`]+`)|([^`\\}]+))\\.)?"
    "((`[^`]+`)|([^`\\}\\(]+)))\\s*(\\(.*\\))?(\\s*\\/\\*([^\\*]|\\*[^\\/])*\\*\\/)*"
    "\\s*(#.*)?)\\s*(\\}\\s*)?$", std::regex_constants::ECMAScript | std::regex_constants::icase);
  /** Check that query can be executed with PREPARE. */
  std::regex MariaDbConnection::PREPARABLE_STATEMENT_PATTERN(
    "^(\\s*\\/\\*([^\\*]|\\*[^\\/])*\\*\\/)*\\s*(SELECT|UPDATE|INSERT|DELETE|REPLACE|DO|CALL)", std::regex_constants::ECMAScript | std::regex_constants::icase);
  /**
    * Creates a new connection with a given protocol and query factory.
    *
    * @param protocol the protocol to use.
    */
  MariaDbConnection::MariaDbConnection(Shared::Protocol& _protocol) :
    protocol(_protocol),
    lock(_protocol->getLock()),
    options(protocol->getOptions()),
    _canUseServerTimeout(protocol->versionGreaterOrEqual(10, 1, 2)),
    sessionStateAware(protocol->sessionStateAware()),
    nullCatalogMeansCurrent(options->nullCatalogMeansCurrent),
    exceptionFactory(ExceptionFactory::of(this->getServerThreadId(), options)),
    lowercaseTableNames(-1),
    pooledConnection(nullptr),
    stateFlag(0),
    defaultTransactionIsolation(0),
    savepointCount(0),
    warningsCleared(true)
  {
    if (options->cacheCallableStmts)
    {
      callableStatementCache.reset(CallableStatementCache::newInstance(options->callableStmtCacheSize));
    }
  }

  /**
    * Create new connection Object.
    *
    * @param urlParser parser
    * @param globalInfo global info
    * @return connection object
    * @throws SQLException if any connection error occur
    */
  MariaDbConnection* MariaDbConnection::newConnection(UrlParser &urlParser, GlobalStateInfo *globalInfo)
  {
    if (urlParser.getOptions()->pool)
    {
      //return Pools::retrievePool(urlParser)->getConnection();
    }
    Shared::Protocol protocol(Utils::retrieveProxy(urlParser, globalInfo));

    return new MariaDbConnection(protocol);
  }


  SQLString MariaDbConnection::quoteIdentifier(const SQLString& string) {
    return "`"+replaceAll(string, "`", "``")+"`";
  }


  SQLString MariaDbConnection::unquoteIdentifier(SQLString& string)
  {
    if (string.startsWith("`") && string.endsWith("`") && string.length() >= 2) {
      return replace(string.substr(1, string.length()-1), "``", "`");
    }
    return string;
  }

  MariaDbConnection::~MariaDbConnection()
  {
    protocol->closeExplicit();
  }


  Shared::Protocol& MariaDbConnection::getProtocol() {
    return protocol;
  }

  /**
    * creates a new statement.
    *
    * @return a statement
    * @throws SQLException if we cannot create the statement.
    */
  Statement* MariaDbConnection::createStatement()
  {
    checkConnection();
    return new MariaDbStatement(this, ResultSet::TYPE_FORWARD_ONLY, ResultSet::CONCUR_READ_ONLY, exceptionFactory);
  }

  /**
    * Creates a <code>Statement</code> object that will generate <code>ResultSet</code> objects with
    * the given type and concurrency. This method is the same as the <code>createStatement</code>
    * method above, but it allows the default result set type and concurrency to be overridden. The
    * holdability of the created result sets can be determined by calling {@link #getHoldability}.
    *
    * @param resultSetType a result set type; one of <code>ResultSet.TYPE_FORWARD_ONLY</code>, <code>
    *     ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @return a new <code>Statement</code> object that will generate <code>ResultSet</code> objects
    *     with the given type and concurrency
    */

  Statement* MariaDbConnection::createStatement(int32_t resultSetType, int32_t resultSetConcurrency) {
    return new MariaDbStatement(this, resultSetType, resultSetConcurrency, exceptionFactory);
  }
  /**
    * Creates a <code>Statement</code> object that will generate <code>ResultSet</code> objects with
    * the given type, concurrency, and holdability. This method is the same as the <code>
    * createStatement</code> method above, but it allows the default result set type, concurrency,
    * and holdability to be overridden.
    *
    * @param resultSetType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.CONCUR_READ_ONLY</code> or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @param resultSetHoldability one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.HOLD_CURSORS_OVER_COMMIT</code> or <code>ResultSet.CLOSE_CURSORS_AT_COMMIT</code>
    * @return a new <code>Statement</code> object that will generate <code>ResultSet</code> objects
    *     with the given type, concurrency, and holdability
    * @see ResultSet
    */
  Statement* MariaDbConnection::createStatement(int32_t resultSetType, int32_t resultSetConcurrency, int32_t resultSetHoldability) {
    return new MariaDbStatement(this, resultSetType, resultSetConcurrency, exceptionFactory);
  }


  void MariaDbConnection::checkConnection()
  {
    if (protocol->isExplicitClosed()) {
      exceptionFactory->create("createStatement() is called on closed connection", "08000").Throw();
    }
    if (protocol->isClosed() && protocol->getProxy())
    {
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try
      {
        protocol->getProxy()->reconnect();
      }
      catch (SQLException&)
      {
      }
    }
  }

  /**
    * Create a new client prepared statement.
    *
    * @param sql the query.
    * @return a client prepared statement.
    * @throws SQLException if there is a problem preparing the statement.
    */
  ClientSidePreparedStatement* MariaDbConnection::clientPrepareStatement(const SQLString& sql)
  {
    return new ClientSidePreparedStatement(
      this,
      sql,
      ResultSet::TYPE_FORWARD_ONLY,
      ResultSet::CONCUR_READ_ONLY,
      Statement::RETURN_GENERATED_KEYS,
      exceptionFactory);
  }

/**
  * Create a new server prepared statement.
  *
  * @param sql the query.
  * @return a server prepared statement.
  * @throws SQLException if there is a problem preparing the statement.
  */
  ServerSidePreparedStatement* MariaDbConnection::serverPrepareStatement(const SQLString& sql)
  {
    return new ServerSidePreparedStatement(
      this,
      sql,
      ResultSet::TYPE_FORWARD_ONLY,
      ResultSet::CONCUR_READ_ONLY,
      Statement::RETURN_GENERATED_KEYS,
      exceptionFactory);
  }
  /**
    * creates a new prepared statement.
    *
    * @param sql the query.
    * @return a prepared statement.
    * @throws SQLException if there is a problem preparing the statement.
    */
  PreparedStatement* MariaDbConnection::prepareStatement(const SQLString& sql) {
    return internalPrepareStatement(
      sql, ResultSet::TYPE_FORWARD_ONLY, ResultSet::CONCUR_READ_ONLY, Statement::NO_GENERATED_KEYS);
  }
  /**
    * Creates a <code>PreparedStatement</code> object that will generate <code>ResultSet</code>
    * objects with the given type and concurrency. This method is the same as the <code>
    * prepareStatement</code> method above, but it allows the default result set type and concurrency
    * to be overridden. The holdability of the created result sets can be determined by calling
    * {@link #getHoldability}.
    *
    * @param sql a <code>String</code> object that is the SQL statement to be sent to the database;
    *     may contain one or more '?' IN parameters
    * @param resultSetType a result set type; one of <code>ResultSet.TYPE_FORWARD_ONLY</code>, <code>
    *     ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @return a new PreparedStatement object containing the pre-compiled SQL statement that will
    *     produce <code>ResultSet</code> objects with the given type and concurrency
    * @throws SQLException if a database access error occurs, this method is called on a closed
    *     connection or the given parameters are not<code>ResultSet</code> constants indicating type
    *     and concurrency
    */
  PreparedStatement* MariaDbConnection::prepareStatement(const SQLString& sql, int32_t resultSetType, int32_t resultSetConcurrency) {
    return internalPrepareStatement(
      sql, resultSetType, resultSetConcurrency, Statement::NO_GENERATED_KEYS);
  }
  /**
    * Creates a <code>PreparedStatement</code> object that will generate <code>ResultSet</code>
    * objects with the given type, concurrency, and holdability.
    *
    * <p>This method is the same as the <code>prepareStatement</code> method above, but it allows the
    * default result set type, concurrency, and holdability to be overridden.
    *
    * @param sql a <code>String</code> object that is the SQL statement to be sent to the database;
    *     may contain one or more '?' IN parameters
    * @param resultSetType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.CONCUR_READ_ONLY</code> or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @param resultSetHoldability one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.HOLD_CURSORS_OVER_COMMIT</code> or <code>ResultSet.CLOSE_CURSORS_AT_COMMIT</code>
    * @return a new <code>PreparedStatement</code> object, containing the pre-compiled SQL statement,
    *     that will generate <code>ResultSet</code> objects with the given type, concurrency, and
    *     holdability
    * @throws SQLException if a database access error occurs, this method is called on a closed
    *     connection or the given parameters are not <code>ResultSet</code> constants indicating
    *     type, concurrency, and holdability
    * @see ResultSet
    */
  PreparedStatement* MariaDbConnection::prepareStatement(
    const SQLString& sql,
    int32_t resultSetType,
    int32_t resultSetConcurrency,
    int32_t resultSetHoldability)
  {
    return internalPrepareStatement(
      sql, resultSetType, resultSetConcurrency, Statement::NO_GENERATED_KEYS);
  }
  /**
    * Creates a default <code>PreparedStatement</code> object that has the capability to retrieve
    * auto-generated keys. The given constant tells the driver whether it should make auto-generated
    * keys available for retrieval. This parameter is ignored if the SQL statement is not an <code>
    * INSERT</code> statement, or an SQL statement able to return auto-generated keys (the list of
    * such statements is vendor-specific).
    *
    * <p><B>Note:</B> This method is optimized for handling parametric SQL statements that benefit
    * from precompilation. If the driver supports precompilation, the method <code>prepareStatement
    * </code> will send the statement to the database for precompilation. Some drivers may not
    * support precompilation. In this case, the statement may not be sent to the database until the
    * <code>PreparedStatement</code> object is executed. This has no direct effect on users; however,
    * it does affect which methods throw certain SQLExceptions.
    *
    * <p>Result sets created using the returned <code>PreparedStatement</code> object will by default
    * be type <code>TYPE_FORWARD_ONLY</code> and have a concurrency level of <code>CONCUR_READ_ONLY
    * </code>. The holdability of the created result sets can be determined by calling {@link
    * #getHoldability}.
    *
    * @param sql an SQL statement that may contain one or more '?' IN parameter placeholders
    * @param autoGeneratedKeys a flag indicating whether auto-generated keys should be returned; one
    *     of <code>Statement.RETURN_GENERATED_KEYS</code> or <code>Statement.NO_GENERATED_KEYS</code>
    * @return a new <code>PreparedStatement</code> object, containing the pre-compiled SQL statement,
    *     that will have the capability of returning auto-generated keys
    * @throws SQLException if a database access error occurs, this method is called on a closed
    *     connection or the given parameter is not a <code>Statement</code> constant indicating
    *     whether auto-generated keys should be returned
    */
  PreparedStatement* MariaDbConnection::prepareStatement(const SQLString& sql, int32_t autoGeneratedKeys) {
    return internalPrepareStatement(
      sql, ResultSet::TYPE_FORWARD_ONLY, ResultSet::CONCUR_READ_ONLY, autoGeneratedKeys);
  }
  /**
    * Creates a default <code>PreparedStatement</code> object capable of returning the auto-generated
    * keys designated by the given array. This array contains the indexes of the columns in the
    * target table that contain the auto-generated keys that should be made available. The driver
    * will ignore the array if the SQL statement is not an <code>INSERT</code> statement, or an SQL
    * statement able to return auto-generated keys (the list of such statements is vendor-specific).
    *
    * <p>An SQL statement with or without IN parameters can be pre-compiled and stored in a <code>
    * PreparedStatement</code> object. This object can then be used to efficiently execute this
    * statement multiple times.
    *
    * <p><B>Note:</B> This method is optimized for handling parametric SQL statements that benefit
    * from precompilation. If the driver supports precompilation, the method <code>prepareStatement
    * </code> will send the statement to the database for precompilation. Some drivers may not
    * support precompilation. In this case, the statement may not be sent to the database until the
    * <code>PreparedStatement</code> object is executed. This has no direct effect on users; however,
    * it does affect which methods throw certain SQLExceptions.
    *
    * <p>Result sets created using the returned <code>PreparedStatement</code> object will by default
    * be type <code>TYPE_FORWARD_ONLY</code> and have a concurrency level of <code>CONCUR_READ_ONLY
    * </code>. The holdability of the created result sets can be determined by calling {@link
    * #getHoldability}.
    *
    * @param sql an SQL statement that may contain one or more '?' IN parameter placeholders
    * @param columnIndexes an array of column indexes indicating the columns that should be returned
    *     from the inserted row or rows
    * @return a new <code>PreparedStatement</code> object, containing the pre-compiled statement,
    *     that is capable of returning the auto-generated keys designated by the given array of
    *     column indexes
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     connection
    */
  PreparedStatement* MariaDbConnection::prepareStatement(const SQLString& sql, int32_t* columnIndexes) {
    return prepareStatement(sql, Statement::RETURN_GENERATED_KEYS);
  }
  /**
    * Creates a default <code>PreparedStatement</code> object capable of returning the auto-generated
    * keys designated by the given array. This array contains the names of the columns in the target
    * table that contain the auto-generated keys that should be returned. The driver will ignore the
    * array if the SQL statement is not an <code>INSERT</code> statement, or an SQL statement able to
    * return auto-generated keys (the list of such statements is vendor-specific).
    *
    * <p>An SQL statement with or without IN parameters can be pre-compiled and stored in a <code>
    * PreparedStatement</code> object. This object can then be used to efficiently execute this
    * statement multiple times.
    *
    * <p><B>Note:</B> This method is optimized for handling parametric SQL statements that benefit
    * from precompilation. If the driver supports precompilation, the method <code>prepareStatement
    * </code> will send the statement to the database for precompilation. Some drivers may not
    * support precompilation. In this case, the statement may not be sent to the database until the
    * <code>PreparedStatement</code> object is executed. This has no direct effect on users; however,
    * it does affect which methods throw certain SQLExceptions.
    *
    * <p>Result sets created using the returned <code>PreparedStatement</code> object will by default
    * be type <code>TYPE_FORWARD_ONLY</code> and have a concurrency level of <code>CONCUR_READ_ONLY
    * </code>. The holdability of the created result sets can be determined by calling {@link
    * #getHoldability}.
    *
    * @param sql an SQL statement that may contain one or more '?' IN parameter placeholders
    * @param columnNames an array of column names indicating the columns that should be returned from
    *     the inserted row or rows
    * @return a new <code>PreparedStatement</code> object, containing the pre-compiled statement,
    *     that is capable of returning the auto-generated keys designated by the given array of
    *     column names
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     connection
    */
  PreparedStatement* MariaDbConnection::prepareStatement(const SQLString& sql, const SQLString* columnNames) {
    return prepareStatement(sql, Statement::RETURN_GENERATED_KEYS);
  }
  /**
    * Send ServerPrepareStatement or ClientPrepareStatement depending on SQL query and options If
    * server side and PREPARE can be delayed, a facade will be return, to have a fallback on client
    * prepareStatement.
    *
    * @param sql sql query
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @param autoGeneratedKeys a flag indicating whether auto-generated keys should be returned; one
    *     of <code>Statement.RETURN_GENERATED_KEYS</code> or <code>Statement.NO_GENERATED_KEYS</code>
    * @return PrepareStatement
    * @throws SQLException if a connection error occur during the server preparation.
    */
  PreparedStatement* MariaDbConnection::internalPrepareStatement(
    const SQLString& sql,
    int32_t resultSetScrollType,
    int32_t resultSetConcurrency,
    int32_t autoGeneratedKeys)
  {
    if (!sql.empty())
    {
      SQLString sqlQuery= Utils::nativeSql(sql, protocol.get());

      if (options->useServerPrepStmts && std::regex_search(StringImp::get(sqlQuery), PREPARABLE_STATEMENT_PATTERN))
      {
        checkConnection();
        //try {
          return new ServerSidePreparedStatement(this, sqlQuery, resultSetScrollType, resultSetConcurrency,
            autoGeneratedKeys, exceptionFactory);
        //!!!! TODO: !!!!! Need to fix exceptions, so we do not throw SQLException's only
        /*}
        catch (SQLNonTransientConnectionException& e) {
          throw e;
        }
        catch (SQLException& e) {
          // on some specific case, server cannot prepared data (CONJ-238)
          // will use clientPreparedStatement
        }*/
      }
      return new ClientSidePreparedStatement(
        this, sqlQuery, resultSetScrollType, resultSetConcurrency, autoGeneratedKeys, exceptionFactory);
    }
    else {
      throw SQLException("SQL value can not be empty");
    }
  }
  /**
    * Creates a <code>CallableStatement</code> object for calling database stored procedures. The
    * <code>CallableStatement</code> object provides methods for setting up its IN and OUT
    * parameters, and methods for executing the call to a stored procedure. example : {?= call
    * &lt;procedure-name&gt;[(&lt;arg1&gt;,&lt;arg2&gt;, ...)]} or {call
    * &lt;procedure-name&gt;[(&lt;arg1&gt;,&lt;arg2&gt;, ...)]}
    *
    * <p><b>Note:</b> This method is optimized for handling stored procedure call statements.
    *
    * @param sql an SQL statement that may contain one or more '?' parameter placeholders. Typically
    *     this statement is specified using JDBC call escape syntax.
    * @return a new default <code>CallableStatement</code> object containing the pre-compiled SQL
    *     statement
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     connection
    */
  CallableStatement* MariaDbConnection::prepareCall(const SQLString& sql) {
    return prepareCall(sql, ResultSet::TYPE_FORWARD_ONLY, ResultSet::CONCUR_READ_ONLY);
  }
  /**
    * Creates a <code>CallableStatement</code> object that will generate <code>ResultSet</code>
    * objects with the given type and concurrency. This method is the same as the <code>prepareCall
    * </code> method above, but it allows the default result set type and concurrency to be
    * overridden. The holdability of the created result sets can be determined by calling {@link
    * #getHoldability}.
    *
    * @param sql a <code>String</code> object that is the SQL statement to be sent to the database;
    *     may contain on or more '?' parameters
    * @param resultSetType a result set type; one of <code>ResultSet.TYPE_FORWARD_ONLY</code>, <code>
    *     ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @return a new <code>CallableStatement</code> object containing the pre-compiled SQL statement
    *     that will produce <code>ResultSet</code> objects with the given type and concurrency
    * @throws SQLException if a database access error occurs, this method is called on a closed
    *     connection or the given parameters are not <code>ResultSet</code> constants indicating type
    *     and concurrency
    */
  CallableStatement* MariaDbConnection::prepareCall(const SQLString& sql, int32_t resultSetType, int32_t resultSetConcurrency)
  {
    checkConnection();
    std::smatch matcher;

    if (!std::regex_search(StringImp::get(sql), matcher, CALLABLE_STATEMENT_PATTERN))
    {
      throw SQLSyntaxErrorException(
        "invalid callable syntax. must be like {[?=]call <procedure/function name>[(?,?, ...)]}\n but was : "
        +sql);
    }

    SQLString query= Utils::nativeSql(matcher[2].str(), protocol.get());

    bool isFunction= !matcher[3].str().empty();

    SQLString databaseAndProcedure(matcher[8].str());
    SQLString database(matcher[10].str());
    SQLString procedureName(matcher[13].str());
    SQLString arguments(matcher[16].str());

    if (database.empty() && sessionStateAware)
    {
      database= protocol->getDatabase();
    }
    if (!database.empty() && options->cacheCallableStmts)
    {
      CallableStatementCacheKey key(database, query);
      CallableStatementCache::iterator it= callableStatementCache->find(key);

      if (it != callableStatementCache->end())
      {
        Shared::CallableStatement callableStatement(it->second);

        if (callableStatement)
        {
          /* Do we need a clone? */
          CloneableCallableStatement* cloneable= dynamic_cast<CloneableCallableStatement*>(callableStatement.get());

          if (cloneable == NULL)
          {
            throw std::runtime_error("Cached statement is not cloneable");
          }
          return cloneable->clone(this);
        }
      }
      CallableStatement* callableStatement =
        createNewCallableStatement(
          query,
          procedureName,
          isFunction,
          databaseAndProcedure,
          database,
          arguments,
          resultSetType,
          resultSetConcurrency,
          exceptionFactory);

      callableStatementCache->insert(key, callableStatement);
      return callableStatement;
    }

    return createNewCallableStatement(
      query,
      procedureName,
      isFunction,
      databaseAndProcedure,
      database,
      arguments,
      resultSetType,
      resultSetConcurrency,
      exceptionFactory);
  }
  /**
    * Creates a <code>CallableStatement</code> object that will generate <code>ResultSet</code>
    * objects with the given type and concurrency. This method is the same as the <code>prepareCall
    * </code> method above, but it allows the default result set type, result set concurrency type
    * and holdability to be overridden.
    *
    * @param sql a <code>String</code> object that is the SQL statement to be sent to the database;
    *     may contain on or more '?' parameters
    * @param resultSetType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.CONCUR_READ_ONLY</code> or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @param resultSetHoldability one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.HOLD_CURSORS_OVER_COMMIT</code> or <code>ResultSet.CLOSE_CURSORS_AT_COMMIT</code>
    * @return a new <code>CallableStatement</code> object, containing the pre-compiled SQL statement,
    *     that will generate <code>ResultSet</code> objects with the given type, concurrency, and
    *     holdability
    * @throws SQLException if a database access error occurs, this method is called on a closed
    *     connection or the given parameters are not <code>ResultSet</code> constants indicating
    *     type, concurrency, and holdability
    * @see ResultSet
    */
  CallableStatement* MariaDbConnection::prepareCall(
    const SQLString& sql,
    int32_t resultSetType,
    int32_t resultSetConcurrency,
    int32_t resultSetHoldability)
  {
    return prepareCall(sql);
  }


  CallableStatement* MariaDbConnection::createNewCallableStatement(
    SQLString query, SQLString& procedureName,
    bool isFunction, SQLString& databaseAndProcedure, SQLString& database, SQLString& arguments,
    int32_t resultSetType,
    int32_t resultSetConcurrency,
    Shared::ExceptionFactory& expFactory)
  {
    /*if (isFunction)
    {
      return new MariaDbFunctionStatement(
        this,
        database,
        databaseAndProcedure,
        (arguments.empty()) ? "()" : arguments,
        resultSetType,
        resultSetConcurrency);
    }
    else*/
    {
      return new MariaDbProcedureStatement(
        query, this, procedureName, database, resultSetType, resultSetConcurrency, expFactory);
    }
  }


  SQLString MariaDbConnection::nativeSQL(const SQLString& sql)
  {
    return Utils::nativeSql(sql, protocol.get());
  }

  /**
    * returns true if statements on this connection are auto commited.
    *
    * @return true if auto commit is on.
    * @throws SQLException if there is an error
    */
  bool MariaDbConnection::getAutoCommit()
  {
    return protocol->getAutocommit();
  }

  /**
    * Sets whether this connection is auto commited.
    *
    * @param autoCommit if it should be auto commited.
    * @throws SQLException if something goes wrong talking to the server.
    */
  void MariaDbConnection::setAutoCommit(bool autoCommit)
  {
    if (autoCommit == getAutoCommit())
    {
      return;
    }
    Unique::Statement stmt(createStatement());

    if (stmt)
    {
      stateFlag|= ConnectionState::STATE_AUTOCOMMIT;
      stmt->executeUpdate(SQLString("set autocommit=").append((autoCommit) ? '1' : '0'));
    }
  }

  /**
    * Sends commit to the server.
    *
    * @throws SQLException if there is an error commiting.
    */
  void MariaDbConnection::commit()
  {
    // Looks like this lock is not needed - it will be done below(where it's actually needed)
    //std::lock_guard<std::mutex> localScopeLock(*lock);

    if (protocol->inTransaction())
    {
      Unique::Statement st(this->createStatement());
      if (st)
      {
        st->execute("COMMIT");
      }
    }
  }

  /**
    * Rolls back a transaction.
    *
    * @throws SQLException if there is an error rolling back.
    */
  void MariaDbConnection::rollback()
  {
    if (protocol->inTransaction())
    {
      // It doesn't look that stmt creation needs lock. and execute will lock.
      Unique::Statement st(this->createStatement());
      if (st)
      {
        st->execute("ROLLBACK");
      }
    }
  }

  /**
    * Undoes all changes made after the given <code>Savepoint</code> object was set.
    *
    * <p>This method should be used only when auto-commit has been disabled.
    *
    * @param savepoint the <code>Savepoint</code> object to roll back to
    * @throws SQLException if a database access error occurs, this method is called while
    *     participating in a distributed transaction, this method is called on a closed connection,
    *     the <code>Savepoint</code> object is no longer valid, or this <code>Connection</code>
    *     object is currently in auto-commit mode
    * @see Savepoint
    * @see #rollback
    */
  void MariaDbConnection::rollback(const Savepoint* savepoint)
  {
    std::unique_lock<std::mutex> localScopeLock(*lock);
    Unique::Statement st(createStatement());
    localScopeLock.unlock();
    st->execute("ROLLBACK TO SAVEPOINT " + savepoint->toString());
  }

  /**
    * close the connection.
    *
    * @throws SQLException if there is a problem talking to the server.
    */
  void MariaDbConnection::close()
  {
    if (pooledConnection)
    {
      rollback();
      pooledConnection->fireConnectionClosed();
      return;
    }
    protocol->closeExplicit();
  }

  /**
    * checks if the connection is closed.
    *
    * @return true if the connection is closed
    */
  bool MariaDbConnection::isClosed()
  {
    return protocol->isClosed();
  }

  /**
    * returns the meta data about the database.
    *
    * @return meta data about the db.
    */
  DatabaseMetaData* MariaDbConnection::getMetaData()
  {
    return new MariaDbDatabaseMetaData(this, protocol->getUrlParser());
  }
  /**
    * Retrieves whether this <code>Connection</code> object is in read-only mode.
    *
    * @return <code>true</code> if this <code>Connection</code> object is read-only; <code>false
    *     </code> otherwise
    * @throws SQLException SQLException if a database access error occurs or this method is called on
    *     a closed connection
    */
  bool MariaDbConnection::isReadOnly() {
    return protocol->getReadonly();
  }
  /**
    * Sets whether this connection is read only.
    *
    * @param readOnly true if it should be read only.
    * @throws SQLException if there is a problem
    */
  void MariaDbConnection::setReadOnly(bool readOnly) {
    try
    {
      SQLString LogMsg("conn=");

      LogMsg.append(std::to_string(protocol->getServerThreadId())).append(protocol->isMasterConnection() ? "(M)" : "(S)").
            append(" - set read-only to value ").append(std::to_string(readOnly));
      logger->debug(LogMsg);

      if (readOnly) {
        stateFlag |= ConnectionState::STATE_READ_ONLY;
      }
      else {
        stateFlag &= ~static_cast<int32_t>(ConnectionState::STATE_READ_ONLY);
      }
      protocol->setReadonly(readOnly);
    }
    catch (SQLException &e) {
      throw e;// ExceptionMapper.getException(e, this, NULL, false);
    }
  }
  /**
    * Retrieves this <code>Connection</code> object's current catalog name.
    *
    * @return the current catalog name or <code>null</code> if there is none
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     connection
    * @see #setCatalog
    */
  SQLString MariaDbConnection::getCatalog()
  {
    return emptyStr;
  }
  /**
    * Sets the given catalog name in order to select a subspace of this <code>Connection</code>
    * object's database in which to work.
    *
    * <p>If the driver does not support catalogs, it will silently ignore this request. MariaDB
    * treats catalogs and databases as equivalent
    *
    * @param catalog the name of a catalog (subspace in this <code>Connection</code> object's
    *     database) in which to work
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     connection
    * @see #getCatalog
    */
  void MariaDbConnection::setCatalog(const SQLString& catalog) {
  }


  bool MariaDbConnection::isServerMariaDb() {
    return protocol->isServerMariaDb();
  }


  bool MariaDbConnection::versionGreaterOrEqual(int32_t major, int32_t minor, int32_t patch) {
    return protocol->versionGreaterOrEqual(major, minor, patch);
  }

  /**
    * Retrieves this <code>Connection</code> object's current transaction isolation level.
    *
    * @return the current transaction isolation level, which will be one of the following constants:
    *     <code>Connection.TRANSACTION_READ_UNCOMMITTED</code>, <code>
    *     Connection.TRANSACTION_READ_COMMITTED</code>, <code>Connection.TRANSACTION_REPEATABLE_READ
    *     </code>, <code>Connection.TRANSACTION_SERIALIZABLE</code>, or <code>
    *     Connection.TRANSACTION_NONE</code>.
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     connection
    * @see #setTransactionIsolation
    */
  int32_t MariaDbConnection::getTransactionIsolation()
  {
    Unique::Statement stmt(createStatement());

    SQLString sql("SELECT @@tx_isolation");

    if (!protocol->isServerMariaDb())
    {
      if ((protocol->getMajorServerVersion() >= 8 && protocol->versionGreaterOrEqual(8, 0, 3))
        ||(protocol->getMajorServerVersion() < 8 && protocol->versionGreaterOrEqual(5, 7, 20)))
      {
        sql= "SELECT @@transaction_isolation";
      }
    }
    //executeQuery has its locking
    Unique::ResultSet rs(stmt->executeQuery(sql));
    std::lock_guard<std::mutex> localScopeLock(*lock);

    if (rs->next())
    {
      const SQLString response(rs->getString(1));

      if (response.compare("REPEATABLE-READ") == 0)
      {
        return sql::TRANSACTION_REPEATABLE_READ;
      }
      else if (response.compare("READ-UNCOMMITTED") == 0)
      {
        return sql::TRANSACTION_READ_UNCOMMITTED;
      }
      else if (response.compare("READ-COMMITTED") == 0)
      {
        return sql::TRANSACTION_READ_COMMITTED;
      }
      else if (response.compare("SERIALIZABLE") == 0)
      {
        return sql::TRANSACTION_SERIALIZABLE;
      }
      else
      {
        throw SQLException("Could not get transaction isolation level: Invalid value \"" + response + "\"");
      }
    }
    exceptionFactory->create("Failed to retrieve transaction isolation").Throw();
    return sql::TRANSACTION_NONE;
  }

  /**
    * Attempts to change the transaction isolation level for this <code>Connection</code> object to
    * the one given. The constants defined in the interface <code>Connection</code> are the possible
    * transaction isolation levels.
    *
    * <p><B>Note:</B> If this method is called during a transaction, the result is
    * implementation-defined.
    *
    * @param level one of the following <code>Connection</code> constants: <code>
    *     Connection.TRANSACTION_READ_UNCOMMITTED</code>, <code>Connection.TRANSACTION_READ_COMMITTED
    *     </code>, <code>Connection.TRANSACTION_REPEATABLE_READ</code>, or <code>
    *     Connection.TRANSACTION_SERIALIZABLE</code>. (Note that <code>Connection.TRANSACTION_NONE
    *     </code> cannot be used because it specifies that transactions are not supported.)
    * @throws SQLException if a database access error occurs, this method is called on a closed
    *     connection or the given parameter is not one of the <code>Connection</code> constants
    * @see DatabaseMetaData#supportsTransactionIsolationLevel
    * @see #getTransactionIsolation
    */
  void MariaDbConnection::setTransactionIsolation(int32_t level) {
    try {
      stateFlag |=ConnectionState::STATE_TRANSACTION_ISOLATION;
      protocol->setTransactionIsolation(level);
    }
    catch (SQLException& e) {
      throw e;//ExceptionMapper.getException(e, this, NULL, false);
    }
  }
  /**
    * Retrieves the first warning reported by calls on this <code>Connection</code> object. If there
    * is more than one warning, subsequent warnings will be chained to the first one and can be
    * retrieved by calling the method <code>SQLWarning.getNextWarning</code> on the warning that was
    * retrieved previously.
    *
    * <p>This method may not be called on a closed connection; doing so will cause an <code>
    * SQLException</code> to be thrown.
    *
    * <p><B>Note:</B> Subsequent warnings will be chained to this SQLWarning.
    * <p><B>Note:</B> It's application's responsibility to delete warning objects
    * @return the first <code>SQLWarning</code> object or <code>null</code> if there are none
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     connection
    * @see SQLWarning
    */
  SQLWarning* MariaDbConnection::getWarnings()
  {
    if (warningsCleared || isClosed() || !protocol->hasWarnings())
    {
      return nullptr;
    }

    SQLWarning* last= nullptr;
    SQLWarning* first= nullptr;

    Unique::Statement st(this->createStatement());
    Unique::ResultSet rs(st->executeQuery("show warnings"));

    while (rs->next())
    {
      int32_t code= rs->getInt(2);
      SQLString message= rs->getString(3);

      SQLWarning* warning= new MariaDBWarning(message, "", code);

      if (first == nullptr)
      {
        first= warning;
        last= warning;
      }
      else
      {
        last->setNextWarning(warning);
        last= warning;
      }
    }

    return first;
  }

  /**
    * Clears all warnings reported for this <code>Connection</code> object. After a call to this
    * method, the method <code>getWarnings</code> returns <code>null</code> until a new warning is
    * reported for this <code>Connection</code> object.
    *
    * @throws SQLException SQLException if a database access error occurs or this method is called on
    *     a closed connection
    */
  void MariaDbConnection::clearWarnings() {
    if (this->isClosed()) {
      throw SQLException("Connection::clearWarnings cannot be called on a closed connection");//ExceptionMapper.getSqlException
    }
    warningsCleared= true;
  }

  /** Reenable warnings, when next statement is executed. */
  void MariaDbConnection::reenableWarnings() {
    warningsCleared= false;
  }
  /**
    * Retrieves the current holdability of <code>ResultSet</code> objects created using this <code>
    * Connection</code> object.
    *
    * @return the holdability, one of <code>ResultSet.HOLD_CURSORS_OVER_COMMIT</code> or <code>
    *     ResultSet.CLOSE_CURSORS_AT_COMMIT</code>
    * @see #setHoldability
    * @see DatabaseMetaData#getResultSetHoldability
    * @see ResultSet
    * @since 1.4
    */
  int32_t MariaDbConnection::getHoldability() {
    return ResultSet::HOLD_CURSORS_OVER_COMMIT;
  }

  void MariaDbConnection::setHoldability(int32_t holdability) {
  }

  /**
    * Creates an unnamed savepoint in the current transaction and returns the new <code>Savepoint
    * </code> object that represents it.
    *
    * <p>if setSavepoint is invoked outside of an active transaction, a transaction will be started
    * at this newly created savepoint.
    *
    * @return the new <code>Savepoint</code> object
    * @throws SQLException if a database access error occurs, this method is called while
    *     participating in a distributed transaction, this method is called on a closed connection or
    *     this <code>Connection</code> object is currently in auto-commit mode
    * @see Savepoint
    * @since 1.4
    */
  Savepoint* MariaDbConnection::setSavepoint() {
    return setSavepoint("unnamed");
  }

  /**
    * Creates a savepoint with the given name in the current transaction and returns the new <code>
    * Savepoint</code> object that represents it. if setSavepoint is invoked outside of an active
    * transaction, a transaction will be started at this newly created savepoint.
    *
    * @param name a <code>String</code> containing the name of the savepoint
    * @return the new <code>Savepoint</code> object
    * @throws SQLException if a database access error occurs, this method is called while
    *     participating in a distributed transaction, this method is called on a closed connection or
    *     this <code>Connection</code> object is currently in auto-commit mode
    * @see Savepoint
    * @since 1.4
    */
  Savepoint* MariaDbConnection::setSavepoint(const SQLString& name)
  {
    Savepoint* savepoint= new MariaDbSavepoint(name, savepointCount++);
    std::unique_ptr<Statement> st(createStatement());

    st->execute("SAVEPOINT "+savepoint->toString());

    return savepoint;
  }

  /**
    * Removes the specified <code>Savepoint</code> and subsequent <code>Savepoint</code> objects from
    * the current transaction. Any reference to the savepoint after it have been removed will cause
    * an <code>SQLException</code> to be thrown.
    *
    * @param savepoint the <code>Savepoint</code> object to be removed
    * @throws SQLException if a database access error occurs, this method is called on a closed
    *     connection or the given <code>Savepoint</code> object is not a valid savepoint in the
    *     current transaction
    */
  void MariaDbConnection::releaseSavepoint(const Savepoint* savepoint)
  {
    std::unique_ptr<Statement> st(createStatement());
    st->execute("RELEASE SAVEPOINT " + savepoint->toString());
  }


  sql::Connection* MariaDbConnection::setClientOption(const SQLString& name, void* value) {
    throw SQLFeatureNotImplementedException("setClientOption support is not implemented yet");
  }


  sql::Connection* MariaDbConnection::setClientOption(const SQLString& name, const SQLString& value) {
    throw SQLFeatureNotImplementedException("setClientOption support is not implemented yet");
  }


  void MariaDbConnection::getClientOption(const SQLString& n, void* v) {
    throw SQLFeatureNotSupportedException("getClientOption is not supported");
  }
  

  SQLString MariaDbConnection::getClientOption(const SQLString& n) {
    throw SQLFeatureNotSupportedException("getClientOption is not supported");
  }
  /**
    * Constructs an object that implements the <code>Clob</code> interface. The object returned
    * initially contains no data. The <code>setAsciiStream</code>, <code>setCharacterStream</code>
    * and <code>setString</code> methods of the <code>Clob</code> interface may be used to add data
    * to the <code>Clob</code>.
    *
    * @return An object that implements the <code>Clob</code> interface
    */
  Clob* MariaDbConnection::createClob()
  {
    throw SQLFeatureNotImplementedException("Clob's support is not implemented yet");
    //return new MariaDbClob();
  }
  /**
    * Constructs an object that implements the <code>Blob</code> interface. The object returned
    * initially contains no data. The <code>setBinaryStream</code> and <code>setBytes</code> methods
    * of the <code>Blob</code> interface may be used to add data to the <code>Blob</code>.
    *
    * @return An object that implements the <code>Blob</code> interface
    */
  Blob* MariaDbConnection::createBlob()
  {
    throw SQLFeatureNotImplementedException("Blob's support is not implemented yet");
    //return new MariaDbBlob();
  }
  /**
    * Constructs an object that implements the <code>NClob</code> interface. The object returned
    * initially contains no data. The <code>setAsciiStream</code>, <code>setCharacterStream</code>
    * and <code>setString</code> methods of the <code>NClob</code> interface may be used to add data
    * to the <code>NClob</code>.
    *
    * @return An object that implements the <code>NClob</code> interface
    */
  NClob* MariaDbConnection::createNClob() {
    throw SQLFeatureNotImplementedException("NClob's support is not implemented");
    //return new MariaDbClob();
  }

  sql::SQLXML* MariaDbConnection::createSQLXML()
  {
    throw SQLFeatureNotSupportedException("Not supported");//ExceptionMapper.getFeatureNotSupportedException("Not supported");
  }
  /**
    * Returns true if the connection has not been closed and is still valid. The driver shall submit
    * a query on the connection or use some other mechanism that positively verifies the connection
    * is still valid when this method is called.
    *
    * <p>The query submitted by the driver to validate the connection shall be executed in the
    * context of the current transaction.
    *
    * @param timeout - The time in seconds to wait for the database operation used to validate the
    *     connection to complete. If the timeout period expires before the operation completes, this
    *     method returns false. A value of 0 indicates a timeout is not applied to the database
    *     operation.
    * @return true if the connection is valid, false otherwise
    * @throws SQLException if the value supplied for <code>timeout</code> is less then 0
    * @see DatabaseMetaData#getClientInfoProperties
    * @since 1.6
    */
  bool MariaDbConnection::isValid(int32_t timeout)
  {
    if (timeout < 0) {
      throw SQLException("the value supplied for timeout is negative");
    }
    if (isClosed()) {
      return false;
    }
    try {
      return protocol->isValid(timeout *1000);
    }
    catch (SQLException&) {
      // eat
      return false;
    }
  }


  bool MariaDbConnection::isValid() {
    return protocol->ping();
  }

  void MariaDbConnection::checkClientClose(const SQLString& name)
  {
    if (protocol->isExplicitClosed())
    {
      std::map<SQLString, ClientInfoStatus> failures;
      failures.insert({ name, ClientInfoStatus::_REASON_UNKNOWN });
      throw SQLException("setClientInfo() is called on closed connection");// SQLClientInfoException("setClientInfo() is called on closed connection", failures);
    }
  }


  void MariaDbConnection::checkClientReconnect(const SQLString& name)
  {
    if (protocol->isClosed()) {
      if (protocol->getProxy() != nullptr) {

        std::lock_guard<std::mutex> localScopeLock(*lock);
        try
        {
          protocol->getProxy()->reconnect();
        }
        catch (SQLException& /*sqle*/)
        {
          std::map<SQLString, ClientInfoStatus>failures;
          failures.insert({ name, ClientInfoStatus::_REASON_UNKNOWN });
          throw SQLException("ClientInfoException: Connection closed");// SQLClientInfoException("Connection* closed", failures, sqle);
        }
      }
      else {
        protocol->reconnect();
      }
    }
  }


  void MariaDbConnection::checkClientValidProperty(const SQLString& name) {
    if (name.empty()
      ||(!(name.compare("ApplicationName") == 0)
        &&!(name.compare("ClientUser") == 0)
        &&!(name.compare("ClientHostname") == 0)))
    {
      std::map<SQLString, ClientInfoStatus>failures;
      failures.insert({ name, ClientInfoStatus::REASON_UNKNOWN_PROPERTY });
      throw //SQLClientInfoException(
        SQLException("setClientInfo() parameters can only be \"ApplicationName\",\"ClientUser\" or \"ClientHostname\", "
          "but was : " +name);//, failures);
    }
  }
  SQLString MariaDbConnection::buildClientQuery(const SQLString& name, const SQLString& value)
  {
    SQLString escapeQuery("SET @");

    escapeQuery.append(name).append("=");

    if (value.empty()) {
      escapeQuery.append("NULL");
    }
    else
    {
      escapeQuery.append("'");
      std::size_t charsOffset= 0;
      std::size_t charsLength= value.length();
      char charValue;

      if (protocol->noBackslashEscapes())
      {
        while (charsOffset < charsLength)
        {
          charValue= value.at(charsOffset);
          if (charValue == '\'') {
            escapeQuery.append('\'');
          }
          escapeQuery.append(charValue);
          charsOffset++;
        }
      }
      else {
        while (charsOffset < charsLength) {
          charValue= value.at(charsOffset);
          if (charValue =='\''||charValue =='\\'||charValue =='"'||charValue ==0) {
            escapeQuery.append('\\');
          }
          escapeQuery.append(charValue);
          charsOffset++;
        }
      }
      escapeQuery.append("'");
    }
    return escapeQuery;
  }
  /**
    * Sets the value of the client info property specified by name to the value specified by value.
    *
    * <p>Applications may use the <code>DatabaseMetaData.getClientInfoProperties</code> method to
    * determine the client info properties supported by the driver and the maximum length that may be
    * specified for each property.
    *
    * <p>The driver stores the value specified in a suitable location in the database. For example in
    * a special register, session parameter, or system table column. For efficiency the driver may
    * defer setting the value in the database until the next time a statement is executed or
    * prepared. Other than storing the client information in the appropriate place in the database,
    * these methods shall not alter the behavior of the connection in anyway. The values supplied to
    * these methods are used for accounting, diagnostics and debugging purposes only.
    *
    * <p>The driver shall generate a warning if the client info name specified is not recognized by
    * the driver.
    *
    * <p>If the value specified to this method is greater than the maximum length for the property
    * the driver may either truncate the value and generate a warning or generate a <code>
    * SQLClientInfoException</code>. If the driver generates a <code>SQLClientInfoException</code>,
    * the value specified was not set on the connection.
    *
    * <p>The following are standard client info properties. Drivers are not required to support these
    * properties however if the driver supports a client info property that can be described by one
    * of the standard properties, the standard property name should be used.
    *
    * <ul>
    *   <li>ApplicationName - The name of the application currently utilizing the connection
    *   <li>ClientUser - The name of the user that the application using the connection is performing
    *       work for. This may not be the same as the user name that was used in establishing the
    *       connection.
    *   <li>ClientHostname - The hostname of the computer the application using the connection is
    *       running on.
    * </ul>
    *
    * @param name The name of the client info property to set
    * @param value The value to set the client info property to. If the value is null, the current
    *     value of the specified property is cleared.
    * @throws SQLClientInfoException if the database server returns an error while setting the client
    *     info value on the database server or this method is called on a closed connection
    * @since 1.6
    */
  void MariaDbConnection::setClientInfo(const SQLString& name, const SQLString& value)
  {
    checkClientClose(name);
    checkClientReconnect(name);
    checkClientValidProperty(name);

    try {
      Unique::Statement statement(createStatement());
      statement->execute(buildClientQuery(name, value));
    }
    catch (SQLException &/*sqle*/)
    {
      std::map<SQLString, ClientInfoStatus> failures;
      failures.insert({ name, ClientInfoStatus::_REASON_UNKNOWN });
      throw //SQLClientInfoException
        SQLException("unexpected error during setClientInfo");// , failures, sqle);
    }
  }
  /**
    * Sets the value of the connection's client info properties. The <code>Properties</code> object
    * contains the names and values of the client info properties to be set. The set of client info
    * properties contained in the properties list replaces the current set of client info properties
    * on the connection. If a property that is currently set on the connection is not present in the
    * properties list, that property is cleared. Specifying an empty properties list will clear all
    * of the properties on the connection. See <code>setClientInfo (String, String)</code> for more
    * information.
    *
    * <p>If an error occurs in setting any of the client info properties, a <code>
    * SQLClientInfoException</code> is thrown. The <code>SQLClientInfoException</code> contains
    * information indicating which client info properties were not set. The state of the client
    * information is unknown because some databases do not allow multiple client info properties to
    * be set atomically. For those databases, one or more properties may have been set before the
    * error occurred.
    *
    * @param properties the list of client info properties to set
    * @throws SQLClientInfoException if the database server returns an error while setting the
    *     clientInfo values on the database server or this method is called on a closed connection
    * @see Connection#setClientInfo(String, String) setClientInfo(String, String)
    * @since 1.6
    */
  void MariaDbConnection::setClientInfo(const Properties& properties)
  {
    std::map<SQLString, ClientInfoStatus>propertiesExceptions;

    for (SQLString name : { "ApplicationName","ClientUser","ClientHostname" })
    {
      try {
        auto cit= properties.find(name);
        setClientInfo(name, cit != properties.cend() ? cit->second : "");
      }
      catch (SQLException& /*e*/) {
#ifdef MAYBE_IN_NEXTVERSION
        propertiesExceptions.putAll(e.getFailedProperties());
#endif
      }
    }
    if (!propertiesExceptions.empty()) {
      SQLString errorMsg("setClientInfo errors : the following properties where not set : ");
#ifdef MAYBE_IN_NEXTVERSION
        +propertiesExceptions.keySet();
#endif
      throw SQLException("ClientInfoException: " + errorMsg);//SQLClientInfoException(errorMsg, propertiesExceptions);
    }
  }

  /**
    * Returns a list containing the name and current value of each client info property supported by
    * the driver. The value of a client info property may be null if the property has not been set
    * and does not have a default value.
    *
    * @return A <code>Properties</code> object that contains the name and current value of each of
    *     the client info properties supported by the driver.
    * @throws SQLException if the database server returns an error when fetching the client info
    *     values from the database or this method is called on a closed connection
    */
  Properties MariaDbConnection::getClientInfo()
  {
    checkConnection();
    Properties properties;

    std::unique_ptr<Statement> statement(createStatement());
    {
      std::unique_ptr<ResultSet> rs(statement->executeQuery("SELECT @ApplicationName, @ClientUser, @ClientHostname"));
      {
        if (rs->next())
        {
          /* I'd imagine we would need to test here if the field is NULL*/
          if (!rs->getString(1).empty()) {
            properties.insert({ "ApplicationName", rs->getString(1) });
          }
          if (!rs->getString(2).empty()) {
            properties.insert({ "ClientUser", rs->getString(2) });
          }
          if (!rs->getString(3).empty()) {
            properties.insert({ "ClientHostname", rs->getString(3) });
          }
          return properties;
        }
      }
    }

    properties.emplace("ApplicationName", "");
    properties.emplace("ClientUser", "");
    properties.emplace("ClientHostname", "");

    return properties;
  }

  /**
    * Returns the value of the client info property specified by name. This method may return null if
    * the specified client info property has not been set and does not have a default value. This
    * method will also return null if the specified client info property name is not supported by the
    * driver. Applications may use the <code>DatabaseMetaData.getClientInfoProperties</code> method
    * to determine the client info properties supported by the driver.
    *
    * @param name The name of the client info property to retrieve
    * @return The value of the client info property specified
    * @throws SQLException if the database server returns an error when fetching the client info
    *     value from the database or this method is called on a closed connection
    * @see DatabaseMetaData#getClientInfoProperties
    * @since 1.6
    */
  SQLString MariaDbConnection::getClientInfo(const SQLString& name)
  {
    checkConnection();
    if (!(name.compare("ApplicationName") == 0)
      &&!(name.compare("ClientUser") == 0)
      &&!(name.compare("ClientHostname") == 0)) {
      throw SQLException(
        "name must be \"ApplicationName\", \"ClientUser\" or \"ClientHostname\", but was \""
        +name
        +"\"");
    }
    std::unique_ptr<Statement> statement(createStatement()); {
      std::unique_ptr<ResultSet> rs(statement->executeQuery("SELECT @"+name)); {
        if (rs->next()) {
          return rs->getString(1);
        }
      }
    }
    return nullptr;
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /**
    * Factory method for creating Array objects. <b>Note: </b>When <code>createArrayOf</code> is used
    * to create an array object that maps to a primitive data type, then it is implementation-defined
    * whether the <code>Array</code> object is an array of that primitive data type or an array of
    * <code>Object</code>. <b>Note: </b>The JDBC driver is responsible for mapping the elements
    * <code>Object</code> array to the default JDBC SQL type defined in java.sql.Types for the given
    * class of <code>Object</code>. The default mapping is specified in Appendix B of the JDBC
    * specification. If the resulting JDBC type is not the appropriate type for the given typeName
    * then it is implementation defined whether an <code>SQLException</code> is thrown or the driver
    * supports the resulting conversion.
    *
    * @param typeName the SQL name of the type the elements of the array map to. The typeName is a
    *     database-specific name which may be the name of a built-in type, a user-defined type or a
    *     standard SQL type supported by this database. This is the value returned by <code>
    *     Array.getBaseTypeName</code>
    * @param elements the elements that populate the returned object
    * @return an Array object whose elements map to the specified SQL type
    * @throws SQLException if a database error occurs, the JDBC type is not appropriate for the
    *     typeName and the conversion is not supported, the typeName is null or this method is called
    *     on a closed connection
    */
  sql::Array* MariaDbConnection::createArrayOf(const SQLString& typeName, const sql::Object* elements)
  {
    throw SQLFeatureNotSupportedException("At the moment is not planned to support");
  }

  /**
    * Factory method for creating Struct objects.
    *
    * @param typeName the SQL type name of the SQL structured type that this <code>Struct</code>
    *     object maps to. The typeName is the name of a user-defined type that has been defined for
    *     this database. It is the value returned by <code>Struct.getSQLTypeName</code>.
    * @param attributes the attributes that populate the returned object
    * @return a Struct object that maps to the given SQL type and is populated with the given
    *     attributes
    * @throws SQLException if a database error occurs, the typeName is null or this method is called
    *     on a closed connection
    */
  sql::Struct* MariaDbConnection::createStruct(const SQLString& typeName, const sql::Object* attributes)
  {
    throw SQLFeatureNotSupportedException("At the moment the feature is not planned to be supported");
  }
#endif

  /**
    * Returns an object that implements the given interface to allow access to non-standard methods,
    * or standard methods not exposed by the proxy. If the receiver implements the interface then the
    * result is the receiver or a proxy for the receiver. If the receiver is a wrapper and the
    * wrapped object implements the interface then the result is the wrapped object or a proxy for
    * the wrapped object. Otherwise return the the result of calling <code>unwrap</code> recursively
    * on the wrapped object or a proxy for that result. If the receiver is not a wrapper and does not
    * implement the interface, then an <code>SQLException</code> is thrown.
    *
    * @param iface A Class defining an interface that the result must implement.
    * @return an object that implements the interface. May be a proxy for the actual implementing
    *     object.
    * @throws SQLException If no object found that implements the interface
    * @since 1.6
    */
#ifdef WE_NEED_IT_AND_HAVE_FOUND_THE_WAY_TO_IMPLEMENT_IT
  template <class T>T unwrap() {
    try {
      if (isWrapperFor(iface)) {
        return iface.cast(this);
      }
      else {
        throw SQLException("The receiver is not a wrapper for "+iface.getName());
      }
    }
    catch (Exception e) {
      throw SQLException("The receiver is not a wrapper and does not implement the interface");
    }
  }

  /**
    * Returns true if this either implements the interface argument or is directly or indirectly a
    * wrapper for an object that does. Returns false otherwise. If this implements the interface then
    * return true, else if this is a wrapper then return the result of recursively calling <code>
    * isWrapperFor</code> on the wrapped object. If this does not implement the interface and is not
    * a wrapper, return false. This method should be implemented as a low-cost operation compared to
    * <code>unwrap</code> so that callers can use this method to avoid expensive <code>unwrap</code>
    * calls that may fail. If this method returns true then calling <code>unwrap</code> with the same
    * argument should succeed.
    *
    * @param iface a Class defining an interface.
    * @return true if this implements the interface or directly or indirectly wraps an object that
    *     does.
    * @since 1.6
    */

  bool MariaDbConnection::isWrapperFor() {
    return iface.isInstance(this);
  }
#endif

  SQLString MariaDbConnection::getUsername() {
    return protocol->getUsername();
  }


  SQLString MariaDbConnection::getHostname() {
    return protocol->getHost();
  }


  int32_t MariaDbConnection::getPort() {
    return protocol->getPort();
  }


  bool MariaDbConnection::getPinGlobalTxToPhysicalConnection()
  {
    return protocol->getPinGlobalTxToPhysicalConnection();
  }

  /** If failover is not activated, will close connection when a connection error occur. */
  void MariaDbConnection::setHostFailed()
  {
    if (protocol->getProxy() == NULL) {
      protocol->setHostFailedWithoutProxy();
    }
  }

  /**
    * Are table case sensitive or not . Default Value: 0 (Unix), 1 (Windows), 2 (Mac OS X). If set to
    * 0 (the default on Unix-based systems), table names and aliases and database names are compared
    * in a case-sensitive manner. If set to 1 (the default on Windows), names are stored in lowercase
    * and not compared in a case-sensitive manner. If set to 2 (the default on Mac OS X), names are
    * stored as declared, but compared in lowercase.
    *
    * @return int value.
    * @throws SQLException if a connection error occur
    */
  int32_t MariaDbConnection::getLowercaseTableNames() {
    if (lowercaseTableNames == -1) {
      std::unique_ptr<Statement> st(createStatement()); {
        std::unique_ptr<ResultSet> rs(st->executeQuery("select @@lower_case_table_names")); {
          rs->next();
          lowercaseTableNames= rs->getInt(1);
        }
      }
    }
    return lowercaseTableNames;
  }

  /**
    * Abort connection.
    *
    * @param executor executor
    * @throws SQLException if security manager doesn't permit it.
    */
  void MariaDbConnection::abort(sql::Executor* executor)
  {
    if (this->isClosed()) {
      return;
    }
    throw SQLFeatureNotSupportedException("Connection abort is not supported yet");

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
    SQLPermission sqlPermission= new SQLPermission("callAbort");
    SecurityManager securityManager= System.getSecurityManager();
    if (securityManager != nullptr) {
      securityManager.checkPermission(sqlPermission);
    }
    if (executor == nullptr) {
      throw ExceptionMapper.getSqlException("Cannot abort the connection: NULL executor passed");
    }
    executor.execute(protocol->abort());
#endif
  }

  /**
    * Get network timeout.
    *
    * @return timeout
    * @throws SQLException if database socket error occur
    */
  int32_t MariaDbConnection::getNetworkTimeout() {
    return this->protocol->getTimeout();
  }


  SQLString MariaDbConnection::getSchema() {
    return protocol->getCatalog();
  }


  void MariaDbConnection::setSchema(const SQLString& schema) {
    if (schema.empty()) {
      throw SQLException("The catalog name may not be empty", "XAE05");
    }
    try {
      stateFlag |= ConnectionState::STATE_DATABASE;
      protocol->setCatalog(schema);
    }
    catch (SQLException &e) {
      throw e;// ExceptionMapper.getException(e, this, NULL, false);
    }
  }

  /**
    * Change network timeout.
    *
    * @param executor executor (can be null)
    * @param milliseconds network timeout in milliseconds.
    * @throws SQLException if security manager doesn't permit it.
    */
  void MariaDbConnection::setNetworkTimeout(Executor* executor, int32_t milliseconds)
  {
    throw SQLFeatureNotImplementedException("setNetworkTimeout is not yet implemented");

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
    if (this->isClosed()) {
      throw SQLException("Connection::setNetworkTimeout cannot be called on a closed connection");//ExceptionMapper.getSqlException
    }

    if (milliseconds <0) {
      throw ExceptionMapper.getSqlException(
        "Connection::setNetworkTimeout cannot be called with a negative timeout");
    }
    SQLPermission sqlPermission= new SQLPermission("setNetworkTimeout");
    SecurityManager securityManager= System.getSecurityManager();
    if (securityManager != nullptr) {
      securityManager.checkPermission(sqlPermission);
    }
    try {
      stateFlag|= ConnectionState::STATE_NETWORK_TIMEOUT;
      protocol->setTimeout(milliseconds);
    }
    catch (SocketException& se) {
      throw ExceptionMapper.getSqlException("Cannot set the network timeout", se);
    }
#endif
  }


  int64_t MariaDbConnection::getServerThreadId() {
    return protocol->getServerThreadId();
  }


  bool MariaDbConnection::canUseServerTimeout() {
    return _canUseServerTimeout;
  }


  void MariaDbConnection::setDefaultTransactionIsolation(int32_t defaultTransactionIsolation) {
    this->defaultTransactionIsolation= defaultTransactionIsolation;
  }

  /**
    * Reset connection set has it was after creating a "fresh" new connection.
    * defaultTransactionIsolation must have been initialized.
    *
    * <p>BUT : - session variable state are reset only if option useResetConnection is set and - if
    * using the option "useServerPrepStmts", PREPARE statement are still prepared
    *
    * @throws SQLException if resetting operation failed
    */
  void MariaDbConnection::reset()
  {
    bool useComReset=
      options->useResetConnection
      && ((protocol->isServerMariaDb() && protocol->versionGreaterOrEqual(10, 2, 4))
        || (!protocol->isServerMariaDb() && protocol->versionGreaterOrEqual(5, 7, 3)));
    if (useComReset) {
      protocol->reset();
    }
    if (stateFlag !=0) {
      try {
        if ((stateFlag & ConnectionState::STATE_NETWORK_TIMEOUT) != 0) {
          setNetworkTimeout(nullptr, options->socketTimeout);
        }
        if ((stateFlag & ConnectionState::STATE_AUTOCOMMIT) != 0) {
          setAutoCommit(options->autocommit);
        }
        if ((stateFlag & ConnectionState::STATE_DATABASE) != 0) {
          protocol->resetDatabase();
        }
        if ((stateFlag & ConnectionState::STATE_READ_ONLY) != 0) {
          setReadOnly(false);
        }
        if (!useComReset && (stateFlag & ConnectionState::STATE_TRANSACTION_ISOLATION) != 0) {
          setTransactionIsolation(defaultTransactionIsolation);
        }
        stateFlag= 0;
      }
      catch (SQLException&) {
        throw SQLException("Error resetting connection");
      }
    }
    warningsCleared= true;
  }

  bool MariaDbConnection::reconnect() {
    checkClientReconnect("reconnect");
    // checkClientReconnect would throw, if reconnect was not successful
    return true;
  }


  bool MariaDbConnection::includeDeadLockInfo() {
    return options->includeInnodbStatusInDeadlockExceptions;
  }


  bool MariaDbConnection::includeThreadsTraces() {
    return options->includeThreadDumpInDeadlockExceptions;
  }

  CallableParameterMetaData* MariaDbConnection::getInternalParameterMetaData(const SQLString& procedureName, const SQLString& databaseName, bool isFunction)
  {
    SQLString sql("SELECT * from INFORMATION_SCHEMA.PARAMETERS WHERE SPECIFIC_NAME=? AND SPECIFIC_SCHEMA=");
    sql.append(!databaseName.empty() ? "?" : "DATABASE()");
    sql.append(" ORDER BY ORDINAL_POSITION");
    std::unique_ptr<PreparedStatement> preparedStatement(this->prepareStatement(sql));

    preparedStatement->setString(1, procedureName);
    if (!databaseName.empty()) {
      preparedStatement->setString(2, databaseName);
    }

    return new CallableParameterMetaData(preparedStatement->executeQuery(), isFunction);
  }
}
}
