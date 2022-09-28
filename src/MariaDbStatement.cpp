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


#include "MariaDbStatement.h"

#include "SqlStates.h"
#include "ExceptionFactory.h"
#include "util/Utils.h"
#include "Results.h"

namespace sql
{
namespace mariadb
{
  //\\u0080-\\uFFFF "[\u0000'\"\b\n\r\t\u001A\\\\]"
  std::regex MariaDbStatement::identifierPattern("[0-9a-zA-Z\\$_]*", std::regex_constants::ECMAScript);
  //| Pattern.UNICODE_CASE |Pattern.CANON_EQ);
  std::regex MariaDbStatement::escapePattern("['\"\b\n\r\t\\\\]", std::regex_constants::ECMAScript );
  const std::map<std::string, std::string> MariaDbStatement::mapper= {
    {"\u0000", "\\0"},
    {"'",      "\\\\'"},
    {"\"",     "\\\\\""},
    {"\b",     "\\\\b"},
    {"\n",     "\\\\n"},
    {"\r",     "\\\\r"},
    {"\t",     "\\\\t"},
    {"\u001A", "\\\\Z"},
    {"\\",     "\\\\"},
  };

  Shared::Logger MariaDbStatement::logger= LoggerFactory::getLogger(typeid(MariaDbStatement));
  /**
   * Creates a new Statement.
   *
   * @param connection the connection to return in getConnection.
   * @param _resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
   *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
   *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
   * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
   *     or <code>ResultSet.CONCUR_UPDATABLE</code>
   */
  MariaDbStatement::MariaDbStatement(
    MariaDbConnection* _connection,
    int32_t _resultSetScrollType,
    int32_t _resultSetConcurrency,
    Shared::ExceptionFactory& factory)
    : connection(_connection),
      protocol(_connection->getProtocol()),
      lock(_connection->lock),
      resultSetScrollType(_resultSetScrollType),
      resultSetConcurrency(_resultSetConcurrency),
      options(protocol->getOptions()),
      canUseServerTimeout(_connection->canUseServerTimeout()),
      fetchSize(options->defaultFetchSize),
      closed(false),
      mustCloseOnCompletion(false),
      warningsCleared(true),
      maxRows(0),
      maxFieldSize(0),
      exceptionFactory(factory),
      isTimedout(false),
      queryTimeout(0),
      executing(false),
      batchRes(0),
      largeBatchRes(0)
  {
  }


  Statement* MariaDbStatement::setResultSetType(int32_t rsType)
  {
    resultSetScrollType= rsType;
    return this;
  }

  /**
   * Clone statement.
   *
   * @param connection connection
   * @return Clone statement.
   * @throws CloneNotSupportedException if any error occur.
   */
  MariaDbStatement* MariaDbStatement::clone(MariaDbConnection* connection)
  {
    Shared::ExceptionFactory ef(ExceptionFactory::of(this->exceptionFactory->getThreadId(), this->exceptionFactory->getOptions()));
    MariaDbStatement* clone= new MariaDbStatement(connection, this->resultSetScrollType, this->resultSetConcurrency, ef);
    clone->fetchSize= this->options->defaultFetchSize;

    return clone;
  }

  MariaDbStatement::~MariaDbStatement()
  {
  }

  // Part of query prolog - setup timeout timer
  void MariaDbStatement::setTimerTask(bool isBatch)
  {
#ifdef MAYBE_IN_NEXTVERSION
    assert(!timerTaskFuture);
    if (!timeoutScheduler)
    {
      timeoutScheduler= SchedulerServiceProviderHolder::getTimeoutScheduler();
    }
    timerTaskFuture =
      timeoutScheduler.schedule(
          ()->{
          try {
          isTimedout= true;
          if (!isBatch){
          protocol->cancelCurrentQuery();
          }
          protocol->interrupt();
          }catch (std::exception& e){

          }
          },
          queryTimeout,
          TimeUnit::SECONDS);
#endif
  }

  /**
   * Command prolog.
   *
   * <ol>
   *   <li>clear previous query state
   *   <li>launch timeout timer if needed
   * </ol>
   *
   * @param isBatch is batch
   * @throws SQLException if statement is closed
   */
  void MariaDbStatement::executeQueryPrologue(bool isBatch) {
    setExecutingFlag();
    if (closed){
      exceptionFactory->raiseStatementError(connection, this)->create("execute() is called on closed statement").Throw();
    }
    protocol->prolog(maxRows, protocol->getProxy(), connection, this);
    if (queryTimeout != 0 &&(!canUseServerTimeout ||isBatch)){
      setTimerTask(isBatch);
    }
  }


  void MariaDbStatement::stopTimeoutTask()
  {
#ifdef MAYBE_IN_NEXTVERSION
    if (timerTaskFuture){
      if (!timerTaskFuture.cancel(true)){


        try {
          timerTaskFuture.get();
        }catch (InterruptedException& e){

          Thread.currentThread().interrupt();
        }catch (InterruptedException& e){

        }
      }
      timerTaskFuture= NULL;
    }
#endif
  }

  /**
   * Reset timeout after query, re-throw SQL exception.
   *
   * @param sqle current exception
   * @return SQLException exception with new message in case of timer timeout.
   */
  MariaDBExceptionThrower MariaDbStatement::executeExceptionEpilogue(SQLException& sqle)
  {
    if (!sqle.getSQLState().empty() && sqle.getSQLState().startsWith("08")){
      try {
        close();
      }catch (SQLException& ){

      }
    }

    if (sqle.getErrorCode()==1148 &&!options->allowLocalInfile){
      return exceptionFactory->raiseStatementError(connection, this)->create(
          "Usage of LOCAL INFILE is disabled. "
          "To use it enable it via the connection property allowLocalInfile=true",
          "42000",
          1148,
          &sqle);
    }

    if (isTimedout){
      return exceptionFactory->raiseStatementError(connection, this)->create("Query timed out", "70100", 1317, &sqle);
    }
    MariaDBExceptionThrower sqlException= exceptionFactory->raiseStatementError(connection, this)->create(sqle);
    logger->error("error executing query", sqlException);

    return sqlException;
  }


  void MariaDbStatement::executeEpilogue()
  {
    stopTimeoutTask();
    isTimedout= false;
    setExecutingFlag(false);
  }

