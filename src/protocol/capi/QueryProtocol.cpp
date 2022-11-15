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


#include "QueryProtocol.h"

#include "logger/LoggerFactory.h"
#include "Results.h"
#include "util/LogQueryTool.h"
#include "util/ClientPrepareResult.h"
#include "util/ServerPrepareResult.h"
#include "util/ServerPrepareStatementCache.h"
#include "util/StateChange.h"
#include "util/Utils.h"
#include "protocol/MasterProtocol.h"
#include "SqlStates.h"
#include "com/capi/ColumnDefinitionCapi.h"
#include "ExceptionFactory.h"
#include "util/ServerStatus.h"
//I guess eventually it should go from here
#include "com/Packet.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysqld_error.h"

  static const int64_t MAX_PACKET_LENGTH= 0x00ffffff + 4;

  const Shared::Logger QueryProtocol::logger= LoggerFactory::getLogger(typeid(QueryProtocol));
  const SQLString QueryProtocol::CHECK_GALERA_STATE_QUERY("show status like 'wsrep_local_state'");

  void throwStmtError(MYSQL_STMT* stmt) {
    SQLString err(mysql_stmt_error(stmt)), sqlState(mysql_stmt_sqlstate(stmt));
    uint32_t errNo = mysql_stmt_errno(stmt);
    // Actually, this can be a good idea to close the stmt right away
    throw SQLException(err, sqlState, errNo);
  }

  /**
   * Get a protocol instance.
   *
   * @param urlParser connection URL information's
   * @param lock the lock for thread synchronisation
   */
  QueryProtocol::QueryProtocol(std::shared_ptr<UrlParser>& urlParser, GlobalStateInfo* globalInfo, Shared::mutex& lock)
    : super(urlParser, globalInfo, lock)
    , logQuery(new LogQueryTool(options))
    , activeFutureTask(nullptr)
    , statementIdToRelease(nullptr)
    , maxRows(0)
    , transactionIsolationLevel(0)
  {
    if (!urlParser->getOptions()->galeraAllowedState.empty())
    {
      galeraAllowedStates= split(urlParser->getOptions()->galeraAllowedState, ",");
    }
  }

  void QueryProtocol::reset()
  {
    cmdPrologue();
    try {

      if (mysql_reset_connection(connection.get()))
      {
        throw SQLException("Connection reset failed");
      }

      if (options->cachePrepStmts && options->useServerPrepStmts){
        //serverPrepareStatementCache->clear();
      }

    }catch (SQLException& sqlException){
      throw logQuery->exceptionWithQuery("COM_RESET_CONNECTION failed.", sqlException, explicitClosed);
    }catch (std::runtime_error& e){
      handleIoException(e).Throw();
    }
  }

  /**
   * Execute internal query.
   *
   * <p>!! will not support multi values queries !!
   *
   * @param sql sql
   * @throws SQLException in any exception occur
   */
  void QueryProtocol::executeQuery(const SQLString& sql)
  {
    Shared::Results res(new Results());
    executeQuery(isMasterConnection(), res, sql);
  }

  void QueryProtocol::executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql)
  {

    cmdPrologue();
    try {

      realQuery(sql);
      getResult(results.get());

    }catch (SQLException& sqlException){
      if (sqlException.getSQLState().compare("70100") == 0 && 1927 == sqlException.getErrorCode()){
        throw sqlException;
      }
      throw logQuery->exceptionWithQuery(sql, sqlException, explicitClosed);
    }catch (std::runtime_error& e){
      handleIoException(e).Throw();
    }
  }

  void QueryProtocol::executeQuery( bool mustExecuteOnMaster, Shared::Results& results,const SQLString& sql, const Charset* charset)
  {
    cmdPrologue();
    try {

      realQuery(sql);
      getResult(results.get());

    }catch (SQLException& sqlException){
      throw logQuery->exceptionWithQuery(sql, sqlException, explicitClosed);
    }catch (std::runtime_error& e){
      handleIoException(e).Throw();
    }
  }

  /**
   * Execute a unique clientPrepareQuery.
   *
   * @param mustExecuteOnMaster was intended to be launched on master connection
   * @param results results
   * @param clientPrepareResult clientPrepareResult
   * @param parameters parameters
   * @throws SQLException exception
   */
  void QueryProtocol::executeQuery(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<Shared::ParameterHolder>& parameters)
  {
    executeQuery(mustExecuteOnMaster, results, clientPrepareResult, parameters, -1);
  }

  SQLString& addQueryTimeout(SQLString& sql, int32_t queryTimeout)
  {
    if (queryTimeout > 0) {
      sql.append("SET STATEMENT max_statement_time=" + std::to_string(queryTimeout) + " FOR ");
    }
    return sql;
  }

  void assemblePreparedQueryForExec(
    SQLString& out,
    ClientPrepareResult* clientPrepareResult,
    std::vector<Shared::ParameterHolder>& parameters,
    int32_t queryTimeout)
  {
    addQueryTimeout(out, queryTimeout);

    const std::vector<SQLString> &queryPart= clientPrepareResult->getQueryParts();

    if (clientPrepareResult->isRewriteType()) {

      out.append(queryPart[0]);
      out.append(queryPart[1]);

      for (uint32_t i = 0; i < clientPrepareResult->getParamCount(); i++) {
        parameters[i]->writeTo(out);
        out.append(queryPart[i + 2]);
      }
      out.append(queryPart[clientPrepareResult->getParamCount() + 2]);

    }
    else {

      out.append(queryPart.front());
      for (uint32_t i = 0; i < clientPrepareResult->getParamCount(); i++) {
        parameters[i]->writeTo(out);
        out.append(queryPart[i + 1]);
      }
    }
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
  void QueryProtocol::executeQuery(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<Shared::ParameterHolder>& parameters,
      int32_t queryTimeout)
  {
    cmdPrologue();

    SQLString sql;
    addQueryTimeout(sql, queryTimeout);

    try {

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
      getResult(results.get());

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
  bool QueryProtocol::executeBatchClient(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* prepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList,
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

    if (options->rewriteBatchedStatements){
      if (prepareResult->isQueryMultiValuesRewritable()
        && results->getAutoGeneratedKeys() == Statement::NO_GENERATED_KEYS){
        // values rewritten in one query :
        // INSERT INTO X(a,b) VALUES (1,2), (3,4), ...
        executeBatchRewrite(results, prepareResult, parametersList, true);
        return true;

      }else if (prepareResult->isQueryMultipleRewritable()){

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
        && executeBulkBatch(results,prepareResult->getSql(),nullptr,parametersList)){
      return true;
    }

    if (options->useBatchMultiSend){

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
  bool QueryProtocol::executeBulkBatch(
      Shared::Results& results, const SQLString& origSql,
      ServerPrepareResult* serverPrepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList)
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
    std::vector<Shared::ParameterHolder> initParameters= parametersList.front();
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
    if (StringImp::get(lcCopy.toLowerCase()).find("select") != std::string::npos) {
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

      // **************************************************************************************
      // send BULK
      // **************************************************************************************
      capi::MYSQL_STMT* statementId= tmpServerPrepareResult ? tmpServerPrepareResult->getStatementId() : nullptr;

      //TODO: shouldn't throw if stmt is NULL? Returning false so far.
      if (statementId == nullptr)
      {
        return false;
      }
      unsigned int bulkArrSize= static_cast<unsigned int>(parametersList.size());

      capi::mysql_stmt_attr_set(statementId, STMT_ATTR_ARRAY_SIZE, (const void*)&bulkArrSize);
      auto firstParameters= parametersList.front();

      tmpServerPrepareResult->bindParameters(parametersList, types.data());
      capi::mysql_stmt_execute(statementId);

      try {
        getResult(results.get(), tmpServerPrepareResult);
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

      if (!exception.getMessage().empty()){
        throw exception;
      }
      results->setRewritten(true);
      
      if (!serverPrepareResult && tmpServerPrepareResult){
      releasePrepareStatement(tmpServerPrepareResult);
      }
      return true;

    }catch (std::runtime_error& e){
      if (!serverPrepareResult && tmpServerPrepareResult) {
        releasePrepareStatement(tmpServerPrepareResult);
      }
      handleIoException(e).Throw();
    }
    //To please compilers etc
    return false;
  }

  void QueryProtocol::initializeBatchReader()
  {
    if (options->useBatchMultiSend){
      //readScheduler= SchedulerServiceProviderHolder::getBulkScheduler();
    }
  }

  /**
   * Execute clientPrepareQuery batch.
   *
   * @param results results
   * @param clientPrepareResult ClientPrepareResult
   * @param parametersList List of parameters
   * @throws SQLException exception
   */
  void QueryProtocol::executeBatchMulti(
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList)

  {

    cmdPrologue();
    initializeBatchReader();

    SQLString sql;

    for (auto& parameters : parametersList)
    {
      sql.clear();

      assemblePreparedQueryForExec(sql, clientPrepareResult, parameters, -1);
      realQuery(sql);
      getResult(results.get());
    }
  }

  /**
   * Execute batch from Statement.executeBatch().
   *
   * @param mustExecuteOnMaster was intended to be launched on master connection
   * @param results results
   * @param queries queries
   * @throws SQLException if any exception occur
   */
  void QueryProtocol::executeBatchStmt(bool mustExecuteOnMaster, Shared::Results& results, const std::vector<SQLString>& queries)
  {
    cmdPrologue();
    if (this->options->rewriteBatchedStatements){

      // check that queries are rewritable
      bool canAggregateSemiColumn= true;
      std::size_t totalLen= 0;
      for (SQLString query : queries){
        if (!ClientPrepareResult::canAggregateSemiColon(query,noBackslashEscapes())){
          canAggregateSemiColumn= false;
          break;
        }
        totalLen+= query.length() + 1/*;*/;
      }

      if (isInterrupted()){
        // interrupted by timeout, must throw an exception manually
        throw SQLTimeoutException("Timeout during batch execution", "00000");
      }

      if (canAggregateSemiColumn){
        executeBatchAggregateSemiColon(results, queries, totalLen);
      }else {
        executeBatch(results,queries);
      }

    }else {
      executeBatch(results,queries);
    }
  }

  /**
   * Execute list of queries not rewritable.
   *
   * @param results result object
   * @param queries list of queries
   * @throws SQLException exception
   */
  void QueryProtocol::executeBatch(Shared::Results& results, const std::vector<SQLString>& queries)
  {
    if (!options->useBatchMultiSend){

      MariaDBExceptionThrower exception;

      for (auto& sql : queries) {
        try {
          realQuery(sql);
          getResult(results.get());

        }catch (SQLException& sqlException){
          if (!exception){
            SQLException ex(logQuery->exceptionWithQuery(sql, sqlException, explicitClosed));
            exception.take(ex);
            if (!options->continueBatchOnError){
              exception.Throw();
            }
          }
        }catch (std::runtime_error& e){
          if (!exception){
            exception.assign(handleIoException(e, false));
            if (!options->continueBatchOnError){
              exception.Throw();
            }
          }
        }
      }
      stopIfInterrupted();

      if (exception){
        exception.Throw();
      }
      return;
    }
    initializeBatchReader();

    for (auto& query : queries)
    {
      realQuery(query);
      getResult(results.get());
    }
  }


  ServerPrepareResult* QueryProtocol::prepareInternal(const SQLString& sql, bool executeOnMaster)
  {
    if (options->cachePrepStmts && options->useServerPrepStmts) {

      ServerPrepareResult* pr = serverPrepareStatementCache->get(database + "-" + sql);

      if (pr && pr->incrementShareCounter()) {
        return pr;
      }
    }

    capi::MYSQL_STMT* stmtId = capi::mysql_stmt_init(connection.get());

    if (stmtId == nullptr)
    {
      throw SQLException(capi::mysql_error(connection.get()), capi::mysql_sqlstate(connection.get()), capi::mysql_errno(connection.get()));
    }

    static const my_bool updateMaxLength= 1;

    capi::mysql_stmt_attr_set(stmtId, STMT_ATTR_UPDATE_MAX_LENGTH, &updateMaxLength);

    if (capi::mysql_stmt_prepare(stmtId, sql.c_str(), static_cast<unsigned long>(sql.length())))
    {
      SQLString err(mysql_stmt_error(stmtId)), sqlState(mysql_stmt_sqlstate(stmtId));
      uint32_t errNo = mysql_stmt_errno(stmtId);

      capi::mysql_stmt_close(stmtId);
      throw SQLException(err, sqlState, errNo);
    }

    ServerPrepareResult* res= new ServerPrepareResult(sql, stmtId, this);

    if (options->cachePrepStmts
      && options->useServerPrepStmts
      && sql.length() < static_cast<size_t>(options->prepStmtCacheSqlLimit)) {
      SQLString key(getDatabase() + "-" + sql);

      ServerPrepareResult* cachedServerPrepareResult= addPrepareInCache(key, res);

      if (cachedServerPrepareResult != nullptr)
      {
        delete res;
        res= cachedServerPrepareResult;
      }
    }
    return res;
  }


  ServerPrepareResult* QueryProtocol::prepare(const SQLString& sql,bool executeOnMaster)
  {
    cmdPrologue();
    std::unique_ptr<std::lock_guard<std::mutex>> localScopeLock;

    return prepareInternal(sql, executeOnMaster);
  }


  bool checkRemainingSize(int64_t newQueryLen)
  {
    return newQueryLen < MAX_PACKET_LENGTH;
  }


  size_t assembleBatchAggregateSemiColonQuery(SQLString& sql, const SQLString &firstSql, const std::vector<SQLString>& queries, size_t currentIndex)
  {
    sql.append(firstSql);

    // add query with ";"
    while (currentIndex < queries.size()) {

      if (!checkRemainingSize(sql.length() + queries[currentIndex].length() + 1)) {
        break;
      }
      sql.append(';').append(queries[currentIndex]);
      ++currentIndex;
    }

    return currentIndex;
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
  void QueryProtocol::executeBatchAggregateSemiColon(Shared::Results& results, const std::vector<SQLString>& queries, std::size_t totalLenEstimation)
  {

    SQLString firstSql;
    size_t currentIndex= 0;
    size_t totalQueries= queries.size();
    SQLException exception;
    SQLString sql;

    do {

      try {

        firstSql= queries[currentIndex++];
        if (totalLenEstimation == 0) {
          totalLenEstimation= firstSql.length()*queries.size() + queries.size() - 1;
        }
        sql.reserve(((std::min<int64_t>(MAX_PACKET_LENGTH, totalLenEstimation) + 7) / 8) * 8);
        currentIndex= assembleBatchAggregateSemiColonQuery(sql, firstSql, queries, currentIndex);
        realQuery(sql);
        sql.clear(); // clear is not supposed to release memory

        getResult(results.get(), nullptr, true);

      }catch (SQLException& sqlException){
        if (exception.getMessage().empty()){
          exception= logQuery->exceptionWithQuery(firstSql, sqlException, explicitClosed);
          if (!options->continueBatchOnError){
            throw exception;
          }
        }
      }catch (std::runtime_error& e){
        handleIoException(e).Throw();
      }
      stopIfInterrupted();

    }while (currentIndex < totalQueries);

    if (!exception.getMessage().empty()) {
      throw exception;
    }
  }


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
    std::vector<std::vector<Shared::ParameterHolder>> &parameterList,
    bool rewriteValues)

  {
    std::size_t index= currentIndex;
    std::vector<Shared::ParameterHolder> &parameters= parameterList[index++];

    const SQLString &firstPart= queryParts[1];
    const SQLString &secondPart= queryParts[0];

    if (!rewriteValues) {

      pos.append(firstPart);
      pos.append(secondPart);

      std::size_t staticLength= 1;
      for (auto& queryPart : queryParts) {
        staticLength+= queryPart.length();
      }

      for (size_t i= 0; i < paramCount; i++) {
        parameters[i]->writeTo(pos);
        pos.append(queryParts[i +2]);
      }
      pos.append(queryParts[paramCount +2]);


      while (index <parameterList.size()) {
        parameters= parameterList[index];


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
            pos.append(queryParts[paramCount +2]);
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
          pos.append(queryParts[paramCount +2]);
          ++index;
          break;
        }
      }

    }
    else {
      pos.append(firstPart);
      pos.append(secondPart);
      size_t lastPartLength= queryParts[paramCount +2].length();
      size_t intermediatePartLength= queryParts[1].length();

      for (size_t i= 0; i <paramCount; i++) {
        parameters[i]->writeTo(pos);
        pos.append(queryParts[i +2]);
        intermediatePartLength +=queryParts[i +2].length();
      }

      while (index <parameterList.size()) {
        parameters= parameterList[index];


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
  void QueryProtocol::executeBatchRewrite(
      Shared::Results& results,
      ClientPrepareResult* prepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parameterList,
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
        getResult(results.get(), nullptr, !rewriteValues);

#ifdef THE_TIME_HAS_COME
        if (Thread.currentThread().isInterrupted()){
          throw SQLException(
              "Interrupted during batch",INTERRUPTED_EXCEPTION.getSqlState(),-1);
        }
#endif
      } while (currentIndex < totalParameterList);

    }catch (SQLException& sqlEx){
      throw logQuery->exceptionWithQuery(sqlEx,prepareResult);
    }catch (std::runtime_error& e){
      handleIoException(e).Throw();
    }/* TODO: something with the finally was once here */ {
      results->setRewritten(rewriteValues);
    }
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
  bool QueryProtocol::executeBatchServer(
      bool mustExecuteOnMaster,
      ServerPrepareResult* serverPrepareResult,
      Shared::Results& results, const SQLString& sql,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList,
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

    capi::MYSQL_STMT *stmt= nullptr;

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


  void QueryProtocol::executePreparedQuery(
      bool mustExecuteOnMaster,
      ServerPrepareResult* serverPrepareResult,
      Shared::Results& results,
      std::vector<Shared::ParameterHolder>& parameters)
  {

    cmdPrologue();

    try {
      std::unique_ptr<sql::bytes> ldBuffer;
      uint32_t bytesInBuffer;

      serverPrepareResult->bindParameters(parameters);

      for (uint32_t i= 0; i < serverPrepareResult->getParameters().size(); i++){
        if (parameters[i]->isLongData()){
          if (!ldBuffer)
          {
            ldBuffer.reset(new sql::bytes(MAX_PACKET_LENGTH - 4));
          }

          while ((bytesInBuffer= parameters[i]->writeBinary(*ldBuffer)) > 0)
          {
            capi::mysql_stmt_send_long_data(serverPrepareResult->getStatementId(), i, ldBuffer->arr, bytesInBuffer);
          }
        }
      }

      if (capi::mysql_stmt_execute(serverPrepareResult->getStatementId()) != 0) {
        throwStmtError(serverPrepareResult->getStatementId());
      }
      /*CURSOR_TYPE_NO_CURSOR);*/
      getResult(results.get(), serverPrepareResult);

    }catch (SQLException& qex){
      throw logQuery->exceptionWithQuery(parameters, qex, serverPrepareResult);
    }catch (std::runtime_error& e){
      handleIoException(e).Throw();
    }
  }

  /** Rollback transaction. */
  void QueryProtocol::rollback()
  {
    cmdPrologue();

    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {

      if (inTransaction()){
        executeQuery("ROLLBACK");
      }

    } catch (std::runtime_error&){
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
  bool QueryProtocol::forceReleasePrepareStatement(MYSQL_STMT* statementId)
  {

    if (lock->try_lock()) {
      checkClose();

      if (mysql_stmt_close(statementId))
      {
        connected= false;
        lock->unlock();
        throw SQLException(
            "Could not deallocate query",
            CONNECTION_EXCEPTION.getSqlState().c_str());
      }
      lock->unlock();
      return true;

    }else {
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
  void QueryProtocol::forceReleaseWaitingPrepareStatement()
  {
    if (statementIdToRelease != nullptr && forceReleasePrepareStatement(statementIdToRelease)){
      statementIdToRelease= nullptr;
    }
  }


  bool QueryProtocol::ping()
  {

    cmdPrologue();
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {

      return mysql_ping(connection.get()) == 0;

    }catch (std::runtime_error& e){
      connected= false;
      throw SQLException(SQLString("Could not ping: ").append(e.what()), CONNECTION_EXCEPTION.getSqlState(), 0, &e);
    }
  }


  bool QueryProtocol::isValid(int32_t timeout)
  {

    int32_t initialTimeout= -1;
    try {
      initialTimeout= this->socketTimeout;
      if (initialTimeout == 0){
        this->changeSocketSoTimeout(timeout);
      }
      if (isMasterConnection() && galeraAllowedStates->size() != 0){


        Shared::Results results(new Results());
        executeQuery(true, results, CHECK_GALERA_STATE_QUERY);
        results->commandEnd();
        ResultSet* rs= results->getResultSet();

        if (rs && rs->next())
        {
          SQLString statusVal(rs->getString(2));
          auto cit= galeraAllowedStates->cbegin();

          for (; cit != galeraAllowedStates->end(); ++cit)
          {
            if (cit->compare(statusVal) == 0)
            {
              break;
            }
          }
          return (cit != galeraAllowedStates->end());
        }
        return false;
      }

      return ping();

    }catch (/*SocketException*/std::runtime_error& socketException){
      logger->trace(SQLString("Connection* is not valid").append(socketException.what()));
      connected= false;
      return false;
    }
    /* TODO: something with the finally was once here */
    /*{

      try {
        if (initialTimeout != -1){
          this->changeSocketSoTimeout(initialTimeout);
        }
      }catch (std::runtime_error& socketException){
        logger->warn("Could not set socket timeout back to " + std::to_string(initialTimeout) + socketException.what());
        connected= false;

      }
    }*/
  }

  SQLString QueryProtocol::getCatalog()
  {

    if ((serverCapabilities & MariaDbServerCapabilities::CLIENT_SESSION_TRACK)!=0){

      return database;
    }

    cmdPrologue();
    std::lock_guard<std::mutex> localScopeLock(*lock);

    Shared::Results results(new Results());
    executeQuery(isMasterConnection(), results, "select database()");
    results->commandEnd();
    ResultSet* rs= results->getResultSet();
    if (rs->next()){
      this->database= rs->getString(1);
      return database;
    }
    return nullptr;

  }

  void QueryProtocol::setCatalog(const SQLString& _database)
  {

    cmdPrologue();

    std::unique_lock<std::mutex> localScopeLock(*lock);

    if (capi::mysql_select_db(connection.get(), _database.c_str()) != 0) {
      // TODO: realQuery should throw. Here we could catch and change message
      if (mysql_get_socket(connection.get()) == MARIADB_INVALID_SOCKET) {
        std::string msg("Connection lost: ");
        msg.append(mysql_error(connection.get()));
        std::runtime_error e(msg.c_str());
        localScopeLock.unlock();
        throw logQuery->exceptionWithQuery("COM_INIT_DB", *handleIoException(e, false).getException(), false);
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

  void QueryProtocol::resetDatabase()
  {
    if (!(database.compare(urlParser->getDatabase()) == 0)){
      setCatalog(urlParser->getDatabase());
    }
  }

  void QueryProtocol::cancelCurrentQuery()
  {
    Shared::mutex newMutex(new std::mutex());
    std::unique_ptr<MasterProtocol> copiedProtocol(
      new MasterProtocol(urlParser, new GlobalStateInfo(), newMutex));

    copiedProtocol->setHostAddress(getHostAddress());
    copiedProtocol->connect();

    copiedProtocol->executeQuery("KILL QUERY " + std::to_string(serverThreadId));

    interrupted= true;
  }

  bool QueryProtocol::getAutocommit()
  {
    return ((serverStatus & ServerStatus::AUTOCOMMIT)!=0);
  }

  bool QueryProtocol::inTransaction()
  {
    return ((serverStatus &  ServerStatus::IN_TRANSACTION)!=0);
  }


  void QueryProtocol::closeExplicit()
  {
    this->explicitClosed= true;
    close();
  }


  void QueryProtocol::releasePrepareStatement(ServerPrepareResult* serverPrepareResult)
  {
    // If prepared cache is enabled, the ServerPrepareResult can be shared in many PrepStatement,
    // so synchronised use count indicator will be decrement.
    serverPrepareResult->decrementShareCounter();

    // deallocate from server if not cached
    if (serverPrepareResult->canBeDeallocate()){
      forceReleasePrepareStatement(serverPrepareResult->getStatementId());
    }
  }


  int64_t QueryProtocol::getMaxRows()
  {
    return maxRows;
  }


  void QueryProtocol::setMaxRows(int64_t max)
  {
    if (maxRows != max){
      if (max == 0){
        executeQuery("set @@SQL_SELECT_LIMIT=DEFAULT");
      }else {
        executeQuery("set @@SQL_SELECT_LIMIT=" + std::to_string(max));
      }
      maxRows= max;
    }
  }


  void QueryProtocol::setLocalInfileInputStream(std::istream& inputStream)
  {
    this->localInfileInputStream.reset(&inputStream);
  }


  int32_t QueryProtocol::getTimeout()
  {
    return this->socketTimeout;
  }


  void QueryProtocol::setTimeout(int32_t timeout)
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);

    this->changeSocketSoTimeout(timeout);
  }

  /**
   * Set transaction isolation.
   *
   * @param level transaction level.
   * @throws SQLException if transaction level is unknown
   */
  void QueryProtocol::setTransactionIsolation(int32_t level)
  {
    cmdPrologue();
    std::lock_guard<std::mutex> localScopeLock(*lock);

    SQLString query= "SET SESSION TRANSACTION ISOLATION LEVEL";

    switch (level){
      case sql::TRANSACTION_READ_UNCOMMITTED:
        query.append(" READ UNCOMMITTED");
        break;
      case sql::TRANSACTION_READ_COMMITTED:
        query.append(" READ COMMITTED");
        break;
      case sql::TRANSACTION_REPEATABLE_READ:
        query.append(" REPEATABLE READ");
        break;
      case sql::TRANSACTION_SERIALIZABLE:
        query.append(" SERIALIZABLE");
        break;
      default:
        throw SQLException("Unsupported transaction isolation level");
    }

    executeQuery(query);
    transactionIsolationLevel= level;
  }


  int32_t QueryProtocol::getTransactionIsolationLevel()
  {
    return transactionIsolationLevel;
  }


  void QueryProtocol::checkClose()
  {
    if (!this->connected){
      throw SQLException("Connection* is close","08000",1220);
    }
  }


  void QueryProtocol::moveToNextResult(Results* results, ServerPrepareResult* spr)
  {
    int32_t res;
    if (spr != nullptr) {
      res= capi::mysql_stmt_next_result(spr->getStatementId());
    }
    else {
      res= capi::mysql_next_result(connection.get());
    }

    if (res != 0) {
      readErrorPacket(results, spr);
    }
  }

  void QueryProtocol::getResult(Results* results, ServerPrepareResult *pr, bool readAllResults)
  {
    readPacket(results, pr);

    if (readAllResults) {
      while (hasMoreResults()) {
        moveToNextResult(results, pr);
        readPacket(results, pr);
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
  void QueryProtocol::readPacket(Results* results, ServerPrepareResult *pr)
  {
    switch (errorOccurred(pr))
    {
      case 0:
        if (fieldCount(pr) == 0)
        {
          readOkPacket(results, pr);
        }
        else
        {
          readResultSet(results, pr);
        }
        break;

      default:
        throw readErrorPacket(results, pr);
    }
  }

  /**
   * Read OK_Packet.
   *
   * @param buffer current buffer
   * @param results result object
   * @see <a href="https://mariadb.com/kb/en/mariadb/ok_packet/">OK_Packet</a>
   */
  void QueryProtocol::readOkPacket(Results* results, ServerPrepareResult *pr)
  {
    const int64_t updateCount= (pr == nullptr ? capi::mysql_affected_rows(connection.get()) : capi::mysql_stmt_affected_rows(pr->getStatementId()));
    const int64_t insertId= (pr == nullptr ? capi::mysql_insert_id(connection.get()) : capi::mysql_stmt_insert_id(pr->getStatementId()));

    capi::mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_STATUS, (void*)&this->serverStatus);
    hasWarningsFlag= capi::mysql_warning_count(connection.get()) > 0;

    if ((serverStatus & ServerStatus::SERVER_SESSION_STATE_CHANGED_)!=0){
      handleStateChange(results);
    }

    results->addStats(updateCount, insertId, hasMoreResults());
  }

  void QueryProtocol::handleStateChange(Results* results)
  {
    const char *value;
    size_t len;

    for (int32_t type=SESSION_TRACK_BEGIN; type < SESSION_TRACK_END; ++type)
    {
      if (mysql_session_track_get_first(connection.get(), static_cast<enum capi::enum_session_state_type>(type), &value, &len) == 0)
      {
        std::string str(value, len);

        switch (type) {
        case StateChange::SESSION_TRACK_SYSTEM_VARIABLES:
          if (str.compare("auto_increment_increment") == 0)
          {
            autoIncrementIncrement= std::stoi(str);
            results->setAutoIncrement(autoIncrementIncrement);
          }
          break;

        case StateChange::SESSION_TRACK_SCHEMA:
          database= str;
          logger->debug("Database change : now is '" + database + "'");
          break;

        default:
          break;
        }
      }
    }
  }

  uint32_t capi::QueryProtocol::errorOccurred(ServerPrepareResult * pr)
  {
    if (pr != nullptr) {
      return capi::mysql_stmt_errno(pr->getStatementId());
    }
    else {
      return capi::mysql_errno(connection.get());
    }
  }

  uint32_t QueryProtocol::fieldCount(ServerPrepareResult * pr)
  {
    if (pr != nullptr)
    {
      return capi::mysql_stmt_field_count(pr->getStatementId());
    }
    else
    {
      return capi::mysql_field_count(connection.get());
    }
  }

  /**
   * Get current auto increment increment. *** no lock needed ****
   *
   * @return auto increment increment.
   * @throws SQLException if cannot retrieve auto increment value
   */
  int32_t QueryProtocol::getAutoIncrementIncrement()
  {
    if (autoIncrementIncrement == 0) {
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {
        Shared::Results results(new Results());
        executeQuery(true, results,"select @@auto_increment_increment");
        results->commandEnd();
        ResultSet* rs= results->getResultSet();
        rs->next();
        autoIncrementIncrement= rs->getInt(1);
      }catch (SQLException& e){
        if (e.getSQLState().startsWith("08")){
          throw e;
        }
        autoIncrementIncrement= 1;
      }
    }
    return autoIncrementIncrement;
  }

  /**
   * Read ERR_Packet.
   *
   * @param buffer current buffer
   * @param results result object
   * @return SQLException if sub-result connection fail
   * @see <a href="https://mariadb.com/kb/en/mariadb/err_packet/">ERR_Packet</a>
   */
  SQLException QueryProtocol::readErrorPacket(Results* results, ServerPrepareResult *pr)
  {
    removeHasMoreResults();
    this->hasWarningsFlag= false;

    int32_t errorNumber= errorOccurred(pr);
    SQLString message(mysql_error(connection.get()));
    SQLString sqlState(mysql_sqlstate(connection.get()));

    results->addStatsError(false);

    serverStatus |= ServerStatus::IN_TRANSACTION;

    removeActiveStreamingResult();
    return SQLException(message, sqlState, errorNumber);
  }

  /**
   * Read Local_infile Packet.
   *
   * @param buffer current buffer
   * @param results result object
   * @throws SQLException if sub-result connection fail
   * @see <a href="https://mariadb.com/kb/en/mariadb/local_infile-packet/">local_infile packet</a>
   */
  void QueryProtocol::readLocalInfilePacket(Shared::Results& results)
  {

#ifdef UNLIKELY_WE_HAVE_TO_DO_ANYTHING_HERE_CARED_BY_CAPI
    int32_t seq= 2;
    SQLString fileName= buffer->readStringNullEnd(StandardCharsets.UTF_8);
    try {

      std::ifstream is;
      writer.startPacket(seq);
      if (localInfileInputStream/*.empty() == true*/){

        if (!getUrlParser().getOptions()->allowLocalInfile){
          writer.writeEmptyPacket();
          reader.getPacket(true);
          throw SQLException(
              "Usage of LOCAL INFILE is disabled. To use it enable it via the connection property allowLocalInfile=true",
              FEATURE_NOT_SUPPORTED.getSqlState(),
              -1);
        }


        ServiceLoader<LocalInfileInterceptor> loader =
          ServiceLoader::load(typeid(LocalInfileInterceptor));
        for (LocalInfileInterceptor interceptor :loader){
          if (!interceptor.validate(fileName)){
            writer.writeEmptyPacket();
            reader.getPacket(true);
            throw SQLException(
                "LOAD DATA LOCAL INFILE request to send local file named \""
                +fileName
                +"\" not validated by interceptor \""
                +interceptor.getClass().getName()
                +"\"");
          }
        }

        if (results->getSql().empty()){
          writer.writeEmptyPacket();
          reader.getPacket(true);
          throw SQLException(
              "LOAD DATA LOCAL INFILE not permit in batch. file '"+fileName +"'",
              SqlStates.INVALID_AUTHORIZATION.getSqlState(),
              -1);

        }else if (!Utils::validateFileName(results->getSql(),results->getParameters(),fileName)){
          writer.writeEmptyPacket();
          reader.getPacket(true);
          throw SQLException(
              "LOAD DATA LOCAL INFILE asked for file '"
              +fileName
              +"' that doesn't correspond to initial query "
              +results->getSql()
              +". Possible malicious proxy changing server answer ! Command interrupted",
              SqlStates::INVALID_AUTHORIZATION.getSqlState(),
              -1);
        }

        try {
          URL url(fileName);
          is= url.openStream();
        }catch (std::runtime_error& ioe){
          try {
            is= new FileInputStream(fileName);
          }catch (std::runtime_error& ioe){
            writer.writeEmptyPacket();
            reader.getPacket(true);
            throw SQLException("Could not send file : "+f.getMessage(),"22000",-1,f);
          }
        }
      }else {
        is= localInfileInputStream;
        localInfileInputStream= nullptr;
      }

      try {

        char buf[8192];
        int32_t len;
        while ((len= is.read(buf))>0){
          writer.startPacket(seq++);
          writer.write(buf,0,len);
          writer.flush();
        }
        writer.writeEmptyPacket();

      }catch (std::runtime_error& ioe){
        handleIoException(ioe).Throw();
      }/* TODO: something with the finally was once here */ {
        is.close();
      }

      getResult(results.get());

    }catch (std::runtime_error& ioe){
      handleIoException(e.Throw();
    }
#endif

  }

  /**
   * Read ResultSet Packet.
   *
   * @param buffer current buffer
   * @param results result object
   * @throws SQLException if sub-result connection fail
   * @see <a href="https://mariadb.com/kb/en/mariadb/resultset/">resultSet packets</a>
   */
  void QueryProtocol::readResultSet(Results* results, ServerPrepareResult *pr)
  {
    try {

      SelectResultSet* selectResultSet;

      capi::mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_STATUS, (void*)&this->serverStatus);
      bool callableResult= (serverStatus & ServerStatus::PS_OUT_PARAMETERS)!=0;

      if (pr == nullptr)
      {
        selectResultSet= SelectResultSet::create(results, this, connection.get(), eofDeprecated);
      }
      else {
        pr->reReadColumnInfo();
        if (results->getResultSetConcurrency() == ResultSet::CONCUR_READ_ONLY) {
          selectResultSet= SelectResultSet::create(results, this, pr, callableResult, eofDeprecated);
        }
        else {
          // remove fetch size to permit updating results without creating new connection
          results->removeFetchSize();
          selectResultSet= UpdatableResultSet::create(results, this, pr, callableResult, eofDeprecated);
        }
      }
      results->addResultSet(selectResultSet, hasMoreResults() || results->getFetchSize() > 0);

    }
    catch (SQLException & e) {
      throw e;
    }
    catch (std::runtime_error& e){
      handleIoException(e).Throw();
    }
  }


  void QueryProtocol::prologProxy(
      ServerPrepareResult* serverPrepareResult,
      int64_t maxRows,
      bool hasProxy,
      MariaDbConnection* connection, MariaDbStatement* statement)

  {
    prolog(maxRows, hasProxy, connection, statement);
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
  void QueryProtocol::prolog(int64_t maxRows, bool hasProxy, MariaDbConnection* connection, MariaDbStatement* statement)
  {
    if (explicitClosed){
      throw SQLNonTransientConnectionException("execute() is called on closed connection", "08000");
    }

    if (!hasProxy && shouldReconnectWithoutProxy()) {
      try {
        connectWithoutProxy();
      } catch (SQLException& qe){
        exceptionFactory.reset(ExceptionFactory::of(serverThreadId, options));
        exceptionFactory->create(qe).Throw();
      }
    }

    try {
      setMaxRows(maxRows);
    }catch (SQLException& qe){
      exceptionFactory->create(qe).Throw();
    }

    connection->reenableWarnings();
  }

  ServerPrepareResult* QueryProtocol::addPrepareInCache(const SQLString& key, ServerPrepareResult* serverPrepareResult)
  {
    return serverPrepareStatementCache->put(key, serverPrepareResult);
  }

  void QueryProtocol::cmdPrologue()
  {
    Shared::Results activeStream = getActiveStreamingResult();
    if (activeStream) {
      activeStream->loadFully(false, this);
      activeStreamingResult.reset();
    }

    if (activeFutureTask){

      try {
        //activeFutureTask->get();
      }
      catch (/*ExecutionException*/std::runtime_error& ) {
      }
      /*catch (InterruptedException& interruptedException){
        forceReleaseWaitingPrepareStatement();
        //Thread.currentThread().interrupt();
        throw SQLException(
            "Interrupted reading remaining batch response ",
            INTERRUPTED_EXCEPTION.getSqlState(),
            -1,
            interruptedException);
      }*/
      /*finally*/forceReleaseWaitingPrepareStatement();

      activeFutureTask= nullptr;
    }

    if (!this->connected){
      throw SQLException("Connection* is closed", "08000", 1220);
    }
    interrupted= false;
  }

  // TODO set all client affected variables when implementing CONJ-319
  void QueryProtocol::resetStateAfterFailover(
      int64_t maxRows,int32_t transactionIsolationLevel, const SQLString& database,bool autocommit)
  {
    setMaxRows(maxRows);

    if (transactionIsolationLevel != 0){
      setTransactionIsolation(transactionIsolationLevel);
    }

    if (!database.empty() && !(getDatabase().compare(database) == 0)){
      setCatalog(database);
    }

    if (getAutocommit()!=autocommit){
      executeQuery(SQLString("set autocommit=").append(autocommit ?"1":"0"));
    }
  }

  /**
   * Handle IoException (reconnect if Exception is due to having send too much data, making server
   * close the connection.
   *
   * <p>There is 3 kind of IOException :
   *
   * <ol>
   *   <li>MaxAllowedPacketException : without need of reconnect : thrown when driver don't send
   *       packet that would have been too big then error is not a CONNECTION_EXCEPTION
   *   <li>packets size is greater than max_allowed_packet (can be checked with
   *       writer.isAllowedCmdLength()). Need to reconnect
   *   <li>unknown IO error throw a CONNECTION_EXCEPTION
   * </ol>
   *
   * @param initialException initial Io error
   * @return the resulting error to return to client.
   */
  MariaDBExceptionThrower QueryProtocol::handleIoException(std::runtime_error& initialException, bool throwRightAway)
  {
    bool mustReconnect= options->autoReconnect;
    bool maxSizeError;
    MaxAllowedPacketException* maxAllowedPacketEx= dynamic_cast<MaxAllowedPacketException*>(&initialException);
    MariaDBExceptionThrower result;

    if (maxAllowedPacketEx != nullptr){
      maxSizeError= true;
      if (maxAllowedPacketEx->isMustReconnect()){
        mustReconnect= true;
      }else {
        SQLNonTransientConnectionException ex(
          initialException.what() + getTraces(),
          UNDEFINED_SQLSTATE.getSqlState(), 0,
          &initialException);
        if (throwRightAway) {
          throw ex;
        }
        else {
          result.take(ex);
          return result;
        }
      }
    }else {
      maxSizeError= false;// writer.exceedMaxLength();
      if (maxSizeError){
        mustReconnect= true;
      }
    }

    if (mustReconnect && !explicitClosed){
      try {
        connect();

        try {
          resetStateAfterFailover(
              getMaxRows(), getTransactionIsolationLevel(), getDatabase(), getAutocommit());

          if (maxSizeError){
            SQLTransientConnectionException ex(
                "Could not send query: query size is >= to max_allowed_packet ("
                +/*writer.getMaxAllowedPacket()*/std::to_string(MAX_PACKET_LENGTH)
                +")"
                +getTraces(),
                UNDEFINED_SQLSTATE.getSqlState(), 0,
                &initialException);
            if (throwRightAway) {
              throw ex;
            }
            else {
              result.take(ex);
              return result;
            }
          }

          SQLNonTransientConnectionException ex(
              initialException.what()+getTraces(),
              UNDEFINED_SQLSTATE.getSqlState(), 0,
              &initialException);
          if (throwRightAway) {
            throw ex;
          }
          else {
            result.take(ex);
            return result;
          }

        }catch (SQLException& /*queryException*/){
          SQLNonTransientConnectionException ex(
              "reconnection succeed, but resetting previous state failed",
              UNDEFINED_SQLSTATE.getSqlState()+getTraces(), 0,
              &initialException);
          if (throwRightAway) {
            throw ex;
          }
          else {
            result.take(ex);
            return result;
          }
        }

      }catch (SQLException& /*queryException*/){
        connected= false;
        SQLNonTransientConnectionException ex(
            SQLString(initialException.what()).append("\nError during reconnection").append(getTraces()),
            CONNECTION_EXCEPTION.getSqlState(), 0,
            &initialException);
        if (throwRightAway) {
          throw ex;
        }
        else {
          result.take(ex);
          return result;
        }
      }
    }
    connected= false;
    SQLNonTransientConnectionException ex(
        initialException.what()+getTraces(),
        CONNECTION_EXCEPTION.getSqlState(), 0,
        &initialException);

    if (throwRightAway) {
      throw ex;
    }
    else {
      result.take(ex);
      return result;
    }
  }


  void QueryProtocol::setActiveFutureTask(FutureTask* activeFutureTask)
  {
    this->activeFutureTask= activeFutureTask;
  }


  void QueryProtocol::interrupt()
  {
    interrupted= true;
  }


  bool QueryProtocol::isInterrupted()
  {
    return interrupted;
  }

  /**
   * Throw TimeoutException if timeout has been reached.
   *
   * @throws SQLTimeoutException to indicate timeout exception.
   */
  void QueryProtocol::stopIfInterrupted()
  {
    if (isInterrupted()){

      throw SQLTimeoutException("Timeout during batch execution", "");
    }
  }

  /*void assembleQueryText(SQLString& resultSql, ClientPrepareResult* clientPrepareResult, const std::vector<ParameterHolder>& parameters, int32_t queryTimeout)
  {
    if (queryTimeout > 0) {
      resultSql.append(("SET STATEMENT max_statement_time=" + queryTimeout + " FOR ").getBytes());
    }
    if (clientPrepareResult.isRewriteType()) {

      out.write(clientPrepareResult.getQueryParts().get(0));
      out.write(clientPrepareResult.getQueryParts().get(1));
      for (int i = 0; i < clientPrepareResult.getParamCount(); i++) {
        parameters[i].writeTo(out);
        out.write(clientPrepareResult.getQueryParts().get(i + 2));
      }
      out.write(clientPrepareResult.getQueryParts().get(clientPrepareResult.getParamCount() + 2));

    }
    else {

      out.write(clientPrepareResult.getQueryParts().get(0));
      for (int i = 0; i < clientPrepareResult.getParamCount(); i++) {
        parameters[i].writeTo(out);
        out.write(clientPrepareResult.getQueryParts().get(i + 1));
      }
    }
  }*/
}
}
}
