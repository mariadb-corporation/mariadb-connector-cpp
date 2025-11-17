/************************************************************************************
   Copyright (C) 2023, 2025 MariaDB Corporation plc

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


#include <random>
#include <chrono>

#include "mysqld_error.h"

#include "lru/pscache.h"

#include "Protocol.h"
#include "Results.h"
#include "ClientPrepareResult.h"
#include "ServerPrepareResult.h"
#include "Exception.h"
#include "interface/ResultSet.h"
#include "Results.h"

#define DEFAULT_TRX_ISOL_VARNAME "tx_isolation"
#define CONST_QUERY(QUERY_STRING_LITERAL) realQuery(QUERY_STRING_LITERAL,sizeof(QUERY_STRING_LITERAL)-1)
#define SEND_CONST_QUERY(QUERY_STRING_LITERAL) sendQuery(QUERY_STRING_LITERAL,sizeof(QUERY_STRING_LITERAL)-1)

namespace mariadb
{
  const int64_t Protocol::MAX_PACKET_LENGTH= 0x00ffffff + 4;
  static const char OptionSelected= 1, OptionNotSelected= 0;
  static const unsigned int uintOptionSelected= 1, uintOptionNotSelected= 0;
  static const SQLString MARIADB_RPL_HACK_PREFIX("5.5.5-");

  static const SQLString DefaultIsolationLevel("REPEATABLE READ");
  static const std::map<std::string, enum IsolationLevel> Str2TxIsolationLevel{
    {DefaultIsolationLevel,  TRANSACTION_REPEATABLE_READ},  {"REPEATABLE-READ",  TRANSACTION_REPEATABLE_READ},
    {"READ COMMITTED",   TRANSACTION_READ_COMMITTED},   {"READ-COMMITTED",   TRANSACTION_READ_COMMITTED},
    {"READ UNCOMMITTED", TRANSACTION_READ_UNCOMMITTED}, {"READ-UNCOMMITTED", TRANSACTION_READ_UNCOMMITTED},
    {"SERIALIZABLE",     TRANSACTION_SERIALIZABLE},
  };
  static const std::map<enum IsolationLevel, std::string> TxIsolationLevel2Name{
    {TRANSACTION_REPEATABLE_READ,  DefaultIsolationLevel},
    {TRANSACTION_READ_COMMITTED,   "READ COMMITTED"},
    {TRANSACTION_READ_UNCOMMITTED, "READ UNCOMMITTED"},
    {TRANSACTION_SERIALIZABLE,     "SERIALIZABLE"},
  };

  enum IsolationLevel mapStr2TxIsolation(const char* txIsolation, size_t len)
  {
    const auto& cit= Str2TxIsolationLevel.find({txIsolation, len});
    if (cit != Str2TxIsolationLevel.end()) {
      return cit->second;
    }
    return TRANSACTION_REPEATABLE_READ;
  }

  SQLString& addTxIsolationName2Query(SQLString& query, enum IsolationLevel txIsolation)
  {
    const auto& cit= TxIsolationLevel2Name.find(txIsolation);
    if (cit != TxIsolationLevel2Name.end()) {
      query.append(cit->second);
    }
    else {
      throw 1;
    }
    return query;
  }


  SQLException fromStmtError(MYSQL_STMT* stmt)
  {
    return SQLException(mysql_stmt_error(stmt), mysql_stmt_sqlstate(stmt), mysql_stmt_errno(stmt));
  }


  void throwStmtError(MYSQL_STMT* stmt) {
    throw fromStmtError(stmt);
  }


  SQLException fromConnError(MYSQL* dbc)
  {
    return SQLException(mysql_error(dbc), mysql_sqlstate(dbc), mysql_errno(dbc));
  }


  void throwConnError(MYSQL* dbc)
  {
    throw fromConnError(dbc);
  }


  void Protocol::unsyncedReset()
  {
    if (mysql_reset_connection(connection.get()))
    {
      // I wonder if connection in this case may stay ok, and we shouldn't clear the cache anyway
      throw SQLException("Connection reset failed");
    }
    serverPrepareStatementCache->clear();
    cmdEpilog();
  }

  void Protocol::reset()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    unsyncedReset();
  }

  /**
 * Get a protocol instance.
 *
 * @param 
 * @param 
 * @param 
 */
  Protocol::Protocol(MYSQL* connectedHandle, const SQLString& defaultDb, Cache<std::string, ServerPrepareResult> *psCache, const char *trIsolVarName,
    enum IsolationLevel txIsolation )
    : connection(connectedHandle, &mysql_close)
    , transactionIsolationLevel(txIsolation)
    , database(defaultDb)
    , connected(true)
    , serverVersion(mysql_get_server_info(connectedHandle))
    , serverPrepareStatementCache(psCache)
    , txIsolationVarName(trIsolVarName ? trIsolVarName : "")
  {
    parseVersion(serverVersion);

    if (serverVersion.compare(0, MARIADB_RPL_HACK_PREFIX.length(),
      MARIADB_RPL_HACK_PREFIX) == 0) {
      serverMariaDb= true;
      serverVersion= serverVersion.substr(MARIADB_RPL_HACK_PREFIX.length());
    }
    else {
      serverMariaDb= serverVersion.find("MariaDB") != std::string::npos;
    }
    unsigned long baseCaps, extCaps;
    mariadb_get_infov(connection.get(), MARIADB_CONNECTION_EXTENDED_SERVER_CAPABILITIES, (void*)&extCaps);
    mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_CAPABILITIES, (void*)&baseCaps);
    int64_t serverCaps= extCaps;
    serverCaps= serverCaps << 32;
    serverCaps|= baseCaps;
    this->serverCapabilities= serverCaps;

    getServerStatus();
    if (sessionStateAware())
      sendSessionInfos(trIsolVarName);
  }


  void Protocol::sendSessionInfos(const char *trIsolVarName)
  {
    if (!trIsolVarName) {
      trIsolVarName= DEFAULT_TRX_ISOL_VARNAME;
    }

    SQLString sessionOption("SET session_track_schema=1,session_track_system_variables='auto_increment_increment,");

    sessionOption.append(trIsolVarName);
    // MySQL does not have ansi_quotes in status, need to track it
    if (!serverMariaDb) {
      sessionOption.append(",sql_mode");
      // Getting initial value
      realQuery("SELECT 1 FROM DUAL WHERE @@sql_mode LIKE '%ansi_quotes%'");
      auto res= mysql_store_result(connection.get());
      ansiQuotes= (mysql_fetch_row(res) != nullptr);
      mysql_free_result(res);
    }
    sessionOption.append(1,'\'');
    realQuery(sessionOption);
  }

  /**
   * Execute internal query.
   *
   * <p>!! will not support multi values queries !!
   *
   * @param sql sql
   * @throws SQLException in any exception occur
   */
  void Protocol::executeQuery(const SQLString& sql)
  {
    Results res;
    executeQuery(&res, sql);
  }


  void Protocol::executeQuery(Results* results, const SQLString& sql)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    
    realQuery(sql);
    getResult(results);

    // There is not need ot catch if we just throw rfurther. But just to remember it was here for some reason
    /*catch (SQLException& sqlException) {
     if (sqlException.getSQLState().compare("70100") == 0 && 1927 == sqlException.getErrorCode()) {
        throw sqlException;
    }*/
  }


  SQLString& addQueryTimeout(SQLString& sql, int32_t queryTimeout)
  {
    if (queryTimeout > 0) {
      sql.append("SET STATEMENT max_statement_time=" + std::to_string(queryTimeout) + " FOR ");
    }
    return sql;
  }