  void MariaDbStatement::executeBatchEpilogue(){
    setExecutingFlag(false);
    stopTimeoutTask();
    isTimedout= false;
    clearBatch();
  }

  MariaDBExceptionThrower MariaDbStatement::handleFailoverAndTimeout(SQLException& sqle)
  {
    if (sqle.getSQLState().empty() != true && sqle.getSQLState().startsWith("08")){
      try {
        close();
      }catch (SQLException&){

      }
    }

    if (isTimedout){
      return exceptionFactory->raiseStatementError(connection, this)->create("Query timed out", "70100", 1317, &sqle);
    }
    MariaDBExceptionThrower exThrower;
    exThrower.take<SQLException>(sqle);
    return exThrower;
  }


  BatchUpdateException MariaDbStatement::executeBatchExceptionEpilogue(SQLException& initialSqle, std::size_t size)
  {
    MariaDBExceptionThrower sqle(handleFailoverAndTimeout(initialSqle));

    /* It does not make any sense to fill these arrays really - application won't see it since the exception will be thrown */
    if (!results || !results->commandEnd())
    {
      batchRes.reserve(size);
      /* Thechnically it can't be harmful - the worst that can happen, is that next time reserve() called, it might think that
         current array is not big enough, and delete and allocate the new one */
      batchRes.length= size;
      for (int32_t* p = batchRes.begin(); p < batchRes.end(); ++p) *p= static_cast<int32_t>(Statement::EXECUTE_FAILED);
    }
    else
    {
      batchRes.wrap(results->getCmdInformation()->getUpdateCounts());
    }

    MariaDBExceptionThrower sqle2= exceptionFactory->raiseStatementError(connection, this)->create(*sqle.getException());
    logger->error("error executing query", sqle2);

    return BatchUpdateException(sqle2.getException()->getMessage(), sqle2.getException()->getSQLState(), sqle2.getException()->getErrorCode());//, ret, &sqle2); //MAYBE_IN_NEXTVERSION
  }

  /**
   * Executes a query.
   *
   * @param sql the query
   * @param fetchSize fetch size
   * @param autoGeneratedKeys a flag indicating whether auto-generated keys should be returned; one
   *     of <code>Statement::RETURN_GENERATED_KEYS</code> or <code>Statement::NO_GENERATED_KEYS</code>
   * @return true if there was a result set, false otherwise.
   * @throws SQLException the error description
   */
  bool MariaDbStatement::executeInternal(const SQLString& sql, int32_t fetchSize, int32_t autoGeneratedKeys)
  {
    std::unique_lock<std::mutex> localScopeLock(*lock);

    try {
      std::vector<Shared::ParameterHolder> dummy;
      executeQueryPrologue(false);
      results.reset(
        new Results(
            this,
            fetchSize,
            false,
            1,
            false,
            resultSetScrollType,
            resultSetConcurrency,
            autoGeneratedKeys,
            protocol->getAutoIncrementIncrement(),
            sql,
            dummy));

      protocol->executeQuery(protocol->isMasterConnection(), results, getTimeoutSql(Utils::nativeSql(sql, protocol.get())));

      results->commandEnd();
      executeEpilogue();
      return results->getResultSet() != nullptr;
    }
    catch (SQLException& exception)
    {
      executeEpilogue();
      localScopeLock.unlock();
      if (exception.getSQLState().compare("70100") == 0 && 1927 == exception.getErrorCode() || 2013 == exception.getErrorCode()) {
        
        protocol->handleIoException(exception, true);
      }
      executeExceptionEpilogue(exception).Throw();
    }
    return false;
  }

  /**
   * Enquote String value.
   *
   * @param val string value to enquote
   * @return enquoted string value
   * @throws SQLException -not possible-
   */
  SQLString MariaDbStatement::enquoteLiteral(const SQLString& val)
  {
    std::smatch matcher;
    SQLString escapedVal("'");
    std::string Value(StringImp::get(val));

    while (std::regex_search(Value, matcher, escapePattern))
    {
      escapedVal.append(matcher.prefix().str());
      auto cit= mapper.find(matcher.str());
      escapedVal.append(cit->second);
      Value= matcher.suffix().str();
    }
    /* Appending last suffix where has been nothing left */
    escapedVal.append(Value);
    escapedVal.append("'");
    return escapedVal;
  }

  /**
   * Escaped identifier according to MariaDB requirement.
   *
   * @param identifier identifier
   * @param alwaysQuote indicate if identifier must be enquoted even if not necessary.
   * @return return escaped identifier, quoted when necessary or indicated with alwaysQuote.
   * @see <a href="https://mariadb.com/kb/en/library/identifier-names/">mariadb identifier name</a>
   * @throws SQLException if containing u0000 character
   */
  SQLString MariaDbStatement::enquoteIdentifier(const SQLString& identifier, bool alwaysQuote)
  {
    if (isSimpleIdentifier(identifier))
    {
      return alwaysQuote ? "`"+identifier +"`":identifier;
    }
    else
    {
      if ((identifier.find_first_of("\u0000") != std::string::npos)){
        exceptionFactory->raiseStatementError(connection, this)->create("Invalid name - containing u0000 character", "42000").Throw();
      }

      std::string result(StringImp::get(identifier));
      std::regex rx("^`.+`$");

      if (std::regex_search(result, rx))
      {
        result= result.substr(1, result.size()-1);
      }
      return "`"+replace(result, "`", "``")+"`";
    }
  }

  /**
   * Retrieves whether identifier is a simple SQL identifier. The first character is an alphabetic
   * character from a through z, or from A through Z The string only contains alphanumeric
   * characters or the characters "_" and "$"
   *
   * @param identifier identifier
   * @return true if identifier doesn't have to be quoted
   * @see <a href="https://mariadb.com/kb/en/library/identifier-names/">mariadb identifier name</a>
   * @throws SQLException exception
   */
  bool MariaDbStatement::isSimpleIdentifier(const SQLString& identifier)
  {
    return !identifier.empty() && std::regex_search(StringImp::get(identifier), identifierPattern);
  }

  /**
   * Enquote utf8 value.
   *
   * @param val value to enquote
   * @return enquoted String value
   * @throws SQLException - not possible -
   */
  SQLString MariaDbStatement::enquoteNCharLiteral(const SQLString& val) {
    return "N'"+replace(val, "'", "''")+"'";
  }