#ifdef WE_USE_PARAMETERHOLDER_CLASS
  /**
   * Execute a unique clientPrepareQuery.
   *
   * @param mustExecuteOnMaster was intended to be launched on master connection
   * @param results results
   * @param clientPrepareResult clientPrepareResult
   * @param parameters parameters
   * @throws SQLException exception
   */
  void Protocol::executeQuery(
      bool mustExecuteOnMaster,
      Results* results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<Unique::ParameterHolder>& parameters)
  {
    executeQuery(mustExecuteOnMaster, results, clientPrepareResult, parameters, -1);
  }


  std::size_t estimatePreparedQuerySize(ClientPrepareResult* clientPrepareResult, const std::vector<SQLString> &queryPart,
      std::vector<Unique::ParameterHolder>& parameters)
  {
    std::size_t estimate= queryPart.front().length() + 1/* for \0 */, offset= 0;
    if (clientPrepareResult->isRewriteType()) {
      estimate+= queryPart[1].length() + queryPart[clientPrepareResult->getParamCount() + 2].length();
      offset= 1;
    }
    for (uint32_t i= 0; i < clientPrepareResult->getParamCount(); ++i) {
      estimate+= (parameters)[i]->getApproximateTextProtocolLength();
      estimate+= queryPart[i + 1 + offset].length();
    }
    estimate= ((estimate + 7) / 8) * 8;
    return estimate;
  }

  /**
   * Execute a unique clientPrepareQuery.
   *
   * @param mustExecuteOnMaster was intended to be launched on master connection
   * @param results results
   * @param clientPrepareResult clientPrepareResult
   * @param parameters parameters
   * @param queryTimeout if timeout is set and must use max_statement_time
   * @throws SQLException exception
   */
  void Protocol::executeQuery(
      bool mustExecuteOnMaster,
      Results* results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<Unique::ParameterHolder>& parameters,
      int32_t queryTimeout)
  {
    cmdPrologue();

    SQLString sql;
    try {
      addQueryTimeout(sql, queryTimeout);
      if (clientPrepareResult->getParamCount() == 0
        && !clientPrepareResult->isQueryMultiValuesRewritable()) {
        if (clientPrepareResult->getQueryParts().size() == 1) {
          sql.append(clientPrepareResult->getQueryParts().front());
          realQuery(sql);
        }
        else {
          for (const auto& query : clientPrepareResult->getQueryParts())
          {
            sql.append(query);
          }
          realQuery(sql);
        }
      }
      else {
        /* Timeout has been added already, thus passing -1 for its value */
        assemblePreparedQueryForExec(sql, clientPrepareResult, parameters, -1);
        realQuery(sql);
      }
      getResult(results);

    }
    catch (SQLException& queryException) {
      throw logQuery->exceptionWithQuery(parameters, queryException, clientPrepareResult);
    }
    catch (std::runtime_error& e) {
      handleIoException(e).Throw();
    }
  }

  /**
   * Execute clientPrepareQuery batch.
   *
   * @param mustExecuteOnMaster was intended to be launched on master connection
   * @param results results
   * @param prepareResult ClientPrepareResult
   * @param parametersList List of parameters
   * @param hasLongData has parameter with long data (stream)
   * @throws SQLException exception
   */
  bool Protocol::executeBatchClient(
      bool mustExecuteOnMaster,
      Results* results,
      ClientPrepareResult* prepareResult,
      std::vector<std::vector<Unique::ParameterHolder>>& parametersList,
      bool hasLongData)

  {
    // ***********************************************************************************************************
    // Multiple solution for batching :
    // - rewrite as multi-values (only if generated keys are not needed and query can be rewritten)
    // - multiple INSERT separate by semi-columns
    // - use pipeline
    // - use bulk
    // - one after the other
    // ***********************************************************************************************************

    if (options->rewriteBatchedStatements) {
      if (prepareResult->isQueryMultiValuesRewritable()
        && results->getAutoGeneratedKeys() == Statement::NO_GENERATED_KEYS) {
        // values rewritten in one query :
        // INSERT INTO X(a,b) VALUES (1,2), (3,4), ...
        executeBatchRewrite(results, prepareResult, parametersList, true);
        return true;

      }
      else if (prepareResult->isQueryMultipleRewritable()) {

        if (options->useBulkStmts
            && !hasLongData
            && prepareResult->isQueryMultipleRewritable()
            && results->getAutoGeneratedKeys()==Statement::NO_GENERATED_KEYS
            && executeBulkBatch(results, prepareResult->getSql(), nullptr, parametersList)){
          return true;
        }

        // multi rewritten in one query :
        // INSERT INTO X(a,b) VALUES (1,2);INSERT INTO X(a,b) VALUES (3,4); ...
        executeBatchRewrite(results, prepareResult, parametersList, false);
        return true;
      }
    }

    if (options->useBulkStmts
        && !hasLongData
        && results->getAutoGeneratedKeys()==Statement::NO_GENERATED_KEYS
        && executeBulkBatch(results,prepareResult->getSql(), nullptr, parametersList)) {
      return true;
    }

    if (options->useBatchMultiSend) {
      executeBatchMulti(results, prepareResult, parametersList);
      return true;
    }

    return false;
  }

  /**
   * Execute clientPrepareQuery batch.
   *
   * @param results results
   * @param sql sql command
   * @param serverPrepareResult prepare result if exist
   * @param parametersList List of parameters
   * @return if executed
   * @throws SQLException exception
   */
  bool Protocol::executeBulkBatch(
      Results* results, const SQLString& origSql,
      ServerPrepareResult* serverPrepareResult,
      std::vector<std::vector<Unique::ParameterHolder>>& parametersList)
  {
    const int16_t NullType= ColumnType::_NULL.getType();
    // **************************************************************************************
    // Ensure BULK can be use :
    // - server version >= 10.2.7
    // - no stream
    // - parameter type doesn't change
    // - avoid INSERT FROM SELECT
    // **************************************************************************************

    if ((serverCapabilities & MariaDbServerCapabilities::_MARIADB_CLIENT_STMT_BULK_OPERATIONS) == 0)
      return false;

    SQLString sql(origSql);
    // ensure that type doesn't change
    std::vector<Unique::ParameterHolder> &initParameters= parametersList.front();
    std::size_t parameterCount= initParameters.size();
    std::vector<int16_t> types;
    types.reserve(parameterCount);

    for (size_t i= 0; i < parameterCount; i++) {
      int16_t parameterType= initParameters[i]->getColumnType().getType();
      if (parameterType == NullType && parametersList.size() > 1) {
        for (std::size_t j= 1; j < parametersList.size(); ++j) {
          int16_t tmpParType= parametersList[j][i]->getColumnType().getType();
          if (tmpParType != NullType) {
            parameterType= tmpParType;
            break;
          }
        }
      }
      types.push_back(parameterType);
    }

    for (auto& parameters : parametersList) {
      for (size_t i= 0; i < parameterCount; i++) {
        int16_t rowParType= parameters[i]->getColumnType().getType();
        if (rowParType != types[i] && rowParType != NullType && types[i] != NullType) {
          return false;
        }
      }
    }

    // any select query is not applicable to bulk
    // toLowerCase changes the string in the object
    SQLString lcCopy(sql);

    if ((lcCopy.toLowerCase().find("select") != std::string::npos)){
      return false;
    }

    cmdPrologue();

    ServerPrepareResult* tmpServerPrepareResult= serverPrepareResult;

    try {
      SQLException exception;

      // **************************************************************************************
      // send PREPARE if needed
      // **************************************************************************************
      if (!tmpServerPrepareResult){
        tmpServerPrepareResult= prepareInternal(sql, true);
      }

      MYSQL_STMT* statementId= tmpServerPrepareResult ? tmpServerPrepareResult->getStatementId() : nullptr;

      //TODO: shouldn't throw if stmt is NULL? Returning false so far.
      if (statementId == nullptr)
      {
        return false;
      }
      unsigned int bulkArrSize= static_cast<unsigned int>(parametersList.size());

      mysql_stmt_attr_set(statementId, STMT_ATTR_ARRAY_SIZE, (const void*)&bulkArrSize);

      tmpServerPrepareResult->bindParameters(parametersList, types.data());
      // **************************************************************************************
      // send BULK
      // **************************************************************************************
      mysql_stmt_execute(statementId);

      try {
        getResult(results, tmpServerPrepareResult);
      }catch (SQLException& sqle){
        if (sqle.getSQLState().compare("HY000") == 0 && sqle.getErrorCode()==1295){
          // query contain commands that cannot be handled by BULK protocol
          // clear error and special error code, so it won't leak anywhere
          // and wouldn't be misinterpreted as an additional update count
          results->getCmdInformation()->reset();
          return false;
        }
        if (exception.getMessage().empty()){
          exception= logQuery->exceptionWithQuery(sql, sqle, explicitClosed);
          if (!options->continueBatchOnError){
            throw exception;
          }
        }
      }

      if (!exception.getMessage().empty()) {
        throw exception;
      }
      results->setRewritten(true);
      
      if (!serverPrepareResult && tmpServerPrepareResult) {
        releasePrepareStatement(tmpServerPrepareResult);
      }
      return true;

    } catch (std::runtime_error& e) {
      if (!serverPrepareResult && tmpServerPrepareResult) {
        releasePrepareStatement(tmpServerPrepareResult);
      }
      handleIoException(e).Throw();
    }
    //To please compilers etc
    return false;
  }

  /**
   * Execute list of queries. This method is used when using text batch statement and using
   * rewriting (allowMultiQueries || rewriteBatchedStatements). queries will be send to server
   * according to max_allowed_packet size.
   *
   * @param results result object
   * @param queries list of queries
   * @throws SQLException exception
   */
  void Protocol::executeBatchAggregateSemiColon(Results* results, const std::vector<SQLString>& queries, std::size_t totalLenEstimation)
  {
    SQLString firstSql;
    size_t currentIndex= 0;
    size_t totalQueries= queries.size();
    SQLString sql;

    do {

      firstSql= queries[currentIndex++];
      if (totalLenEstimation == 0) {
        totalLenEstimation= firstSql.length() * queries.size() + queries.size() - 1;
      }
      sql.reserve(static_cast<std::size_t>(((std::min(MAX_PACKET_LENGTH, static_cast<int64_t>(totalLenEstimation)) + 7) / 8) * 8));
      currentIndex= assembleBatchAggregateSemiColonQuery(sql, firstSql, queries, currentIndex);
      realQuery(sql);
      sql.clear(); // clear is not supposed to release memory

      getResult(results, nullptr, true);

      stopIfInterrupted();

    } while (currentIndex < totalQueries);

  }

  /**
   * Execute batch from Statement.executeBatch().
   *
   * @param mustExecuteOnMaster was intended to be launched on master connection
   * @param results results
   * @param queries queries
   * @throws SQLException if any exception occur
   */
  void Protocol::executeBatchStmt(bool mustExecuteOnMaster, Results* results,
    const std::vector<SQLString>& queries)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();

    // check that queries are rewritable
    bool canAggregateSemiColumn= true;
    std::size_t totalLen= 0;
    for (SQLString query : queries) {
      if (!ClientPrepareResult::canAggregateSemiColon(query, noBackslashEscapes())) {
        canAggregateSemiColumn= false;
        break;
      }
      totalLen+= query.length() + 1/*;*/;
    }

    if (isInterrupted()){
      // interrupted by timeout, must throw an exception manually
      throw SQLException("Timeout during batch execution", "00000");
    }

    if (canAggregateSemiColumn){
      executeBatchAggregateSemiColon(results, queries, totalLen);
    }else {
      executeBatch(results, queries);
    }
  }

  /**
   * Execute list of queries not rewritable.
   *
   * @param results result object
   * @param queries list of queries
   * @throws SQLException exception
   */
  void Protocol::executeBatch(Results* results, const std::vector<SQLString>& queries)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    for (auto& sql : queries) {
        realQuery(sql);
        getResult(results);
    }
    stopIfInterrupted();

    return;
  }
#endif

  ServerPrepareResult* Protocol::prepareInternal(const SQLString& sql)
  {
    const SQLString key(getDatabase() + "-" + sql);
    ServerPrepareResult* pr= serverPrepareStatementCache->get(key);

    if (pr) {
       return pr;
    }

    MYSQL_STMT* stmtId= mysql_stmt_init(connection.get());

    if (stmtId == nullptr)
    {
      throw SQLException(mysql_error(connection.get()), mysql_sqlstate(connection.get()), mysql_errno(connection.get()));
    }

    static const my_bool updateMaxLength= 1;

    mysql_stmt_attr_set(stmtId, STMT_ATTR_UPDATE_MAX_LENGTH, &updateMaxLength);

    if (mysql_stmt_prepare(stmtId, sql.c_str(), static_cast<unsigned long>(sql.length())))
    {
      SQLString err(mysql_stmt_error(stmtId)), sqlState(mysql_stmt_sqlstate(stmtId));
      uint32_t errNo= mysql_stmt_errno(stmtId);

      mysql_stmt_close(stmtId);
      throw SQLException(err, sqlState, errNo);
    }

    pr= new ServerPrepareResult(sql, stmtId, this);

    // This condition is now enforced on cache level on the key length
    //if (sql.length() < static_cast<size_t>(options->prepStmtCacheSqlLimit))
    {

      ServerPrepareResult* cachedServerPrepareResult= addPrepareInCache(key, pr);

      if (cachedServerPrepareResult != nullptr)
      {
        delete pr;
        pr= cachedServerPrepareResult;
      }
    }
    return pr;
  }


  ServerPrepareResult* Protocol::prepare(const SQLString& sql)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    return prepareInternal(sql);
  }


  bool Protocol::checkRemainingSize(int64_t newQueryLen)
  {
    return newQueryLen < MAX_PACKET_LENGTH;
  }


  size_t assembleBatchAggregateSemiColonQuery(SQLString& sql, const SQLString &firstSql, const std::vector<SQLString>& queries, size_t currentIndex)
  {
    sql.append(firstSql);

    // add query with ";"
    while (currentIndex < queries.size()) {

      if (!Protocol::checkRemainingSize(sql.length() + queries[currentIndex].length() + 1)) {
        break;
      }
      sql.append(';', 1).append(queries[currentIndex]);
      ++currentIndex;
    }
    return currentIndex;
  }