  SQLString MariaDbStatement::getTimeoutSql(const SQLString& sql)
  {
    if (queryTimeout > 0 && canUseServerTimeout){
      return "SET STATEMENT max_statement_time="+ std::to_string(queryTimeout) +" FOR "+ sql;
    }
    return sql;
  }

  /**
   * ! This method is for test only ! This permit sending query using specific charset.
   *
   * @param sql sql
   * @param charset charset
   * @return boolean if execution went well
   * @throws SQLException if any exception occur
   */
  bool MariaDbStatement::testExecute(const SQLString& sql, const Charset& charset)
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {
      std::vector<Shared::ParameterHolder> dummy;
      executeQueryPrologue(false);
      results.reset(new Results(
            this,
            fetchSize,
            false,
            1,
            false,
            resultSetScrollType,
            resultSetConcurrency,
            Statement::NO_GENERATED_KEYS,
            protocol->getAutoIncrementIncrement(),
            sql,
            dummy));

      protocol->executeQuery(
          protocol->isMasterConnection(),
          results,
          getTimeoutSql(Utils::nativeSql(sql, protocol.get())),
          &charset);

      results->commandEnd();
      executeEpilogue();
      return results->releaseResultSet();

    }
    catch (SQLException& exception){
      executeEpilogue();
      executeExceptionEpilogue(exception).Throw();
    }
    //To please compilers etc
    return false;
  }

  /**
   * executes a query.
   *
   * @param sql the query
   * @return true if there was a result set, false otherwise.
   * @throws SQLException if the query could not be sent to server
   */
  bool MariaDbStatement::execute(const SQLString& sql) {
    return executeInternal(sql, fetchSize, Statement::NO_GENERATED_KEYS);
  }

  /**
   * Executes the given SQL statement, which may return multiple results, and signals the driver
   * that any auto-generated keys should be made available for retrieval. The driver will ignore
   * this signal if the SQL statement is not an <code>INSERT</code> statement, or an SQL statement
   * able to return auto-generated keys (the list of such statements is vendor-specific).
   *
   * <p>In some (uncommon) situations, a single SQL statement may return multiple result sets and/or
   * update counts. Normally you can ignore this unless you are (1) executing a stored procedure
   * that you know may return multiple results or (2) you are dynamically executing an unknown SQL
   * string. The <code>execute</code> method executes an SQL statement and indicates the form of the
   * first result. You must then use the methods <code>getResultSet</code> or <code>getUpdateCount
   * </code> to retrieve the result, and <code>getInternalMoreResults</code> to move to any
   * subsequent result(s).
   *
   * @param sql any SQL statement
   * @param autoGeneratedKeys a constant indicating whether auto-generated keys should be made
   *     available for retrieval using the method<code>getGeneratedKeys</code>; one of the following
   *     constants: <code>Statement.RETURN_GENERATED_KEYS</code> or <code>
   *     Statement.NO_GENERATED_KEYS</code>
   * @return <code>true</code> if the first result is a <code>ResultSet</code> object; <code>false
   *     </code> if it is an update count or there are no results
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the second parameter supplied to this method is not <code>
   *     Statement.RETURN_GENERATED_KEYS</code> or <code>Statement.NO_GENERATED_KEYS</code>.
   * @see #getResultSet
   * @see #getUpdateCount
   * @see #getMoreResults
   * @see #getGeneratedKeys
   */
  bool MariaDbStatement::execute(const SQLString& sql, int32_t autoGeneratedKeys) {
    return executeInternal(sql,fetchSize,autoGeneratedKeys);
  }

  /**
   * Executes the given SQL statement, which may return multiple results, and signals the driver
   * that the auto-generated keys indicated in the given array should be made available for
   * retrieval. This array contains the indexes of the columns in the target table that contain the
   * auto-generated keys that should be made available. The driver will ignore the array if the SQL
   * statement is not an <code>INSERT</code> statement, or an SQL statement able to return
   * auto-generated keys (the list of such statements is vendor-specific).
   *
   * <p>Under some (uncommon) situations, a single SQL statement may return multiple result sets
   * and/or update counts. Normally you can ignore this unless you are (1) executing a stored
   * procedure that you know may return multiple results or (2) you are dynamically executing an
   * unknown SQL string. The <code>execute</code> method executes an SQL statement and indicates the
   * form of the first result. You must then use the methods <code>getResultSet</code> or <code>
   * getUpdateCount</code> to retrieve the result, and <code>getInternalMoreResults</code> to move
   * to any subsequent result(s).
   *
   * @param sql any SQL statement
   * @param columnIndexes an array of the indexes of the columns in the inserted row that should be
   *     made available for retrieval by a call to the method <code>getGeneratedKeys</code>
   * @return <code>true</code> if the first result is a <code>ResultSet</code> object; <code>false
   *     </code> if it is an update count or there are no results
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the elements in the <code>int</code> array passed to this method
   *     are not valid column indexes
   * @see #getResultSet
   * @see #getUpdateCount
   * @see #getMoreResults
   */
  bool MariaDbStatement::execute(const SQLString& sql, int32_t* columnIndexes) {
    return executeInternal(sql, fetchSize, Statement::RETURN_GENERATED_KEYS);
  }

  /**
   * Executes the given SQL statement, which may return multiple results, and signals the driver
   * that the auto-generated keys indicated in the given array should be made available for
   * retrieval. This array contains the names of the columns in the target table that contain the
   * auto-generated keys that should be made available. The driver will ignore the array if the SQL
   * statement is not an <code>INSERT</code> statement, or an SQL statement able to return
   * auto-generated keys (the list of such statements is vendor-specific).
   *
   * <p>In some (uncommon) situations, a single SQL statement may return multiple result sets and/or
   * update counts. Normally you can ignore this unless you are (1) executing a stored procedure
   * that you know may return multiple results or (2) you are dynamically executing an unknown SQL
   * string.
   *
   * <p>The <code>execute</code> method executes an SQL statement and indicates the form of the
   * first result. You must then use the methods <code>getResultSet</code> or <code>getUpdateCount
   * </code> to retrieve the result, and <code>getInternalMoreResults</code> to move to any
   * subsequent result(s).
   *
   * @param sql any SQL statement
   * @param columnNames an array of the names of the columns in the inserted row that should be made
   *     available for retrieval by a call to the method <code>getGeneratedKeys</code>
   * @return <code>true</code> if the next result is a <code>ResultSet</code> object; <code>false
   *     </code> if it is an update count or there are no more results
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the elements of the <code>String</code> array passed to this
   *     method are not valid column names
   * @see #getResultSet
   * @see #getUpdateCount
   * @see #getMoreResults
   * @see #getGeneratedKeys
   */
  bool MariaDbStatement::execute(const SQLString& sql,const SQLString* columnNames) {
    return executeInternal(sql, fetchSize, Statement::RETURN_GENERATED_KEYS);
  }