#ifdef WE_USE_PARAMETERHOLDER_CLASS
  /**
  * Client side PreparedStatement.executeBatch values rewritten (concatenate value params according
  * to max_allowed_packet)
  *
  * @param pos query string
  * @param queryParts query parts
  * @param currentIndex currentIndex
  * @param paramCount parameter pos
  * @param parameterList parameter list
  * @param rewriteValues is query rewritable by adding values
  * @return current index
  * @throws IOException if connection fail
  */
  std::size_t rewriteQuery(SQLString& pos,
    const std::vector<SQLString> &queryParts,
    std::size_t currentIndex,
    std::size_t paramCount,
    std::vector<std::vector<Unique::ParameterHolder>>& parameterList,
    bool rewriteValues)
  {
    std::size_t index= currentIndex, capacity= StringImp::get(pos).capacity(), estimatedLength;
    std::vector<Unique::ParameterHolder> &parameters= parameterList[index++];

    const SQLString &firstPart= queryParts[1];
    const SQLString &secondPart= queryParts.front();

    if (!rewriteValues) {

      pos.append(firstPart);
      pos.append(secondPart);

      std::size_t staticLength= 1;
      for (auto& queryPart : queryParts) {
        staticLength+= queryPart.length();
      }

      for (size_t i= 0; i < paramCount; i++) {
        parameters[i]->writeTo(pos);
        pos.append(queryParts[i + 2]);
      }
      pos.append(queryParts[paramCount + 2]);

      estimatedLength= pos.length() * (parameterList.size() - currentIndex);
      if (estimatedLength > capacity) {
        pos.reserve(((std::min<int64_t>(MAX_PACKET_LENGTH, static_cast<int64_t>(estimatedLength)) + 7) / 8) * 8);
      }

      while (index < parameterList.size()) {
        auto& parameters= parameterList[index];
        int64_t parameterLength= 0;
        bool knownParameterSize= true;

        for (auto& parameter : parameters) {
          int64_t paramSize= parameter->getApproximateTextProtocolLength();
          if (paramSize == -1) {
            knownParameterSize= false;
            break;
          }
          parameterLength+= paramSize;
        }

        if (knownParameterSize) {

          if (checkRemainingSize(pos.length() + staticLength + parameterLength)) {
            pos.append(';');
            pos.append(firstPart);
            pos.append(secondPart);
            for (size_t i= 0; i <paramCount; i++) {
              parameters[i]->writeTo(pos);
              pos.append(queryParts[i + 2]);
            }
            pos.append(queryParts[paramCount + 2]);
            ++index;
          }
          else {
            break;
          }
        }
        else {

          pos.append(';');
          pos.append(firstPart);
          pos.append(secondPart);
          for (size_t i= 0; i < paramCount; i++) {
            parameters[i]->writeTo(pos);
            pos.append(queryParts[i +2]);
          }
          pos.append(queryParts[paramCount + 2]);
          ++index;
          break;
        }
      }

    }
    else {
      pos.append(firstPart);
      pos.append(secondPart);
      size_t lastPartLength= queryParts[paramCount + 2].length();
      size_t intermediatePartLength= queryParts[1].length();

      for (size_t i= 0; i <paramCount; i++) {
        parameters[i]->writeTo(pos);
        pos.append(queryParts[i +2]);
        intermediatePartLength +=queryParts[i +2].length();
      }

      while (index < parameterList.size()) {
        auto& parameters= parameterList[index];

        int64_t parameterLength= 0;
        bool knownParameterSize= true;
        for (auto& parameter : parameters) {
          int64_t paramSize= parameter->getApproximateTextProtocolLength();
          if (paramSize == -1) {
            knownParameterSize= false;
            break;
          }
          parameterLength +=paramSize;
        }

        if (knownParameterSize) {

          if (checkRemainingSize(pos.length() + 1 + parameterLength + intermediatePartLength + lastPartLength)) {
            pos.append(',');
            pos.append(secondPart);

            for (size_t i= 0; i <paramCount; i++) {
              parameters[i]->writeTo(pos);
              pos.append(queryParts[i + 2]);
            }
            ++index;
          }
          else {
            break;
          }
        }
        else {
          pos.append(',');
          pos.append(secondPart);

          for (size_t i= 0; i <paramCount; i++) {
            parameters[i]->writeTo(pos);
            pos.append(queryParts[i +2]);
          }
          ++index;
          break;
        }
      }
      pos.append(queryParts[paramCount + 2]);
    }

    return index;
  }

  /**
   * Specific execution for batch rewrite that has specific query for memory.
   *
   * @param results result
   * @param prepareResult prepareResult
   * @param parameterList parameters
   * @param rewriteValues is rewritable flag
   * @throws SQLException exception
   */
  void Protocol::executeBatchRewrite(
      Results* results,
      ClientPrepareResult* prepareResult,
      std::vector<std::vector<Unique::ParameterHolder>>& parameterList,
      bool rewriteValues)
  {
    cmdPrologue();
    //std::vector<ParameterHolder>::const_iterator parameters;
    std::size_t currentIndex= 0;
    std::size_t totalParameterList= parameterList.size();

    try {
      SQLString sql;
      sql.reserve(1024); //No estimations. Just something for beginning. stringstream ?
      do {
        sql.clear();
        currentIndex= rewriteQuery(sql, prepareResult->getQueryParts(), currentIndex, prepareResult->getParamCount(), parameterList, rewriteValues);
        realQuery(sql);
        getResult(results, nullptr, !rewriteValues);

#ifdef THE_TIME_HAS_COME
        if (Thread.currentThread().isInterrupted()){
          throw SQLException(
              "Interrupted during batch",INTERRUPTED_EXCEPTION.getSqlState(),-1);
        }
#endif
      } while (currentIndex < totalParameterList);

    } catch (SQLException& sqlEx) {
      results->setRewritten(rewriteValues);
      throw logQuery->exceptionWithQuery(sqlEx,prepareResult);
    } catch (std::runtime_error& e) {
      results->setRewritten(rewriteValues);
      handleIoException(e).Throw();
    }
    results->setRewritten(rewriteValues);
  }

  /**
   * Execute Prepare if needed, and execute COM_STMT_EXECUTE queries in batch.
   *
   * @param mustExecuteOnMaster must normally be executed on master connection
   * @param serverPrepareResult prepare result. can be null if not prepared.
   * @param results execution results
   * @param sql sql query if needed to be prepared
   * @param parametersList parameter list
   * @param hasLongData has long data (stream)
   * @return executed
   * @throws SQLException if parameter error or connection error occur.
   */
  bool Protocol::executeBatchServer(
      bool mustExecuteOnMaster,
      ServerPrepareResult* serverPrepareResult,
      Results* results, const SQLString& sql,
      std::vector<std::vector<Unique::ParameterHolder>>& parametersList,
      bool hasLongData)
  {
    bool needToRelease= false;
    cmdPrologue();

    if (options->useBulkStmts
        && !hasLongData
        && results->getAutoGeneratedKeys()==Statement::NO_GENERATED_KEYS
        && executeBulkBatch(results, sql, serverPrepareResult, parametersList)) {
      return true;
    }

    if (!options->useBatchMultiSend) {
      return false;
    }
    initializeBatchReader();

    MYSQL_STMT *stmt= nullptr;

    if (serverPrepareResult == nullptr) {
      serverPrepareResult= prepare(sql, true);
      needToRelease= true;
    }

    stmt= serverPrepareResult->getStatementId();

    std::size_t totalExecutionNumber= parametersList.size();
    //std::size_t parameterCount= serverPrepareResult->getParameters().size();

    for (auto& paramset : parametersList) {
      executePreparedQuery(true, serverPrepareResult, results, paramset);
    }

    if (needToRelease) {
      delete serverPrepareResult;
    }
    return true;
  }
#endif

  void Protocol::executePreparedQuery(
      ServerPrepareResult* serverPrepareResult,
      Results* results)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();

    if (mysql_stmt_execute(serverPrepareResult->getStatementId()) != 0) {
      throwStmtError(serverPrepareResult->getStatementId());
    }
    /*CURSOR_TYPE_NO_CURSOR);*/
    getResult(results, serverPrepareResult);
  }

  /* Direct exectution of server-side prepared statement */
  void Protocol::directExecutePreparedQuery(
      ServerPrepareResult* serverPrepareResult,
      Results* results)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    auto& query= serverPrepareResult->getSql();
    if (mariadb_stmt_execute_direct(serverPrepareResult->getStatementId(), query.c_str(), query.length()) != 0) {
      throwStmtError(serverPrepareResult->getStatementId());
    }
    /*CURSOR_TYPE_NO_CURSOR);*/
    getResult(results, serverPrepareResult);
  }

/******************** Different batch execution methods **********************/

  /**
   * Execute clientPrepareQuery batch.
   *
   * @param results results
   * @param sql sql command
   * @param serverPrepareResult prepare result if exist
   * @param parametersList List of parameters
   * @return if executed
   * @throws SQLException exception
   */
  bool Protocol::executeBulkBatch(
    Results* results, const SQLString& origSql,
    ServerPrepareResult* serverPrepareResult,
    MYSQL_BIND* parametersList,
    uint32_t paramsetsCount)
  {
    // We are not using this function so far for real, but it's referred in function that is used. false will
    // make it function as expected
    return false;
    // **************************************************************************************
    // Ensure BULK can be use :
    // - server version >= 10.2.7
    // - no stream
    // - parameter type doesn't change
    // - avoid INSERT FROM SELECT
    // **************************************************************************************

    if ((serverCapabilities & MARIADB_CLIENT_STMT_BULK_OPERATIONS) == 0)
      return false;

    cmdPrologue();

    ServerPrepareResult* tmpServerPrepareResult= serverPrepareResult;

    try {
      SQLException exception;

      // **************************************************************************************
      // send PREPARE if needed
      // **************************************************************************************
      if (!tmpServerPrepareResult) {
        tmpServerPrepareResult= prepareInternal(origSql);
      }

      MYSQL_STMT* statementId= tmpServerPrepareResult ? tmpServerPrepareResult->getStatementId() : nullptr;

      //TODO: shouldn't throw if stmt is NULL? Returning false so far.
      if (statementId == nullptr)
      {
        return false;
      }

      mysql_stmt_attr_set(statementId, STMT_ATTR_ARRAY_SIZE, (const void*)&paramsetsCount);
      // it was dealing with tmpServerPrepareResult and maybe should
      mysql_stmt_bind_param(statementId, parametersList);
      // **************************************************************************************
      // send BULK
      // **************************************************************************************
      mysql_stmt_execute(statementId);

      try {
        getResult(results, tmpServerPrepareResult);
      }
      catch (SQLException& sqle) {
        if (!serverPrepareResult && tmpServerPrepareResult) {
          releasePrepareStatement(tmpServerPrepareResult);
          // releasePrepareStatement basically cares only about releasing stmt on server(and C API handle)
          delete tmpServerPrepareResult;
          tmpServerPrepareResult= nullptr;
        }
        if (sqle.getSQLState().compare("HY000") == 0 && sqle.getErrorCode() == 1295) {
          // query contain commands that cannot be handled by BULK protocol
          // clear error and special error code, so it won't leak anywhere
          // and wouldn't be misinterpreted as an additional update count
          results->getCmdInformation()->reset();
          return false;
        }
        if (exception.getMessage().empty()) {
          //throw logQuery->exceptionWithQuery(origSql, sqle, explicitClosed);
        }
      }

      results->setRewritten(true);

      if (!serverPrepareResult && tmpServerPrepareResult) {
        releasePrepareStatement(tmpServerPrepareResult);
        // releasePrepareStatement basically cares only about releasing stmt on server(and C API handle)
        delete tmpServerPrepareResult;
      }

      if (!exception.getMessage().empty()) {
        throw exception;
      }
      return true;
    }
    catch (SQLException &/*e*/)
    {
      //TODO see what to do here in real implementation
      return false;
    }
    //To please compilers etc
    return false;
  }

  /**
   * Specific execution for batch rewrite that has specific query for memory.
   *
   * @param results result
   * @param prepareResult prepareResult
   * @param parameterList parameters
   * @param rewriteValues is rewritable flag
   * @throws SQLException exception
   */
  void Protocol::executeBatchRewrite(
    Results* results,
    ClientPrepareResult* prepareResult,
    MYSQL_BIND* parameterList,
    uint32_t paramsetsCount,
    bool rewriteValues)
  {
    cmdPrologue();
    std::size_t nextIndex= 0;

    while (nextIndex < paramsetsCount) {
      SQLString sql("");
      nextIndex= prepareResult->assembleBatchQuery(sql, parameterList, paramsetsCount, nextIndex);
      // Or should it still go after the query?
      results->setRewritten(prepareResult->isQueryMultiValuesRewritable());
      realQuery(sql);
      getResult(results, nullptr, true);
    }
    // Feels like it's redundnt
    results->setRewritten(rewriteValues);
  }