  /**
   * executes a select query.
   *
   * @param sql the query to send to the server
   * @return a result set
   * @throws SQLException if something went wrong
   */
  ResultSet* MariaDbStatement::executeQuery(const SQLString& sql) {
    if (executeInternal(sql,fetchSize,Statement::NO_GENERATED_KEYS)){
      return results->releaseResultSet();
    }
    return SelectResultSet::createEmptyResultSet();
  }

  /**
   * Executes an update.
   *
   * @param sql the update query.
   * @return update count
   * @throws SQLException if the query could not be sent to server.
   */
  int32_t MariaDbStatement::executeUpdate(const SQLString& sql) {
    if (executeInternal(sql,fetchSize,Statement::NO_GENERATED_KEYS)){
      throw SQLException("executeUpdate should not be used for queries returning a resultset");
    }
    return getUpdateCount();
  }

  /**
   * Executes the given SQL statement and signals the driver with the given flag about whether the
   * auto-generated keys produced by this <code>Statement</code> object should be made available for
   * retrieval. The driver will ignore the flag if the SQL statement is not an <code>INSERT</code>
   * statement, or an SQL statement able to return auto-generated keys (the list of such statements
   * is vendor-specific).
   *
   * @param sql an SQL Data Manipulation Language (DML) statement, such as <code>INSERT</code>,
   *     <code>UPDATE</code> or <code>DELETE</code>; or an SQL statement that returns nothing, such
   *     as a DDL statement.
   * @param autoGeneratedKeys a flag indicating whether auto-generated keys should be made available
   *     for retrieval; one of the following constants: <code>Statement.RETURN_GENERATED_KEYS</code>
   *     <code>Statement.NO_GENERATED_KEYS</code>
   * @return either (1) the row count for SQL Data Manipulation Language (DML) statements or (2) 0
   *     for SQL statements that return nothing
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code>, the given SQL statement returns a <code>ResultSet</code> object, or
   *     the given constant is not one of those allowed
   */
  int32_t MariaDbStatement::executeUpdate(const SQLString& sql, int32_t autoGeneratedKeys)
  {
    if (executeInternal(sql,fetchSize,autoGeneratedKeys))
    {
      throw SQLException("executeUpdate should not be used for queries returning a resultset");
    }
    return getUpdateCount();
  }

  /**
   * Executes the given SQL statement and signals the driver that the auto-generated keys indicated
   * in the given array should be made available for retrieval. This array contains the indexes of
   * the columns in the target table that contain the auto-generated keys that should be made
   * available. The driver will ignore the array if the SQL statement is not an <code>INSERT</code>
   * statement, or an SQL statement able to return auto-generated keys (the list of such statements
   * is vendor-specific).
   *
   * @param sql an SQL Data Manipulation Language (DML) statement, such as <code>INSERT</code>,
   *     <code>UPDATE</code> or <code>DELETE</code>; or an SQL statement that returns nothing, such
   *     as a DDL statement.
   * @param columnIndexes an array of column indexes indicating the columns that should be returned
   *     from the inserted row
   * @return either (1) the row count for SQL Data Manipulation Language (DML) statements or (2) 0
   *     for SQL statements that return nothing
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code>, the SQL statement returns a <code>ResultSet</code> object, or the
   *     second argument supplied to this method is not an <code>int</code> array whose elements are
   *     valid column indexes
   */
  int32_t MariaDbStatement::executeUpdate(const SQLString& sql, int32_t* columnIndexes)
  {
    return executeUpdate(sql, Statement::RETURN_GENERATED_KEYS);
  }

  /**
   * Executes the given SQL statement and signals the driver that the auto-generated keys indicated
   * in the given array should be made available for retrieval. This array contains the names of the
   * columns in the target table that contain the auto-generated keys that should be made available.
   * The driver will ignore the array if the SQL statement is not an <code>INSERT</code> statement,
   * or an SQL statement able to return auto-generated keys (the list of such statements is
   * vendor-specific).
   *
   * @param sql an SQL Data Manipulation Language (DML) statement, such as <code>INSERT</code>,
   *     <code>UPDATE</code> or <code>DELETE</code>; or an SQL statement that returns nothing, such
   *     as a DDL statement.
   * @param columnNames an array of the names of the columns that should be returned from the
   *     inserted row
   * @return either the row count for <code>INSERT</code>, <code>UPDATE</code>, or <code>DELETE
   *     </code> statements, or 0 for SQL statements that return nothing
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code>, the SQL statement returns a <code>ResultSet</code> object, or the
   *     second argument supplied to this method is not a <code>String</code> array whose elements
   *     are valid column names
   */
  int32_t MariaDbStatement::executeUpdate(const SQLString& sql,const SQLString* columnNames) {
    return executeUpdate(sql,Statement::RETURN_GENERATED_KEYS);
  }

  int64_t MariaDbStatement::executeLargeUpdate(const SQLString& sql) {
    if (executeInternal(sql,fetchSize,Statement::NO_GENERATED_KEYS)){
      throw SQLException("executeLargeUpdate should not be used for queries returning a resultset");
    }
    return getLargeUpdateCount();
  }

  int64_t MariaDbStatement::executeLargeUpdate(const SQLString& sql, int32_t autoGeneratedKeys) {
    if (executeInternal(sql,fetchSize,autoGeneratedKeys)){
      return 0;
    }
    return getLargeUpdateCount();
  }

  int64_t MariaDbStatement::executeLargeUpdate(const SQLString& sql, int32_t* columnIndexes) {
    return executeLargeUpdate(sql,Statement::RETURN_GENERATED_KEYS);
  }

  int64_t MariaDbStatement::executeLargeUpdate(const SQLString& sql, const SQLString* columnNames) {
    return executeLargeUpdate(sql,Statement::RETURN_GENERATED_KEYS);
  }