/**
  * Execute client prepared batch as multi-send. sort of.
  *
  * @param results results
  * @param clientPrepareResult ClientPrepareResult
  * @param parametersList List of parameters
  * @param paramsetsCount size of parameters array
  * @throws SQLException exception
  */
  void Protocol::executeBatchMulti(Results* results, ClientPrepareResult* clientPrepareResult,
    MYSQL_BIND* parametersList, uint32_t paramsetsCount)
  {
    cmdPrologue();

    SQLString sql;
    bool autoCommit= getAutocommit();

    if (autoCommit) {
      SEND_CONST_QUERY("SET AUTOCOMMIT=0");
    }

    for (uint32_t i= 0; i < paramsetsCount; ++i)
    {
      sql.clear();
      // Making it to create only from 1 paramset
      clientPrepareResult->assembleBatchQuery(sql, parametersList, i+1, i);
      sendQuery(sql);
    }
    if (autoCommit) {

      // Sending commit, restoring autocommit
      SEND_CONST_QUERY("COMMIT");
      SEND_CONST_QUERY("SET AUTOCOMMIT=1");
      // Getting result for setting autocommit off - we don't need it
      readQueryResult();
    }
    for (uint32_t i= 0; i < paramsetsCount; ++i) {
      // We don't need exception on error here
      mysql_read_query_result(connection.get());
      getResult(results);
    }
    if (autoCommit) {
      // Getting result for commit and setting autocommit back on to clear the connection,
      // reading new server status(with auto-commit)
      commitReturnAutocommit(true);
    }
  }

  /**
    * Execute clientPrepareQuery batch.
    *
    * @param mustExecuteOnMaster was intended to be launched on master connection
    * @param results results
    * @param prepareResult ClientPrepareResult
    * @param parametersList List of parameters
    * @param hasLongData has parameter with long data (stream)
    * @throws SQLException exception
    */
  bool Protocol::executeBatchClient(
    Results* results,
    ClientPrepareResult* prepareResult,
    MYSQL_BIND* parametersList,
    uint32_t paramsetsCount,
    bool hasLongData)
  {
    // ***********************************************************************************************************
    // Multiple solution for batching :
    // - rewrite as multi-values (only if generated keys are not needed and query can be rewritten)
    // - multiple INSERT separate by semi-columns
    // - use pipeline
    // - use bulk
    // - one after the other
    // ***********************************************************************************************************

    /* Multi values is single statement otherwise we need multistatements allowed otherwise.
     * Probably it's better not to read client_flag, but get along other stuff we might need from outside in
     * some stuct in Protocol constructor
     */
    if (prepareResult->isQueryMultiValuesRewritable() || 
      (prepareResult->isQueryMultipleRewritable() && connection->client_flag & CLIENT_MULTI_STATEMENTS)) {
      // values rewritten in one query :
      // INSERT INTO X(a,b) VALUES (1,2), (3,4), ...
      executeBatchRewrite(results, prepareResult, parametersList, paramsetsCount, true);
      return true;

    }
    else if (prepareResult->isQueryMultipleRewritable()) {

      if (!hasLongData
        && executeBulkBatch(results, prepareResult->getSql(), nullptr, parametersList, paramsetsCount)) {
        return true;
      }
      // executeBatchRewrite does the choice on its own
      // multi rewritten in one query :
      // INSERT INTO X(a,b) VALUES (1,2);INSERT INTO X(a,b) VALUES (3,4); ...
      /*executeBatchRewrite(results, prepareResult, parametersList, paramsetsCount, false);
      return true;*/
    }
    // It used to be optional, thus could be also slow after it
    executeBatchMulti(results, prepareResult, parametersList, paramsetsCount);
    return true;
    /*executeBatchSlow(results, prepareResult, parametersList);
    return true;*/
  }

  /** Rollback transaction. */
  void Protocol::commit()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    if (inTransaction() && mysql_commit(getCHandle())) {
      throwConnError(getCHandle());
    }
  }

  /** Rollback transaction. */
  void Protocol::rollback()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    
    if (inTransaction() && mysql_rollback(getCHandle())) {
      throwConnError(getCHandle());
    }
  }

  /**
   * Force release of prepare statement that are not used. This method will be call when adding a
   * new prepare statement in cache, so the packet can be send to server without problem.
   *
   * @param statementId prepared statement Id to remove.
   * @return true if successfully released
   * @throws SQLException if connection exception.
   */
  bool Protocol::forceReleasePrepareStatement(MYSQL_STMT* statementId)
  {
    // We need lock only if still connected. if we get disconnected after we read "connected" flag - still no big deal
    bool needLock= connected;
    if (!needLock || lock.try_lock()) {
      unblockConnection();
      // If connection is closed - we still need to call mysql_stmt_close to deallocate C/C stmt handle
      if (mysql_stmt_close(statementId)) {
        if (needLock) {
          lock.unlock();
        }
        if (mysql_stmt_errno(statementId) == 2014/*CR_COMMANDS_OUT_OF_SYNC*/) {
          statementIdToRelease= statementId;
        }
        else if (mysql_stmt_errno(statementId) == 2013/*CR_SERVER_LOST*/) {
          mysql_stmt_close(statementId);
        }
      }
      if (needLock) {
        lock.unlock();
      }
      return true;

    }
    else {
      statementIdToRelease= statementId;
    }

    return false;
  }

  /**
   * Force release of prepare statement that are not used. This permit to deallocate a statement
   * that cannot be release due to multi-thread use.
   *
   * @throws SQLException if connection occur
   */
  void Protocol::forceReleaseWaitingPrepareStatement()
  {
    if (statementIdToRelease != nullptr) {
      if (mysql_stmt_close(statementIdToRelease))
      {
        // Assuming we shouldn't be out of sync - that's the whole idea. thus if we tried - that all we could do
        statementIdToRelease= nullptr;
        throw SQLException("Could not deallocate query");
      }
      statementIdToRelease= nullptr;
    }
  }


  bool Protocol::ping()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    return mysql_ping(connection.get()) == 0;
  }


  bool Protocol::isValid(int32_t timeout)
  {
    int32_t initialTimeout= -1;

    initialTimeout= this->socketTimeout;
    if (initialTimeout == 0) {
      this->changeReadTimeout(timeout);
    }

    if (!ping()) {
      throw SQLException("Could not ping");
    }
    return true;
  }


  const SQLString& Protocol::getSchema()
  {
    if (sessionStateAware()) {

      return database;
    }

    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();

    realQuery("SELECT DATABASE()");
    // This could not change status/session state
    Unique::MYSQL_RES res(mysql_store_result(getCHandle()), &mysql_free_result);
    auto row= mysql_fetch_row(res.get());
    if (row) {
      database= row[0];
      return database;
    }
    else {
      database= emptyStr;
    }

    return database;
  }


  void Protocol::setSchema(const SQLString& _database)
  {
    std::unique_lock<std::mutex> localScopeLock(lock);
    cmdPrologue();
    if (mysql_select_db(connection.get(), _database.c_str()) != 0) {
      if (mysql_get_socket(connection.get()) == MARIADB_INVALID_SOCKET) {
        std::string msg("Connection lost: ");
        msg.append(mysql_error(connection.get()));
        localScopeLock.unlock();
        throw SQLException(msg);
      }
      else {
        throw SQLException(
          "Could not select database '" + _database + "' : " + mysql_error(connection.get()),
          mysql_sqlstate(connection.get()),
          mysql_errno(connection.get()));
      }
    }
    this->database= _database;
  }

  /*void Protocol::cancelCurrentQuery()
  {
    std::unique_ptr<MasterProtocol> copiedProtocol(
      new MasterProtocol(urlParser, new GlobalStateInfo(), newMutex));

    copiedProtocol->setHostAddress(getHostAddress());
    copiedProtocol->connect();

    copiedProtocol->executeQuery("KILL QUERY " + std::to_string(getServerThreadId()));

    interrupted= true;
  }*/

  bool Protocol::getAutocommit()
  {
    return ((serverStatus & SERVER_STATUS_AUTOCOMMIT) != 0);
  }

  bool Protocol::inTransaction()
  {
    return ((serverStatus & SERVER_STATUS_IN_TRANS) != 0);
  }


  void Protocol::closeExplicit()
  {
    this->explicitClosed= true;
    close();
  }

  void Protocol::markClosed(bool closed)
  {
    this->explicitClosed= closed;
    //close();
  }

  void Protocol::releasePrepareStatement(ServerPrepareResult* serverPrepareResult)
  {
    // If prepared cache is enabled, the ServerPrepareResult can be shared in many PrepStatement,
    // so synchronised use count indicator will be decrement.
    serverPrepareResult->decrementShareCounter();

    // deallocate from server if not cached
    if (serverPrepareResult->canBeDeallocate()){
      forceReleasePrepareStatement(serverPrepareResult->getStatementId());
    }
  }


  int64_t Protocol::getMaxRows()
  {
    return maxRows;
  }


  void Protocol::setMaxRows(int64_t max)
  {
    if (maxRows != max){
      if (max == 0) {
        executeQuery("set @@SQL_SELECT_LIMIT=DEFAULT");
      }else {
        executeQuery("set @@SQL_SELECT_LIMIT=" + std::to_string(max));
      }
      maxRows= max;
    }
  }


  int32_t Protocol::getTimeout()
  {
    return this->socketTimeout;
  }


  void Protocol::setTimeout(int32_t timeout)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    this->changeReadTimeout(timeout);
  }

 
  int32_t Protocol::getTransactionIsolationLevel()
  {
    if (sessionStateAware())
      return transactionIsolationLevel;
    SQLString query("SELECT @@");
    query.append(txIsolationVarName);
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    
    realQuery(query);
    // This could not change status
    Unique::MYSQL_RES res(mysql_store_result(getCHandle()), &mysql_free_result);
    auto row= mysql_fetch_row(res.get());
    auto length= mysql_fetch_lengths(res.get());
    return mapStr2TxIsolation(row[0], length[0]);
  }


  void Protocol::checkClose()
  {
    if (!this->connected){
      throw SQLException("Connection is closed","08000",1220);
    }
  }


  void Protocol::resetError(MYSQL_STMT *stmt)
  {
    stmt->last_errno= 0;
  }


  void Protocol::moveToNextSpsResult(Results *results, ServerPrepareResult *spr)
  {
    //std::lock_guard<std::mutex> localScopeLock(lock);
    MYSQL_STMT *stmt= spr->getStatementId();
    rc= mysql_stmt_next_result(stmt);

    if (rc == 0) {
      // Temporary hack to work-around the bug C/C
      Protocol::resetError(stmt);
    }
    
    getResult(results, spr);
    // Server and session can be changed
    cmdEpilog();
  }

  void Protocol::skipAllResults()
  {
    if (hasMoreResults()) {
      //std::lock_guard<std::mutex> localScopeLock(lock);
      auto conn= connection.get();
      MYSQL_RES *res= nullptr;
      while (mysql_more_results(conn) && mysql_next_result(conn) == 0) {
        res= mysql_use_result(conn);
        mysql_free_result(res);
      }
      // Server and session can be changed
      cmdEpilog();
    }
    

  }


  void Protocol::skipAllResults(ServerPrepareResult *spr)
  {
    if (hasMoreResults()) {
      auto stmt= spr->getStatementId();
      while (mysql_stmt_more_results(stmt)) mysql_stmt_next_result(stmt);
      return;
      // Server and session can be changed
      cmdEpilog();
    }
  }


  void Protocol::moveToNextResult(Results *results, ServerPrepareResult *spr)
  {
    // It's a mess atm - all moveToNextResult thing
    // if we are moving to next result - it is active streaming result then.
     // also, we can't do any delayed operation at this point.
    if (spr != nullptr) {
      moveToNextSpsResult(results, spr);
      return;
    }

    //std::lock_guard<std::mutex> localScopeLock(lock);
    rc= mysql_next_result(connection.get());

    getResult(results, nullptr);
    // Server and session can be changed
    cmdEpilog();
  }


  void Protocol::getResult(Results* results, ServerPrepareResult *pr, bool readAllResults)
  {
    processResult(results, pr);

    if (readAllResults) {
      while (hasMoreResults()) {
        moveToNextResult(results, pr);
        processResult(results, pr);
      }
    }
  }

  /**
   * Read server response packet.
   *
   * @param results result object
   * @throws SQLException if sub-result connection fail
   * @see <a href="https://mariadb.com/kb/en/mariadb/4-server-response-packets/">server response
   *     packets</a>
   */
  void Protocol::processResult(Results* results, ServerPrepareResult *pr)
  {
    switch (rc)//errorOccurred(pr))
    {
      case 0:
        if (fieldCount(pr) == 0)
        {
          readOk(results, pr);
        }
        else
        {
          readResultSet(results, pr);
        }
        break;

      default:
        throw processError(results, pr);
    }
  }

  /**
   * Read OK - result/affected rows etc.
   *
   * @param buffer current buffer
   * @param results result object
   */
  void Protocol::readOk(Results* results, ServerPrepareResult *pr)
  {
    const int64_t updateCount= (pr == nullptr ? mysql_affected_rows(connection.get()) : mysql_stmt_affected_rows(pr->getStatementId()));
    //const int64_t insertId= (pr == nullptr ? mysql_insert_id(connection.get()) : mysql_stmt_insert_id(pr->getStatementId()));

    getServerStatus();
    hasWarningsFlag= mysql_warning_count(connection.get()) > 0;

    if (sessionStateChanged()) {
      handleStateChange();
    }
    results->addStats(updateCount, hasMoreResults());
  }


  void Protocol::handleStateChange()
  {
    const char *key, *value;
    std::size_t len, valueLen;

    for (int32_t type= SESSION_TRACK_BEGIN; type < SESSION_TRACK_END; ++type)
    {
      if (mysql_session_track_get_first(getCHandle(), static_cast<enum enum_session_state_type>(type), &key, &len) == 0)
      {
        switch (type) {
        case SESSION_TRACK_SYSTEM_VARIABLES:
        {
          mysql_session_track_get_next(getCHandle(), SESSION_TRACK_SYSTEM_VARIABLES, &value, &valueLen);

          if (std::strncmp(key, "auto_increment_increment", len) == 0)
          {
            autoIncrementIncrement= std::stoi(value);
          }
          else if (std::strncmp(key, txIsolationVarName.c_str(), len) == 0)
          {
            transactionIsolationLevel= mapStr2TxIsolation(value, valueLen);
          }
          else if (std::strncmp(key, "sql_mode", len) == 0) {
            ansiQuotes= false;
            if (valueLen > 10/*ANSI_QUOTES*/) {
              for (std::size_t i= 0; i < valueLen - 10; ++i) {
                if (value[i] == 'A' && value[++i] == 'N' && value[++i] == 'S' && value[++i] == 'I' &&
                  value[++i] == '_' && value[++i] == 'Q') {// That's enough. Even one byte more than enough
                  ansiQuotes= true;
                  break;
                }
                // Moving to the separator before next mode name
                while (i < valueLen - 11 && value[i] != ',') ++i;
              }
            }
          }
          break;
        }
        case SESSION_TRACK_SCHEMA:
          database.assign(key, len);
          break;

        default:
          break;
        }
      }
    }
  }


  uint32_t Protocol::errorOccurred(ServerPrepareResult *pr)
  {
    if (pr != nullptr) {
      return mysql_stmt_errno(pr->getStatementId());
    }
    else {
      return mysql_errno(connection.get());
    }
  }

  // I wonder why does it have to be in Protocol
  uint32_t Protocol::fieldCount(ServerPrepareResult *pr)
  {
    if (pr != nullptr)
    {
      return mysql_stmt_field_count(pr->getStatementId());
    }
    else
    {
      return mysql_field_count(connection.get());
    }
  }

  /**
   * Get current auto increment increment. *** no lock needed ****
   *
   * @return auto increment increment.
   * @throws SQLException if cannot retrieve auto increment value
   */
  int32_t Protocol::getAutoIncrementIncrement()
  {
    if (autoIncrementIncrement == 0) {
      std::lock_guard<std::mutex> localScopeLock(lock);
      try {
        Results results;
        executeQuery(&results, "SELECT @@auto_increment_increment");
        results.commandEnd();
        ResultSet* rs= results.getResultSet();
        rs->next();
        MYSQL_BIND bind;
        memset(&bind, 0, sizeof(MYSQL_BIND));
        bind.buffer_type= MYSQL_TYPE_LONG;
        bind.buffer= &autoIncrementIncrement;
        rs->get(&bind, 1, 0);
      } catch (SQLException& e) {
        if (e.getSQLState().compare(0, 2, "08", 2)){
          throw e;
        }
        autoIncrementIncrement= 1;
      }
    }
    return autoIncrementIncrement;
  }

  /**
   * Read error and does what it takes in case of error
   *
   * @param buffer current buffer
   * @param results result object
   * @return SQLException if sub-result connection fail
   */
  SQLException Protocol::processError(Results* results, ServerPrepareResult *pr)
  {
    removeHasMoreResults();
    this->hasWarningsFlag= false;

    int32_t errorNumber= errorOccurred(pr);
    SQLString message(mysql_error(connection.get()));
    SQLString sqlState(mysql_sqlstate(connection.get()));

    results->addStatsError(false);

    serverStatus|= SERVER_STATUS_IN_TRANS;

    removeActiveStreamingResult();
    return SQLException(message, sqlState, errorNumber);
  }

  /**
   * Reads ResultSet.
   *
   * @param buffer current buffer
   * @param results result object
   * @throws SQLException if sub-result connection fail
   * @see <a href="https://mariadb.com/kb/en/mariadb/resultset/">resultSet packets</a>
   */
  void Protocol::readResultSet(Results* results, ServerPrepareResult *pr)
  {
    try {
      ResultSet* selectResultSet;

      getServerStatus();
      bool callableResult= (serverStatus & SERVER_PS_OUT_PARAMS)!=0;

      if (pr == nullptr)
      {
        selectResultSet= ResultSet::create(results, this, connection.get());
      }
      else {
        pr->reReadColumnInfo();
        selectResultSet= ResultSet::create(results, this, pr, callableResult);

      }
      results->addResultSet(selectResultSet, hasMoreResults());
    }
    catch (SQLException & e) {
      throw e;
    }
  }

  /**
   * Preparation before command.
   *
   * @param maxRows query max rows
   * @param hasProxy has proxy
   * @param connection current connection
   * @param statement current statement
   * @throws SQLException if any error occur.
   */
  void Protocol::prolog(int64_t maxRows, bool hasProxy, PreparedStatement* statement)
  {
    if (explicitClosed){
      throw SQLException("execute() is called on closed connection", "08000");
    }
    setMaxRows(maxRows);
  }


  ServerPrepareResult* Protocol::addPrepareInCache(const SQLString& key, ServerPrepareResult* serverPrepareResult)
  {
    return serverPrepareStatementCache->put(key, serverPrepareResult);
  }


  void Protocol::unblockConnection()
  {
    if (auto activeStream= getActiveStreamingResult()) {
      activeStream->loadFully(false, this);
      activeStreamingResult= nullptr;
    }
  }


  void Protocol::cmdPrologue()
  {
    rc= 0;
    if (mustReset)
    {
      this->unsyncedReset();
      mustReset= false;
    }
    unblockConnection();
    forceReleaseWaitingPrepareStatement();

    if (!this->connected){
      throw SQLException("Connection* is closed", "08000", 1220);
    }
    interrupted= false;
  }

  void Protocol::cmdEpilog()
  {
    getServerStatus();
    if (sessionStateChanged())
      handleStateChange();
  }

  // TODO set all client affected variables when implementing CONJ-319
  void Protocol::resetStateAfterFailover(
      int64_t maxRows, enum IsolationLevel transactionIsolationLevel, const SQLString& database,bool autocommit)
  {
    setMaxRows(maxRows);

    if (transactionIsolationLevel != 0) {
      setTransactionIsolation(transactionIsolationLevel);
    }

    if (!database.empty() && !(getDatabase().compare(database) == 0)){
      setSchema(database);
    }

    if (getAutocommit() != autocommit){
      executeQuery(SQLString("SET AUTOCOMMIT=").append(autocommit ? "1" : "0"));
    }
  }


  void Protocol::interrupt()
  {
    interrupted= true;
  }


  bool Protocol::isInterrupted()
  {
    return interrupted;
  }

  /**
   * Throw TimeoutException if timeout has been reached.
   *
   * @throws SQLTimeoutException to indicate timeout exception.
   */
  void Protocol::stopIfInterrupted()
  {
    if (isInterrupted()){

      throw SQLException("Timeout during batch execution");
    }
  }


  void Protocol::closeSocket()
  {
    try {
      connection.reset();
    }
    catch (std::exception& ) {
    }
  }


  /** Closes connection and stream readers/writers Attempts graceful shutdown. */
  void Protocol::close()
  {
    std::unique_lock<std::mutex> localScopeLock(lock);
    this->connected= false;
    try {
      // skip acquires lock
      localScopeLock.unlock();
      skip();
    }
    catch (std::runtime_error& ) {
    }
    localScopeLock.lock();
    closeSocket();
    cleanMemory();
  }

  /** Force closes connection and stream readers/writers. */
  void Protocol::abort()
  {
    this->explicitClosed= true;
    bool lockStatus= false;
    // This isn't used anywhere, so leaving it for now
    lockStatus= lock.try_lock();
    this->connected= false;

    abortActiveStream();

    if (!lockStatus){
      //forceAbort();
    } else {
    }

    closeSocket();
    cleanMemory();

    if (lockStatus){
      lock.unlock();
    }
  }


  void Protocol::abortActiveStream()
  {
    try {
      Results* activeStream= getActiveStreamingResult();
      if (activeStream) {
        activeStream->abort();
        activeStreamingResult= nullptr;
      }
    }catch (std::runtime_error& ){

    }
  }

  /**
   * Skip packets not read that are not needed. Packets are read according to needs. If some data
   * have not been read before next execution, skip it. <i>Lock must be set before using this
   * method</i>
   *
   * @throws SQLException exception
   */
  void Protocol::skip()
  {
    Results* activeStream= getActiveStreamingResult();
    if (activeStream) {
      activeStream->loadFully(true, this);
      activeStreamingResult= nullptr;
    }
  }

  void Protocol::cleanMemory()
  {
    serverPrepareStatementCache->clear();
  }

  void Protocol::setServerStatus(uint32_t serverStatus)
  {
    this->serverStatus= serverStatus;
  }

  /** Remove flag has more results. */
  void Protocol::removeHasMoreResults()
  {
    if (hasMoreResults()){
      this->serverStatus= static_cast<int16_t>((serverStatus) ^ SERVER_MORE_RESULTS_EXIST);
    }
  }


  /** Closing connection in case of Connection error after connection creation. */
  void Protocol::destroySocket()
  {
    if (connection) {
      try {
        connection.reset();
      }catch (std::exception&){

      }
    }
  }


  /**
   * Is the connection closed.
   *
   * @return true if the connection is closed
   */
  bool Protocol::isClosed()
  {
    return !this->connected;
  }


  uint32_t Protocol::getServerStatus()
  {
    // Should it be really read each time? it's safer, but probably redundant
    mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_STATUS, (void*)&this->serverStatus);
    return serverStatus;
  }


  bool Protocol::mustBeMasterConnection()
  {
    return true;
  }

  bool Protocol::noBackslashEscapes()
  {
    return ((serverStatus & SERVER_STATUS_NO_BACKSLASH_ESCAPES) != 0);
  }


  const SQLString& Protocol::getServerVersion() const
  {
    return serverVersion;
  }


  bool Protocol::getReadonly() const
  {
    return readOnly;
  }


  void Protocol::setReadonly(bool readOnly)
  {
    this->readOnly= readOnly;
  }


  const SQLString& Protocol::getDatabase() const
  {
    return database;
  }

  void Protocol::parseVersion(const SQLString& _serverVersion)
  {
    size_t length= _serverVersion.length();
    char car;
    size_t offset= 0;
    int32_t type= 0;
    int32_t val= 0;

    for (;offset < length; offset++) {
      car= _serverVersion.at(offset);
      if (car < '0' || car > '9'){
        switch (type){
          case 0:
            majorVersion= val;
            break;
          case 1:
            minorVersion= val;
            break;
          case 2:
            patchVersion= val;
            return;
          default:
            break;
        }
        ++type;
        val= 0;
      }
      else {
        val= val*10 + car - 48;
      }
    }

    if (type == 2){
      patchVersion= val;
    }
  }


  uint32_t Protocol::getMajorServerVersion()
  {
    return majorVersion;
  }

  uint32_t Protocol::getMinorServerVersion()
  {
    return minorVersion;
  }

  uint32_t Protocol::getPatchServerVersion()
  {
    return patchVersion;
  }

  /**
   * Utility method to check if database version is greater than parameters.
   *
   * @param major major version
   * @param minor minor version
   * @param patch patch version
   * @return true if version is greater than parameters
   */
  bool Protocol::versionGreaterOrEqual(uint32_t major, uint32_t minor, uint32_t patch) const
  {
    if (this->majorVersion > major) {
      return true;
    }

    if (this->majorVersion < major) {
      return false;
    }

    /*
    * Major versions are equal, compare minor versions
    */
    if (this->minorVersion > minor) {
      return true;
    }
    if (this->minorVersion < minor) {
      return false;
    }

    // Minor versions are equal, compare patch version.
    return (this->patchVersion >= patch);
  }


  /**
   * Has warnings.
   *
   * @return true if as warnings.
   */
  bool Protocol::hasWarnings()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    return hasWarningsFlag;
  }

  /**
   * Is connected.
   *
   * @return true if connected
   */
  bool Protocol::isConnected()
  {
    if (connected && mysql_get_socket(getCHandle()) == MARIADB_INVALID_SOCKET) {
      connected= false;
    }
    return connected;
  }


  int64_t Protocol::getServerThreadId()
  {
    return mysql_thread_id(getCHandle());
  }

  /*Socket* Protocol::getSocket()
  {
    return mysql_get_socket(Dbc->mariadb) == MARIADB_INVALID_SOCKET;
  }*/

  bool Protocol::isExplicitClosed()
  {
    return explicitClosed;
  }


  void Protocol::setHasWarnings(bool _hasWarnings)
  {
    this->hasWarningsFlag= _hasWarnings;
  }

  Results* Protocol::getActiveStreamingResult()
  {
    return activeStreamingResult;
  }

  void Protocol::setActiveStreamingResult(Results* _activeStreamingResult)
  {
    this->activeStreamingResult= _activeStreamingResult;
  }

  /** Remove exception result and since totally fetched, set fetch size to 0. */
  void Protocol::removeActiveStreamingResult()
  {
    Results* activeStream= getActiveStreamingResult();
    if (activeStream) {
      activeStream->removeFetchSize();
      activeStreamingResult= nullptr;
    }
  }

  /* Probably this normally should not be used outside the class */
  std::mutex& Protocol::getLock()
  {
    return lock;
  }


  bool Protocol::hasMoreResults()
  {
    return (serverStatus & SERVER_MORE_RESULTS_EXIST) != 0;
  }

  bool Protocol::hasMoreResults(Results *results)
  {
    return results == activeStreamingResult && serverStatus & SERVER_MORE_RESULTS_EXIST;
  }


  bool Protocol::hasSpOutparams()
  {
    return (serverStatus & SERVER_PS_OUT_PARAMS) != 0;
  }


  Cache<std::string, ServerPrepareResult>* Protocol::prepareStatementCache()
  {
    return serverPrepareStatementCache.get();
  }


  void Protocol::changeReadTimeout(int32_t millis)
  {
    this->socketTimeout= millis;
    // Making seconds out of millies, as MYSQL_OPT_READ_TIMEOUT needs seconds
    millis= (millis + 999) / 1000;
    mysql_optionsv(connection.get(), MYSQL_OPT_READ_TIMEOUT, (void*)&millis);
  }

  bool Protocol::isServerMariaDb()
  {
    return serverMariaDb;
  }


  bool Protocol::sessionStateAware()
  {
    return (serverCapabilities & CLIENT_SESSION_TRACKING)!=0;
  }


  void Protocol::realQuery(const SQLString& sql)
  {
    if ((rc= mysql_real_query(connection.get(), sql.c_str(),
      static_cast<unsigned long>(sql.length())))) {
      throwConnError(getCHandle());
    }
  }

  void Protocol::realQuery(const char * query, std::size_t len)
  {
    auto con= connection.get();
    if (mysql_real_query(con, query, static_cast<unsigned long>(len))) {
      throw SQLException(mysql_error(con), mysql_sqlstate(con),
        mysql_errno(con));
    }
  }

  void Protocol::sendQuery(const SQLString & sql)
  {
    if (mysql_send_query(connection.get(), sql.c_str(), static_cast<unsigned long>(sql.length()))) {
      throw SQLException(mysql_error(connection.get()), mysql_sqlstate(connection.get()),
        mysql_errno(connection.get()));
    }
  }

  void Protocol::sendQuery(const char * query, std::size_t len)
  {
    if (mysql_send_query(connection.get(), query, static_cast<unsigned long>(len))) {
      throw SQLException(mysql_error(connection.get()), mysql_sqlstate(connection.get()),
        mysql_errno(connection.get()));
    }
  }

  void Protocol::readQueryResult()
  {
    auto con= connection.get();
    if (mysql_read_query_result(con)) {
      throw SQLException(mysql_error(con), mysql_sqlstate(con), mysql_errno(con));
    }
  }


  void Protocol::commitReturnAutocommit(bool justReadMultiSendResults)
  {
    if (justReadMultiSendResults) {
      readQueryResult();//COMMIT
      readQueryResult();//SET AUTOCOMMIT=1
    }
    else {
      CONST_QUERY("COMMIT");
      CONST_QUERY("SET AUTOCOMMIT=1");
    }
    // Need to get autocommit returned to the stored serverstatus
    mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_STATUS, (void*)&this->serverStatus);
  }

  /* Same as realQuery, but loads pending results, and tracks session, if needed. i.e. with standard
     prolog and epilog, but not synced */
  void Protocol::safeRealQuery(const SQLString& sql)
  {
    cmdPrologue();
    realQuery(sql);
    cmdEpilog();
  }

  /**
 * Set transaction isolation.
 *
 * @param level transaction level.
 * @throws SQLException if transaction level is unknown
 */
  void Protocol::setTransactionIsolation(enum IsolationLevel level)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    cmdPrologue();
    SQLString query("SET SESSION TRANSACTION ISOLATION LEVEL ");

    realQuery(addTxIsolationName2Query(query, level));

    transactionIsolationLevel= static_cast<enum IsolationLevel>(level);
  }
}