  /**
   * Releases this <code>Statement</code> object's database and JDBC resources immediately instead
   * of waiting for this to happen when it is automatically closed. It is generally good practice to
   * release resources as soon as you are finished with them to avoid tying up database resources.
   * Calling the method <code>close</code> on a <code>Statement</code> object that is already closed
   * has no effect. <B>Note:</B>When a <code>Statement</code> object is closed, its current <code>
   * ResultSet</code> object, if one exists, is also closed.
   *
   * @throws SQLException if a database access error occurs
   */
  void MariaDbStatement::close()
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);

    try {
      closed = true;
      if (results) {
        if (results->getFetchSize() != 0) {
          skipMoreResults();
        }

        results->close();
      }

      if (protocol->isClosed()
        || !connection->pooledConnection
        || connection->pooledConnection->noStmtEventListeners()) {
        //protocol.reset();
        return;
      }
      connection->pooledConnection->fireStatementClosed(this);
    }
    catch (...) {
    }
    //protocol.reset();
    connection= nullptr;
  }

  /**
   * Retrieves the maximum number of bytes that can be returned for character and binary column
   * values in a <code>ResultSet</code> object produced by this <code>Statement</code> object. This
   * limit applies only to <code>BINARY</code>, <code>VARBINARY</code>, <code>LONGVARBINARY</code>,
   * <code>CHAR</code>, <code>VARCHAR</code>, <code>NCHAR</code>, <code>NVARCHAR</code>, <code>
   * LONGNVARCHAR</code> and <code>LONGVARCHAR</code> columns. If the limit is exceeded, the excess
   * data is silently discarded.
   *
   * @return the current column size limit for columns storing character and binary values; zero
   *     means there is no limit
   * @see #setMaxFieldSize
   */
  uint32_t MariaDbStatement::getMaxFieldSize(){
    return maxFieldSize;
  }

  /**
   * Sets the limit for the maximum number of bytes that can be returned for character and binary
   * column values in a <code>ResultSet</code> object produced by this <code>Statement</code>
   * object. This limit applies only to <code>BINARY</code>, <code>VARBINARY</code>, <code>
   * LONGVARBINARY</code>, <code>CHAR</code>, <code>VARCHAR</code>, <code>NCHAR</code>, <code>
   * NVARCHAR</code>, <code>LONGNVARCHAR</code> and <code>LONGVARCHAR</code> fields. If the limit is
   * exceeded, the excess data is silently discarded. For maximum portability, use values greater
   * than 256.
   *
   * @param max the new column size limit in bytes; zero means there is no limit
   * @see #getMaxFieldSize
   */
  void MariaDbStatement::setMaxFieldSize(uint32_t max){
    maxFieldSize= max;
  }

  /**
   * Retrieves the maximum number of rows that a <code>ResultSet</code> object produced by this
   * <code>Statement</code> object can contain. If this limit is exceeded, the excess rows are
   * silently dropped.
   *
   * @return the current maximum number of rows for a <code>ResultSet</code> object produced by this
   *     <code>Statement</code> object; zero means there is no limit
   * @see #setMaxRows
   */
  int32_t MariaDbStatement::getMaxRows(){
    return (int32_t)maxRows;
  }

  /**
   * Sets the limit for the maximum number of rows that any <code>ResultSet</code> object generated
   * by this <code>Statement</code> object can contain to the given number. If the limit is
   * exceeded, the excess rows are silently dropped.
   *
   * @param max the new max rows limit; zero means there is no limit
   * @throws SQLException if the condition max &gt;= 0 is not satisfied
   * @see #getMaxRows
   */
  void MariaDbStatement::setMaxRows(int32_t max) {
    if (max < 0){
      exceptionFactory->raiseStatementError(connection, this)->create(
        "max rows cannot be negative : asked for " + std::to_string(max),
        "42000").Throw();
    }
    maxRows= max;
  }


  int64_t MariaDbStatement::getLargeMaxRows(){
    return maxRows;
  }


  void MariaDbStatement::setLargeMaxRows(int64_t max) {
    if (max <0){
      exceptionFactory->raiseStatementError(connection, this)->create(
        "max rows cannot be negative : setLargeMaxRows value is " + std::to_string(max),
        "42000").Throw();
    }
    maxRows= max;
  }

  /**
   * Sets escape processing on or off. If escape scanning is on (the default), the driver will do
   * escape substitution before sending the SQL statement to the database. Note: Since prepared
   * statements have usually been parsed prior to making this call, disabling escape processing for
   * <code>PreparedStatements</code> objects will have no effect.
   *
   * @param enable <code>true</code> to enable escape processing; <code>false</code> to disable it
   */
  void MariaDbStatement::setEscapeProcessing(bool enable){
    // not handled
  }

  /**
   * Retrieves the number of seconds the driver will wait for a <code>Statement</code> object to
   * execute. If the limit is exceeded, a <code>SQLException</code> is thrown.
   *
   * @return the current query timeout limit in seconds; zero means there is no limit
   * @see #setQueryTimeout
   */
  int32_t MariaDbStatement::getQueryTimeout(){
    return queryTimeout;
  }

  /**
   * Sets the number of seconds the driver will wait for a <code>Statement</code> object to execute
   * to the given number of seconds. If the limit is exceeded, an <code>SQLException</code> is
   * thrown. A JDBC driver must apply this limit to the <code>execute</code>, <code>executeQuery
   * </code> and <code>executeUpdate</code> methods.
   *
   * @param seconds the new query timeout limit in seconds; zero means there is no limit
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the condition seconds &gt;= 0 is not satisfied
   * @see #getQueryTimeout
   */
  void MariaDbStatement::setQueryTimeout(int32_t seconds) {
    if (seconds < 0){
      exceptionFactory->raiseStatementError(connection, this)->create(
        "Query timeout value cannot be negative : asked for " + std::to_string(seconds)).Throw();
    }
    this->queryTimeout= seconds;
  }

  /**
   * Sets the inputStream that will be used for the next execute that uses "LOAD DATA LOCAL INFILE".
   * The name specified as local file/URL will be ignored.
   *
   * @param inputStream inputStream instance, that will be used to send data to server
   * @throws SQLException if statement is closed
   */
  void MariaDbStatement::setLocalInfileInputStream(std::istream* inputStream)
  {
    checkClose();
    protocol->setLocalInfileInputStream(*inputStream);
  }

  /**
   * Cancels this <code>Statement</code> object if both the DBMS and driver support aborting an SQL
   * statement. This method can be used by one thread to cancel a statement that is being executed
   * by another thread.
   *
   * <p>In case there is result-set from this Statement that are still streaming data from server,
   * will cancel streaming.
   *
   * @throws SQLException if a database access error occurs or this method is called on a closed
   *     <code>Statement</code>
   */
  void MariaDbStatement::cancel()
  {
    checkClose();
    bool locked= lock->try_lock();
    try {
      if (executing)
      {
        protocol->cancelCurrentQuery();

      }else if (results
          && results->getFetchSize()!=0
          && !results->isFullyLoaded(protocol.get()))
      {
        try {
          protocol->cancelCurrentQuery();
          skipMoreResults();
        }catch (SQLException&){
        }
        results->removeFetchSize();
      }
    }
    catch (SQLException& e)
    {
      logger->error("error cancelling query", e);
      if (locked) {
        lock->unlock();
      }
      exceptionFactory->raiseStatementError(connection, this)->create(e);
    }

    if (locked) {
      lock->unlock();
    }
  }

  /**
   * Retrieves the first warning reported by calls on this <code>Statement</code> object. Subsequent
   * <code>Statement</code> object warnings will be chained to this <code>SQLWarning</code> object.
   *
   * <p>The warning chain is automatically cleared each time a statement is (re)executed. This
   * method may not be called on a closed <code>Statement</code> object; doing so will cause an
   * <code>SQLException</code> to be thrown.
   *
   * <p><B>Note:</B> If you are processing a <code>ResultSet</code> object, any warnings associated
   * with reads on that <code>ResultSet</code> object will be chained on it rather than on the
   * <code>Statement</code> object that produced it.
   *
   * @return the first <code>SQLWarning</code> object or <code>null</code> if there are no warnings
   * @throws SQLException if a database access error occurs or this method is called on a closed
   *     <code>Statement</code>
   */
  SQLWarning* MariaDbStatement::getWarnings() {
    checkClose();
    if (!warningsCleared){
      return this->connection->getWarnings();
    }
    return NULL;
  }

  /**
   * Clears all the warnings reported on this <code>Statement</code> object. After a call to this
   * method, the method <code>getWarnings</code> will return <code>null</code> until a new warning
   * is reported for this <code>Statement</code> object.
   */
  void MariaDbStatement::clearWarnings(){
    warningsCleared= true;
  }

  /**
   * Sets the SQL cursor name to the given <code>String</code>, which will be used by subsequent
   * <code>Statement</code> object <code>execute</code> methods. This name can then be used in SQL
   * positioned update or delete statements to identify the current row in the <code>ResultSet
   * </code> object generated by this statement. If the database does not support positioned
   * update/delete, this method is a noop. To insure that a cursor has the proper isolation level to
   * support updates, the cursor's <code>SELECT</code> statement should have the form <code>
   * SELECT FOR UPDATE</code>. If <code>FOR UPDATE</code> is not present, positioned updates may
   * fail.
   *
   * <p><B>Note:</B> By definition, the execution of positioned updates and deletes must be done by
   * a different <code>Statement</code> object than the one that generated the <code>ResultSet
   * </code> object being used for positioning. Also, cursor names must be unique within a
   * connection.
   *
   * @param name the new cursor name, which must be unique within a connection
   * @throws SQLException if a database access error occurs or this method is called on a closed
   *     <code>Statement</code>
   */
  void MariaDbStatement::setCursorName(const SQLString& name) {
    throw exceptionFactory->raiseStatementError(connection, this)->notSupported("Cursors are not supported");
  }

  /**
   * Gets the connection that created this statement.
   *
   * @return the connection
   */
  Connection* MariaDbStatement::getConnection(){
    return this->connection;
  }

  /**
   * Retrieves any auto-generated keys created as a result of executing this <code>Statement</code>
   * object. If this <code>Statement</code> object did not generate any keys, an empty <code>
   * ResultSet</code> object is returned.
   *
   * <p><B>Note:</B>If the columns which represent the auto-generated keys were not specified, the
   * JDBC driver implementation will determine the columns which best represent the auto-generated
   * keys.
   *
   * @return a <code>ResultSet</code> object containing the auto-generated key(s) generated by the
   *     execution of this <code>Statement</code> object
   * @throws SQLException if a database access error occurs or this method is called on a closed
   *     <code>Statement</code>
   */
  ResultSet* MariaDbStatement::getGeneratedKeys()
  {
    if (results){
      return results->getGeneratedKeys(protocol.get());
    }
    return SelectResultSet::createEmptyResultSet();
  }

  /**
   * Retrieves the result set holdability for <code>ResultSet</code> objects generated by this
   * <code>Statement</code> object.
   *
   * @return either <code>ResultSet.HOLD_CURSORS_OVER_COMMIT</code> or <code>
   *     ResultSet.CLOSE_CURSORS_AT_COMMIT</code>
   * @since 1.4
   */
  int32_t MariaDbStatement::getResultSetHoldability(){
    return ResultSet::HOLD_CURSORS_OVER_COMMIT;
  }

  /**
   * Retrieves whether this <code>Statement</code> object has been closed. A <code>Statement</code>
   * is closed if the method close has been called on it, or if it is automatically closed.
   *
   * @return true if this <code>Statement</code> object is closed; false if it is still open
   * @since 1.6
   */
  bool MariaDbStatement::isClosed(){
    return closed;
  }

  bool MariaDbStatement::isPoolable(){
    return false;
  }

  void MariaDbStatement::setPoolable(bool poolable){


  }

  /**
   * Retrieves the current result as a ResultSet object. This method should be called only once per
   * result.
   *
   * @return the current result as a ResultSet object or null if the result is an update count or
   *     there are no more results
   * @throws SQLException if a database access error occurs or this method is called on a closed
   *     Statement
   */
  ResultSet* MariaDbStatement::getResultSet() {
    checkClose();
    return results ? results->releaseResultSet() : nullptr;
  }

  /**
   * Retrieves the current result as an update count; if the result is a ResultSet object or there
   * are no more results, -1 is returned. This method should be called only once per result.
   *
   * @return the current result as an update count; -1 if the current result is a ResultSet object
   *     or there are no more results
   */
  int32_t MariaDbStatement::getUpdateCount(){
    if (results && results->getCmdInformation() && !results->isBatch() ){
      return results->getCmdInformation()->getUpdateCount();
    }
    return -1;
  }

  int64_t MariaDbStatement::getLargeUpdateCount()
  {
    if (results && results->getCmdInformation() && !results->isBatch())
    {
      return results->getCmdInformation()->getLargeUpdateCount();
    }
    return -1;
  }


  void MariaDbStatement::skipMoreResults()
  {
    try
    {
      protocol->skip();
      warningsCleared= false;
      connection->reenableWarnings();
    }
    catch (SQLException& e)
    {
      logger->debug("error skipMoreResults",e);
      exceptionFactory->raiseStatementError(connection, this)->create(e);
    }
  }

  /**
   * Moves to this <code>Statement</code> object's next result, returns <code>true</code> if it is a
   * <code>ResultSet</code> object, and implicitly closes any current <code>ResultSet</code>
   * object(s) obtained with the method <code>getResultSet</code>. There are no more results when
   * the following is true:
   *
   * <pre> // stmt is a Statement object
   * ((stmt.getInternalMoreResults() == false) &amp;&amp; (stmt.getUpdateCount() == -1)) </pre>
   *
   * @return <code>true</code> if the next result is a <code>ResultSet</code> object; <code>false
   *     </code> if it is an update count or there are no more results
   * @throws SQLException if a database access error occurs or this method is called on a closed
   *     <code>Statement</code>
   * @see #execute
   */
  bool MariaDbStatement::getMoreResults() {
    return getMoreResults(Statement::CLOSE_CURRENT_RESULT);
  }

  /**
   * Moves to this <code>Statement</code> object's next result, deals with any current <code>
   * ResultSet</code> object(s) according to the instructions specified by the given flag, and
   * returns <code>true</code> if the next result is a <code>ResultSet</code> object. There are no
   * more results when the following is true:
   *
   * <pre> // stmt is a Statement object
   * ((stmt.getInternalMoreResults(current) == false) &amp;&amp; (stmt.getUpdateCount() == -1))
   * </pre>
   *
   * @param current one of the following <code>Statement</code> constants indicating what should
   *     happen to current <code>ResultSet</code> objects obtained using the method <code>
   *     getResultSet</code>: <code>Statement.CLOSE_CURRENT_RESULT</code>, <code>
   *     Statement.KEEP_CURRENT_RESULT</code>, or <code>Statement.CLOSE_ALL_RESULTS</code>
   * @return <code>true</code> if the next result is a <code>ResultSet</code> object; <code>false
   *     </code> if it is an update count or there are no more results
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the argument supplied is not one of the following: <code>
   *     Statement.CLOSE_CURRENT_RESULT</code>, <code>Statement.KEEP_CURRENT_RESULT</code> or <code>
   *     Statement.CLOSE_ALL_RESULTS</code>
   * @see #execute
   */
  bool MariaDbStatement::getMoreResults(int32_t current) {
    // if fetch size is set to read fully, other resultSet are put in cache
    checkClose();
    return results && results->getMoreResults(current, protocol.get());
  }

  /**
   * Retrieves the direction for fetching rows from database tables that is the default for result
   * sets generated from this <code>Statement</code> object. If this <code>Statement</code> object
   * has not set a fetch direction by calling the method <code>setFetchDirection</code>, the return
   * value is implementation-specific.
   *
   * @return the default fetch direction for result sets generated from this <code>Statement</code>
   *     object
   * @see #setFetchDirection
   * @since 1.2
   */
  int32_t MariaDbStatement::getFetchDirection(){
    return ResultSet::FETCH_FORWARD;
  }

  /**
   * Gives the driver a hint as to the direction in which rows will be processed in <code>ResultSet
   * </code> objects created using this <code>Statement</code> object. The default value is <code>
   * ResultSet.FETCH_FORWARD</code>.
   *
   * <p>Note that this method sets the default fetch direction for result sets generated by this
   * <code>Statement</code> object. Each result set has its own methods for getting and setting its
   * own fetch direction.
   *
   * @param direction the initial direction for processing rows
   * @see #getFetchDirection
   * @since 1.2
   */
  void MariaDbStatement::setFetchDirection(int32_t direction){


  }

  /**
   * Retrieves the number of result set rows that is the default fetch size for <code>ResultSet
   * </code> objects generated from this <code>Statement</code> object. If this <code>Statement
   * </code> object has not set a fetch size by calling the method <code>setFetchSize</code>, the
   * return value is implementation-specific.
   *
   * @return the default fetch size for result sets generated from this <code>Statement</code>
   *     object
   * @see #setFetchSize
   */
  int32_t MariaDbStatement::getFetchSize(){
    return this->fetchSize;
  }

  /**
   * Gives the JDBC driver a hint as to the number of rows that should be fetched from the database
   * when more rows are needed for <code>ResultSet</code> objects generated by this <code>Statement
   * </code>. If the value specified is zero, then the hint is ignored. The default value is zero.
   *
   * @param rows the number of rows to fetch
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the condition <code>rows &gt;= 0</code> is not satisfied.
   * @see #getFetchSize
   */
  void MariaDbStatement::setFetchSize(int32_t rows) {
    if (rows != 0) {
      throw SQLFeatureNotImplementedException("setFetchSize(ResultSet streaming) support is implemented beginning from version 1.1");
    }
    if (rows < 0 && rows != INT32_MIN){
      exceptionFactory->raiseStatementError(connection, this)->create("invalid fetch size").Throw();
    }else if (rows == INT32_MIN)
    {
      this->fetchSize= 1;
      return;
    }
    this->fetchSize= rows;
  }

  /**
   * Retrieves the result set concurrency for <code>ResultSet</code> objects generated by this
   * <code>Statement</code> object.
   *
   * @return either <code>ResultSet.CONCUR_READ_ONLY</code> or <code>ResultSet.CONCUR_UPDATABLE
   *     </code>
   * @since 1.2
   */
  int32_t MariaDbStatement::getResultSetConcurrency(){
    return resultSetConcurrency;
  }

  /**
   * Retrieves the result set type for <code>ResultSet</code> objects generated by this <code>
   * Statement</code> object.
   *
   * @return one of <code>ResultSet.TYPE_FORWARD_ONLY</code>, <code>
   *     ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
   */
  int32_t MariaDbStatement::getResultSetType(){
    return resultSetScrollType;
  }

  /**
   * Adds the given SQL command to the current list of commands for this <code>Statement</code>
   * object. The send in this list can be executed as a batch by calling the method <code>
   * executeBatch</code>.
   *
   * @param sql typically this is a SQL <code>INSERT</code> or <code>UPDATE</code> statement
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the driver does not support batch updates
   * @see #executeBatch
   * @see DatabaseMetaData#supportsBatchUpdates
   */
  void MariaDbStatement::addBatch(const SQLString& sql) {
    if (sql.empty())
    {
      exceptionFactory->raiseStatementError(connection, this)->create("Empty string cannot be set to addBatch(const SQLString& sql)").Throw();
    }
    batchQueries.push_back(sql);
  }

  /**
   * Empties this <code>Statement</code> object's current list of SQL send.
   *
   * @see #addBatch
   * @see DatabaseMetaData#supportsBatchUpdates
   * @since 1.2
   */
  void MariaDbStatement::clearBatch(){
    if (batchQueries.empty() != true){
      batchQueries.clear();
    }
  }

  /**
   * Execute statements. depending on option, queries mays be rewritten :
   *
   * <p>those queries will be rewritten if possible to INSERT INTO ... VALUES (...) ; INSERT INTO
   * ... VALUES (...);
   *
   * <p>if option rewriteBatchedStatements is set to true, rewritten to INSERT INTO ... VALUES
   * (...), (...);
   *
   * @return an array of update counts containing one element for each command in the batch. The
   *     elements of the array are ordered according to the order in which send were added to the
   *     batch.
   * @throws SQLException if a database access error occurs, this method is called on a closed
   *     <code>Statement</code> or the driver does not support batch statements. Throws {@link
   *     BatchUpdateException} (a subclass of <code>SQLException</code>) if one of the send sent to
   *     the database fails to execute properly or attempts to return a result set.
   * @see #addBatch
   * @see DatabaseMetaData#supportsBatchUpdates
   * @since 1.3
   */
  const sql::Ints& MariaDbStatement::executeBatch()
  {
    checkClose();
    std::size_t size= batchQueries.size();

    batchRes.wrap(nullptr, 0);
    if (size == 0) {
      return batchRes;
    }

    std::unique_lock<std::mutex> localScopeLock(*lock);
    try
    {
      internalBatchExecution(size);
      executeBatchEpilogue();

      return batchRes.wrap(results->getCmdInformation()->getUpdateCounts());// .data();
    }
    catch (SQLException& initialSqlEx){
      executeBatchEpilogue();
      localScopeLock.unlock();
      throw executeBatchExceptionEpilogue(initialSqlEx, size);
    }
    //To please compilers etc
    return batchRes;
  }


  const sql::Longs& MariaDbStatement::executeLargeBatch()
  {
    checkClose();
    std::size_t size= batchQueries.size();
    largeBatchRes.wrap(nullptr, 0);

    if (size == 0) {
      return largeBatchRes;
    }

    std::unique_lock<std::mutex> localScopeLock(*lock);
    try
    {
      internalBatchExecution(size);
      executeBatchEpilogue();

      return largeBatchRes.wrap(results->getCmdInformation()->getLargeUpdateCounts());

    }
    catch (SQLException& initialSqlEx)
    {
      executeBatchEpilogue();
      localScopeLock.unlock();
      throw executeBatchExceptionEpilogue(initialSqlEx, size);
    }/* TODO: something with the finally was once here */
    //To please compilers etc
    return largeBatchRes;
  }

  /**
   * Internal batch execution.
   *
   * @param size expected result-set size
   * @throws SQLException throw exception if batch error occur
   */
  void MariaDbStatement::internalBatchExecution(std::size_t size)
  {
    std::vector<Shared::ParameterHolder> dummy;
    executeQueryPrologue(true);
    results.reset(new Results(
          this,
          0,
          true,
          size,
          false,
          resultSetScrollType,
          resultSetConcurrency,
          Statement::RETURN_GENERATED_KEYS,
          protocol->getAutoIncrementIncrement(),
          NULL,
          dummy));
    protocol->executeBatchStmt(protocol->isMasterConnection(),results,batchQueries);
    results->commandEnd();
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
   * @param interfaceOrWrapper a Class defining an interface.
   * @return true if this implements the interface or directly or indirectly wraps an object that
   *     does.
   * @throws SQLException if an error occurs while determining whether this is a wrapper for an
   *     object with the given interface.
   * @since 1.6
   */
#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /* for interface we could do dynamic_cast, for wrapper... */
  bool MariaDbStatement::isWrapperFor(const Class<?>interfaceOrWrapper) {
    return interfaceOrWrapper.isInstance(this);
  }
#endif
  void MariaDbStatement::closeOnCompletion(){
    mustCloseOnCompletion= true;
  }

  bool MariaDbStatement::isCloseOnCompletion(){
    return mustCloseOnCompletion;
  }

  /**
   * Check that close on completion is asked, and close if so.
   *
   * @param resultSet resultSet
   * @throws SQLException if close has error
   */
  void MariaDbStatement::checkCloseOnCompletion(ResultSet* resultSet)
  {
    if (mustCloseOnCompletion
        && !closed
        && results
        && (resultSet == results->getResultSet())) /* TODO: check if just comparing of pointers is enough here */
    {
      close();
    }
  }

  /**
   * Check if statement is closed, and throw exception if so.
   *
   * @throws SQLException if statement close
   */
  void MariaDbStatement::checkClose() {
    if (closed){
      exceptionFactory->raiseStatementError(connection, this)->create("Cannot do an operation on a closed statement").Throw();
    }
  }

  /**
   * Permit to retrieve current connection thread id, or -1 if unknown.
   *
   * @return current connection thread id.
   */
  int64_t MariaDbStatement::getServerThreadId()
  {
    return (protocol ? protocol->getServerThreadId() : -1);
  }

  Shared::Results& MariaDbStatement::getInternalResults() {
    return results;
  }

  void MariaDbStatement::setInternalResults(Results* newResults) {
    results.reset(newResults);
  }

  void MariaDbStatement::setExecutingFlag(bool _set) {
    executing= _set;
  }

  void MariaDbStatement::markClosed()
  {
    closed= true;
    connection= NULL;
  }
}
}
