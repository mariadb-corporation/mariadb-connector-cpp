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


#include "AbstractQueryProtocol.h"

namespace sql
{
namespace mariadb
{

  const Shared::Logger AbstractQueryProtocol::logger= LoggerFactory::getLogger(typeid(AbstractQueryProtocol));
  const SQLString AbstractQueryProtocol::CHECK_GALERA_STATE_QUERY("show status like 'wsrep_local_state'");
  /**
   * Get a protocol instance.
   *
   * @param urlParser connection URL information's
   * @param lock the lock for thread synchronisation
   */
  AbstractQueryProtocol::AbstractQueryProtocol(const UrlParser& urlParser, const GlobalStateInfo& globalInfo, Shared::mutex& lock)
  {
    super(urlParser,globalInfo,lock);
    logQuery= new LogQueryTool(options);
    galeraAllowedStates =
      urlParser.getOptions()->galeraAllowedState/*.empty() == true*/
      ?Collections.emptyList()
      :Arrays.asList(urlParser.getOptions()->galeraAllowedState.split(","));
  }

  void AbstractQueryProtocol::reset()
  {
    cmdPrologue();
    try {

      writer.startPacket(0);
      writer.write(COM_RESET_CONNECTION);
      writer.flush();
      getResult(new Results());


      if (options.cachePrepStmts &&options.useServerPrepStmts){
        serverPrepareStatementCache.clear();
      }

    }catch (SQLException& sqlException){
      throw logQuery.exceptionWithQuery(
          "COM_RESET_CONNECTION failed.",sqlException,explicitClosed);
    }catch (SQLException& sqlException){
      throw handleIoException(e);
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
  void AbstractQueryProtocol::executeQuery(const SQLString& sql)
  {
    executeQuery(isMasterConnection(), new Results(), sql);
  }

  void AbstractQueryProtocol::executeQuery(bool mustExecuteOnMaster, Shared::Results& results,const SQLString& sql)
  {

    cmdPrologue();
    try {

      writer.startPacket(0);
      writer.write(COM_QUERY);
      writer.write(sql);
      writer.flush();
      getResult(results);

    }catch (SQLException& sqlException){
      if ("70100".equals(sqlException.getSQLState())&&1927 == sqlException.getErrorCode()){
        throw handleIoException(sqlException);
      }
      throw logQuery.exceptionWithQuery(sql,sqlException,explicitClosed);
    }catch (SQLException& sqlException){
      throw handleIoException(e);
    }
  }

  void AbstractQueryProtocol::executeQuery( bool mustExecuteOnMaster, Shared::Results& results,const SQLString& sql, Charset* charset)
  {
    cmdPrologue();
    try {

      writer.startPacket(0);
      writer.write(COM_QUERY);
      writer.write(sql.getBytes(charset));
      writer.flush();
      getResult(results);

    }catch (SQLException& sqlException){
      throw logQuery.exceptionWithQuery(sql,sqlException,explicitClosed);
    }catch (SQLException& sqlException){
      throw handleIoException(e);
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
  void AbstractQueryProtocol::executeQuery(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<ParameterHolder>& parameters)

  {
    cmdPrologue();
    try {

      if (clientPrepareResult.getParamCount()==0
          &&!clientPrepareResult.isQueryMultiValuesRewritable()){
        if (clientPrepareResult.getQueryParts().size()==1){
          ComQuery.sendDirect(writer,clientPrepareResult.getQueryParts().get(0));
        }else {
          ComQuery.sendMultiDirect(writer,clientPrepareResult.getQueryParts());
        }
      }else {
        writer.startPacket(0);
        ComQuery.sendSubCmd(writer,clientPrepareResult,parameters,-1);
        writer.flush();
      }
      getResult(results);

    }catch (SQLException& queryException){
      throw logQuery.exceptionWithQuery(parameters,queryException,clientPrepareResult);
    }catch (SQLException& queryException){
      throw handleIoException(e);
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
  void AbstractQueryProtocol::executeQuery(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<ParameterHolder>& parameters,
      int32_t queryTimeout)

  {
    cmdPrologue();
    try {

      if (clientPrepareResult.getParamCount()==0
          &&!clientPrepareResult.isQueryMultiValuesRewritable()){
        if (clientPrepareResult.getQueryParts().size()==1){
          ComQuery.sendDirect(writer,clientPrepareResult.getQueryParts().get(0),queryTimeout);
        }else {
          ComQuery.sendMultiDirect(writer,clientPrepareResult.getQueryParts(),queryTimeout);
        }
      }else {
        writer.startPacket(0);
        ComQuery.sendSubCmd(writer,clientPrepareResult,parameters,queryTimeout);
        writer.flush();
      }
      getResult(results);

    }catch (SQLException& queryException){
      throw logQuery.exceptionWithQuery(parameters,queryException,clientPrepareResult);
    }catch (std::runtime_error& e){
      throw handleIoException(e);
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
  bool AbstractQueryProtocol::executeBatchClient(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* prepareResult,
      const std::vector<std::vector<ParameterHolder>>& parametersList,
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

    if (options.rewriteBatchedStatements){
      if (prepareResult.isQueryMultiValuesRewritable()
          &&results->getAutoGeneratedKeys()==Statement::NO_GENERATED_KEYS){
        // values rewritten in one query :
        // INSERT INTO X(a,b) VALUES (1,2), (3,4), ...
        executeBatchRewrite(results,prepareResult,parametersList,true);
        return true;

      }else if (prepareResult.isQueryMultipleRewritable()){

        if (options.useBulkStmts
            &&!hasLongData
            &&prepareResult.isQueryMultipleRewritable()
            &&results->getAutoGeneratedKeys()==Statement::NO_GENERATED_KEYS
            &&versionGreaterOrEqual(10,2,7)
            &&executeBulkBatch(results,prepareResult.getSql(),NULL,parametersList)){
          return true;
        }



        executeBatchRewrite(results,prepareResult,parametersList,false);
        return true;
      }
    }

    if (options.useBulkStmts
        &&!hasLongData
        &&results->getAutoGeneratedKeys()==Statement::NO_GENERATED_KEYS
        &&versionGreaterOrEqual(10,2,7)
        &&executeBulkBatch(results,prepareResult.getSql(),NULL,parametersList)){
      return true;
    }

    if (options.useBatchMultiSend){

      executeBatchMulti(results,prepareResult,parametersList);
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
  bool AbstractQueryProtocol::executeBulkBatch(
      Shared::Results& results, const SQLString& sql,
      ServerPrepareResult* serverPrepareResult,
      const std::vector<std::vector<ParameterHolder>>& parametersList)

  {
    // **************************************************************************************
    // Ensure BULK can be use :
    // - server version >= 10.2.7
    // - no stream
    // - parameter type doesn't change
    // - avoid INSERT FROM SELECT
    // **************************************************************************************

    ParameterHolder[] initParameters= parametersList.get(0);
    int32_t parameterCount= initParameters.length;
    int16_t[] types= new int16_t[parameterCount];
    for (int32_t i= 0;i <parameterCount;i++){
      types[i] =initParameters[i].getColumnType().getType();
    }


    for (ParameterHolder[] parameters :parametersList){
      for (int32_t i= 0;i <parameterCount;i++){
        if (parameters[i].getColumnType().getType()!=types[i]){
          return false;
        }
      }
    }


    if ((sql.toLowerCase().find_first_of("select") != std::string::npos)){
      return false;
    }

    cmdPrologue();

    ServerPrepareResult tmpServerPrepareResult= serverPrepareResult;
    try {
      SQLException exception= NULL;




      if (serverPrepareResult/*.empty() == true*/){
        tmpServerPrepareResult= prepare(sql,true);
      }




      int32_t statementId =
        tmpServerPrepareResult/*.empty() == true*/ ?tmpServerPrepareResult.getStatementId():-1;

      sql::bytes lastCmdData= NULL;
      int32_t index= 0;
      ParameterHolder[] firstParameters= parametersList.get(0);

      do {
        writer.startPacket(0);
        writer.write(COM_STMT_BULK_EXECUTE);
        writer.writeInt(statementId);
        writer.writeShort(static_cast<int16_t>(128);)

          for (ParameterHolder param :firstParameters){
            writer.writeShort(param.getColumnType().getType());
          }

        if (lastCmdData/*.empty() == true*/){
          writer.checkMaxAllowedLength(lastCmdData.length);
          writer.write(lastCmdData);
          writer.mark();
          index++;
          lastCmdData= NULL;
        }

        for (;index <parametersList.size();index++){
          ParameterHolder[] parameters= parametersList.get(index);
          for (int32_t i= 0;i <parameterCount;i++){
            ParameterHolder holder= parameters[i];
            if (holder.isNullData()){
              writer.write(1);
            }else {
              writer.write(0);
              holder.writeBinary(writer);
            }
          }


          if (writer.exceedMaxLength()&&writer.isMarked()){
            writer.flushBufferStopAtMark();
          }


          if (writer.bufferIsDataAfterMark()){
            break;
          }

          writer.checkMaxAllowedLength(0);
          writer.mark();
        }

        if (writer.bufferIsDataAfterMark()){

          lastCmdData= writer.resetMark();
        }else {
          writer.flush();
          writer.resetMark();
        }

        try {
          getResult(results);
        }catch (SQLException& sqle){
          if ("HY000".equals(sqle.getSQLState())&&sqle.getErrorCode()==1295){



            results->getCmdInformation()->reset();
            return false;
          }
          if (exception/*.empty() == true*/){
            exception= logQuery.exceptionWithQuery(sql,sqle,explicitClosed);
            if (!options.continueBatchOnError){
              throw exception;
            }
          }
        }

      }while (index <parametersList.size()-1);

      if (lastCmdData/*.empty() == true*/){
        writer.startPacket(0);
        writer.write(COM_STMT_BULK_EXECUTE);
        writer.writeInt(statementId);
        writer.writeShort((int8_t)0x80);

        for (ParameterHolder param :firstParameters){
          writer.writeShort(param.getColumnType().getType());
        }
        writer.write(lastCmdData);
        writer.flush();
        try {
          getResult(results);
        }catch (SQLException& sqle){
          if ("HY000".equals(sqle.getSQLState())&&sqle.getErrorCode()==1295){

            return false;
          }
          if (exception/*.empty() == true*/){
            exception= logQuery.exceptionWithQuery(sql,sqle,explicitClosed);
            if (!options.continueBatchOnError){
              throw exception;
            }
          }
        }
      }

      if (exception/*.empty() == true*/){
        throw exception;
      }
      results->setRewritten(true);
      return true;

    }catch (SQLException& sqle){
      throw handleIoException(e);
    }/* TODO: something with the finally was once here */ {
      if (serverPrepareResult/*.empty() == true*/ &&tmpServerPrepareResult/*.empty() == true*/){
        releasePrepareStatement(tmpServerPrepareResult);
      }
      writer.resetMark();
    }
  }

  void AbstractQueryProtocol::initializeBatchReader()
  {
    if (options.useBatchMultiSend){
      readScheduler= SchedulerServiceProviderHolder.getBulkScheduler();
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
  void AbstractQueryProtocol::executeBatchMulti(
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      const std::vector<std::vector<ParameterHolder>>& parametersList)

  {

    cmdPrologue();
    initializeBatchReader();
    new AbstractMultiSend(
        this,writer,results,clientPrepareResult,parametersList,readScheduler)/*{

      @Override
        void AbstractQueryProtocol::sendCmd(
            PacketOutputStream writer,
            Results results,
            std::vector<ParameterHolder[]>parametersList,
            std::vector<SQLString>queries,
            int32_t paramCount,
            BulkStatus status,
            PrepareResult prepareResult)
        {

          ParameterHolder[] parameters= parametersList.get(status.sendCmdCounter);
          writer.startPacket(0);
          ComQuery.sendSubCmd(writer,clientPrepareResult,parameters,-1);
          writer.flush();
        }

      @Override
        void AbstractQueryProtocol::sendCmd(
            SQLException qex,
            Results results,
            std::vector<ParameterHolder[]>parametersList,
            std::vector<SQLString>queries,
            int32_t currentCounter,
            int32_t sendCmdCounter,
            int32_t paramCount,
            PrepareResult prepareResult){

          int32_t counter= results->getCurrentStatNumber()-1;
          ParameterHolder[] parameters= parametersList.get(counter);
          std::vector<int8_t[]>queryParts= clientPrepareResult.getQueryParts();
          SQLString sql(new SQLString(queryParts.get(0)));

          for (int32_t i= 0;i <paramCount;i++){
            sql.append(parameters[i].toString()).append(new SQLString(queryParts.get(i +1)));
          }

          return logQuery.exceptionWithQuery(sql,qex,explicitClosed);
        }

      @Override
        void AbstractQueryProtocol::sendCmd(){
          return clientPrepareResult.getQueryParts().size()-1;
        }

      @Override
        void AbstractQueryProtocol::sendCmd(){
          return parametersList.size();
        }
    }*/.executeBatch();
  }

  /**
   * Execute batch from Statement.executeBatch().
   *
   * @param mustExecuteOnMaster was intended to be launched on master connection
   * @param results results
   * @param queries queries
   * @throws SQLException if any exception occur
   */
  void AbstractQueryProtocol::executeBatchStmt(bool mustExecuteOnMaster, Shared::Results& results, const std::vector<SQLString>& queries)
  {
    cmdPrologue();
    if (this->options.rewriteBatchedStatements){


      bool canAggregateSemiColumn= true;
      for (SQLString query :queries){
        if (!ClientPrepareResult.canAggregateSemiColon(query,noBackslashEscapes())){
          canAggregateSemiColumn= false;
          break;
        }
      }

      if (isInterrupted()){

        throw SQLTimeoutException("Timeout during batch execution");
      }

      if (canAggregateSemiColumn){
        executeBatchAggregateSemiColon(results,queries);
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
  void AbstractQueryProtocol::executeBatch(Shared::Results& results, const std::vector<SQLString>& queries)
  {

    if (!options.useBatchMultiSend){

      SQLString sql;
      SQLException* exception= NULL;

      for (int32_t i= 0;i <queries.size()&&!isInterrupted();i++){

        try {

          sql= queries.get(i);
          writer.startPacket(0);
          writer.write(COM_QUERY);
          writer.write(sql);
          writer.flush();
          getResult(results);

        }catch (SQLException& sqlException){
          if (exception/*.empty() == true*/){
            exception= logQuery.exceptionWithQuery(sql,sqlException,explicitClosed);
            if (!options.continueBatchOnError){
              throw exception;
            }
          }
        }catch (SQLException& sqlException){
          if (exception/*.empty() == true*/){
            exception= handleIoException(e);
            if (!options.continueBatchOnError){
              throw exception;
            }
          }
        }
      }
      stopIfInterrupted();

      if (exception){
        throw *exception;
      }
      return;
    }
    initializeBatchReader();
    new AbstractMultiSend(this,writer,results,queries,readScheduler)
    /*{

        void AbstractQueryProtocol::sendCmd(
            PacketOutputStream pos,
            Results results,
            std::vector<ParameterHolder[]>parametersList,
            std::vector<SQLString>queries,
            int32_t paramCount,
            BulkStatus status,
            PrepareResult prepareResult)
        {

          SQLString sql= queries.get(status.sendCmdCounter);
          pos.startPacket(0);
          pos.write(COM_QUERY);
          pos.write(sql);
          pos.flush();
        }

        void AbstractQueryProtocol::sendCmd(
            SQLException qex,
            Results results,
            std::vector<ParameterHolder[]>parametersList,
            std::vector<SQLString>queries,
            int32_t currentCounter,
            int32_t sendCmdCounter,
            int32_t paramCount,
            PrepareResult prepareResult){

          SQLString sql= queries.get(currentCounter +sendCmdCounter);
          return logQuery.exceptionWithQuery(sql,qex,explicitClosed);
        }

        void AbstractQueryProtocol::sendCmd(){
          return -1;
        }

        void AbstractQueryProtocol::sendCmd(){
          return queries.size();
        }
    }*/.executeBatch();
  }

  ServerPrepareResult AbstractQueryProtocol::prepare(const SQLString& sql,bool executeOnMaster)
  {

    cmdPrologue();
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {
      if (options.cachePrepStmts &&options.useServerPrepStmts){

        ServerPrepareResult pr= serverPrepareStatementCache.get(database +"-"+sql);

        if (pr && pr.incrementShareCounter()){
          return pr;
        }
      }
      writer.startPacket(0);
      writer.write(COM_STMT_PREPARE);
      writer.write(sql);
      writer.flush();

      ComStmtPrepare comStmtPrepare= new ComStmtPrepare(this,sql);
      return comStmtPrepare.read(reader,eofDeprecated);
    }catch (IOException& e){
      throw handleIoException(e);
    }
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
  void AbstractQueryProtocol::executeBatchAggregateSemiColon(Shared::Results results, const std::vector<SQLString>& queries)
  {

    SQLString firstSql= NULL;
    int32_t currentIndex= 0;
    int32_t totalQueries= queries.size();
    SQLException exception= NULL;

    do {

      try {

        firstSql= queries.get(currentIndex++);

        if (totalQueries == 1){
          writer.startPacket(0);
          writer.write(COM_QUERY);
          writer.write(firstSql);
          writer.flush();
        }else {
          currentIndex =
            ComQuery.sendBatchAggregateSemiColon(writer,firstSql,queries,currentIndex);
        }
        getResult(results);

      }catch (SQLException& sqlException){
        if (exception/*.empty() == true*/){
          exception= logQuery.exceptionWithQuery(firstSql,sqlException,explicitClosed);
          if (!options.continueBatchOnError){
            throw exception;
          }
        }
      }catch (SQLException& sqlException){
        throw handleIoException(e);
      }
      stopIfInterrupted();

    }while (currentIndex <totalQueries);

    if (exception/*.empty() == true*/){
      throw exception;
    }
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
  void AbstractQueryProtocol::executeBatchRewrite(
      Shared::Results& results,
      ClientPrepareResult* prepareResult,
      std::vector<std::vector<ParameterHolder>>& parameterList,
      bool rewriteValues)

  {

    cmdPrologue();

    std::vector<ParameterHolder>::const_iterator parameters;
    int32_t currentIndex= 0;
    int32_t totalParameterList= parameterList.size();

    try {

      do {

        currentIndex =
          ComQuery.sendRewriteCmd(
              writer,
              prepareResult.getQueryParts(),
              currentIndex,
              prepareResult.getParamCount(),
              parameterList,
              rewriteValues);
        getResult(results);

        if (Thread.currentThread().isInterrupted()){
          throw SQLException(
              "Interrupted during batch",INTERRUPTED_EXCEPTION.getSqlState(),-1);
        }

      }while (currentIndex <totalParameterList);

    }catch (SQLException& sqlEx){
      throw logQuery.exceptionWithQuery(sqlEx,prepareResult);
    }catch (SQLException& sqlEx){
      throw handleIoException(e);
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
  bool AbstractQueryProtocol::executeBatchServer(
      bool mustExecuteOnMaster,
      ServerPrepareResult* serverPrepareResult,
      Shared::Results& results, const SQLString& sql,
      const std::vector<std::vector<ParameterHolder>>& parametersList,
      bool hasLongData)

  {

    cmdPrologue();

    if (options.useBulkStmts
        &&!hasLongData
        &&results->getAutoGeneratedKeys()==Statement::NO_GENERATED_KEYS
        &&versionGreaterOrEqual(10,2,7)
        &&executeBulkBatch(results,sql,serverPrepareResult,parametersList)){
      return true;
    }

    if (!options.useBatchMultiSend){
      return false;
    }
    initializeBatchReader();
    new AbstractMultiSend(
        this,writer,results,serverPrepareResult,parametersList,true,sql,readScheduler)/*{
      @Override
        void AbstractQueryProtocol::sendCmd(
            PacketOutputStream writer,
            Results results,
            std::vector<ParameterHolder[]>parametersList,
            std::vector<SQLString>queries,
            int32_t paramCount,
            BulkStatus status,
            PrepareResult prepareResult)
        ,IOException {

          ParameterHolder[] parameters= parametersList.get(status.sendCmdCounter);


          if (parameters.length <paramCount){
            throw SQLException(
                "Parameter at position "+(paramCount -1)+" is not set","07004");
          }


          for (int32_t i= 0;i <paramCount;i++){
            if (parameters[i].isLongData()){
              writer.startPacket(0);
              writer.write(COM_STMT_SEND_LONG_DATA);
              writer.writeInt(statementId);
              writer.writeShort(static_cast<int16_t>(i);)
                parameters[i].writeBinary(writer);
              writer.flush();
            }
          }

          writer.startPacket(0);
          ComStmtExecute.writeCmd(
              statementId,
              parameters,
              paramCount,
              parameterTypeHeader,
              writer,
              CURSOR_TYPE_NO_CURSOR);
          writer.flush();
        }

      @Override
        void AbstractQueryProtocol::sendCmd(
            SQLException qex,
            Results results,
            std::vector<ParameterHolder[]>parametersList,
            std::vector<SQLString>queries,
            int32_t currentCounter,
            int32_t sendCmdCounter,
            int32_t paramCount,
            PrepareResult prepareResult){
          return logQuery.exceptionWithQuery(qex,prepareResult);
        }

      @Override
        void AbstractQueryProtocol::sendCmd(){
          return !getPrepareResult()
            ?parametersList.get(0).length
            :((ServerPrepareResult)getPrepareResult()).getParameters().length;
        }

      @Override
        void AbstractQueryProtocol::sendCmd(){
          return parametersList.size();
        }
    }*/.executeBatch();
    return true;
  }

  void AbstractQueryProtocol::executePreparedQuery(
      bool mustExecuteOnMaster,
      ServerPrepareResult serverPrepareResult,
      Results results,
      std::vector<ParameterHolder>& parameters)

  {

    cmdPrologue();

    try {

      int32_t parameterCount= serverPrepareResult.getParameters().length;


      for (int32_t i= 0;i <parameterCount;i++){
        if (parameters[i].isLongData()){
          writer.startPacket(0);
          writer.write(COM_STMT_SEND_LONG_DATA);
          writer.writeInt(serverPrepareResult.getStatementId());
          writer.writeShort(static_cast<int16_t>(i);)
            parameters[i].writeBinary(writer);
          writer.flush();
        }
      }


      ComStmtExecute.send(
          writer,
          serverPrepareResult.getStatementId(),
          parameters,
          parameterCount,
          serverPrepareResult.getParameterTypeHeader(),
          CURSOR_TYPE_NO_CURSOR);
      getResult(results);

    }catch (SQLException& qex){
      throw logQuery.exceptionWithQuery(parameters,qex,serverPrepareResult);
    }catch (SQLException& qex){
      throw handleIoException(e);
    }
  }

  /** Rollback transaction. */
  void AbstractQueryProtocol::rollback()
  {

    cmdPrologue();

    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {

      if (inTransaction()){
        executeQuery("ROLLBACK");
      }

    }catch (Exception& e){

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
  bool AbstractQueryProtocol::forceReleasePrepareStatement(int32_t statementId)
  {

    if (lock.tryLock()){

      try {

        checkClose();

        try {
          writer.startPacket(0);
          writer.write(COM_STMT_CLOSE);
          writer.writeInt(statementId);
          writer.flush();
          return true;
        }catch (IOException& e){
          connected= false;
          throw SQLException(
              "Could not deallocate query: "+e.getMessage(),
              CONNECTION_EXCEPTION.getSqlState(),
              e);
        }

      }/* TODO: something with the finally was once here */ {
        lock.unlock();
      }

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
  void AbstractQueryProtocol::forceReleaseWaitingPrepareStatement()
  {
    if (statementIdToRelease != -1 &&forceReleasePrepareStatement(statementIdToRelease)){
      statementIdToRelease= -1;
    }
  }


  bool AbstractQueryProtocol::ping()
  {

    cmdPrologue();
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {

      writer.startPacket(0);
      writer.write(COM_PING);
      writer.flush();

      Buffer buffer= reader.getPacket(true);
      return buffer.getByteAt(0)==OK;

    }catch (IOException& e){
      connected= false;
      throw SQLException(
          "Could not ping: "+e.getMessage(),CONNECTION_EXCEPTION.getSqlState(),e);
    }
  }


  bool AbstractQueryProtocol::isValid(int32_t timeout)
  {

    int32_t initialTimeout= -1;
    try {
      initialTimeout= this->socketTimeout;
      if (initialTimeout == 0){
        this->changeSocketSoTimeout(timeout);
      }
      if (isMasterConnection()&&!galeraAllowedStates.empty()){


        Results results= new Results();
        executeQuery(true,results,CHECK_GALERA_STATE_QUERY);
        results->commandEnd();
        ResultSet* rs= results->getResultSet();

        return rs/*.empty() != true*/ &&rs->next()&&(galeraAllowedStates.find_first_of(rs->getString(2) != std::string::npos));
      }

      return ping();

    }catch (SocketException& socketException){
      logger.trace("Connection* is not valid",socketException);
      connected= false;
      return false;
    }/* TODO: something with the finally was once here */ {


      try {
        if (initialTimeout != -1){
          this->changeSocketSoTimeout(initialTimeout);
        }
      }catch (SocketException& socketException){
        logger.warn("Could not set socket timeout back to "+initialTimeout,socketException);
        connected= false;

      }
    }
  }

  SQLString AbstractQueryProtocol::getCatalog()
  {

    if ((serverCapabilities &MariaDbServerCapabilities.CLIENT_SESSION_TRACK)!=0){

      if (database/*.empty() != true*/ &&database.empty()){
        return NULL;
      }
      return database;
    }

    cmdPrologue();
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {
      Results results= new Results();
      executeQuery(isMasterConnection(),results,"select database()");
      results->commandEnd();
      ResultSet* rs= results->getResultSet();
      if (rs->next()){
        this->database= rs->getString(1);
        return database;
      }
      return NULL;
    }
  }

  void AbstractQueryProtocol::setCatalog(const SQLString& database)
  {

    cmdPrologue();

    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {

      SendChangeDbPacket.send(writer,database);
      const Buffer buffer= reader.getPacket(true);

      if (buffer.getByteAt(0)==ERROR){
        const ErrorPacket ep= new ErrorPacket(buffer);
        throw SQLException(
            "Could not select database '"+database +"' : "+ep.getMessage(),
            ep.getSqlState(),
            ep.getErrorNumber());
      }

      this->database= database;

    }catch (IOException& e){
      throw handleIoException(e);
    }/* TODO: something with the finally was once here */ {
      lock.unlock();
    }
  }

  void AbstractQueryProtocol::resetDatabase()
  {
    if (!(database.compare(urlParser.getDatabase() == 0))){
      setCatalog(urlParser.getDatabase());
    }
  }

  void AbstractQueryProtocol::cancelCurrentQuery()
  {
    try (MasterProtocol copiedProtocol =
        new MasterProtocol(urlParser,new GlobalStateInfo(), new std::mutex())){
      copiedProtocol.setHostAddress(getHostAddress());
      copiedProtocol.connect();

      copiedProtocol.executeQuery("KILL QUERY "+serverThreadId);
    }
    interrupted= true;
  }

  bool AbstractQueryProtocol::getAutocommit()
  {
    return ((serverStatus & ServerStatus::AUTOCOMMIT)!=0);
  }

  bool AbstractQueryProtocol::inTransaction()
  {
    return ((serverStatus &  ServerStatus::IN_TRANSACTION)!=0);
  }


  void AbstractQueryProtocol::closeExplicit()
  {
    this->explicitClosed= true;
    close();
  }


  void AbstractQueryProtocol::releasePrepareStatement(ServerPrepareResult* serverPrepareResult)
  {


    serverPrepareResult.decrementShareCounter();


    if (serverPrepareResult.canBeDeallocate()){
      forceReleasePrepareStatement(serverPrepareResult.getStatementId());
    }
  }


  int64_t AbstractQueryProtocol::getMaxRows()
  {
    return maxRows;
  }


  void AbstractQueryProtocol::setMaxRows(int64_t max)
  {
    if (maxRows != max){
      if (max == 0){
        executeQuery("set @@SQL_SELECT_LIMIT=DEFAULT");
      }else {
        executeQuery("set @@SQL_SELECT_LIMIT="+max);
      }
      maxRows= max;
    }
  }


  void AbstractQueryProtocol::setLocalInfileInputStream(std::istream& inputStream)
  {
    this->localInfileInputStream= inputStream;
  }


  int32_t AbstractQueryProtocol::getTimeout()
  {
    return this->socketTimeout;
  }


  void AbstractQueryProtocol::setTimeout(int32_t timeout)
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {
      this->changeSocketSoTimeout(timeout);
    }
  }

  /**
   * Set transaction isolation.
   *
   * @param level transaction level.
   * @throws SQLException if transaction level is unknown
   */
  void AbstractQueryProtocol::setTransactionIsolation(int32_t level)
  {
    cmdPrologue();
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {
      SQLString query= "SET SESSION TRANSACTION ISOLATION LEVEL";
      switch (level){
        case Connection::TRANSACTION_READ_UNCOMMITTED:
          query +=" READ UNCOMMITTED";
          break;
        case Connection::TRANSACTION_READ_COMMITTED:
          query +=" READ COMMITTED";
          break;
        case Connection::TRANSACTION_REPEATABLE_READ:
          query +=" REPEATABLE READ";
          break;
        case Connection::TRANSACTION_SERIALIZABLE:
          query +=" SERIALIZABLE";
          break;
        default:
          throw SQLException("Unsupported transaction isolation level");
      }
      executeQuery(query);
      transactionIsolationLevel= level;
    }
  }


  int32_t AbstractQueryProtocol::getTransactionIsolationLevel()
  {
    return transactionIsolationLevel;
  }


  void AbstractQueryProtocol::checkClose()
  {
    if (!this->connected){
      throw SQLException("Connection* is close","08000",1220);
    }
  }


  void AbstractQueryProtocol::getResult(Shared::Results& results)
  {

    readPacket(results);


    while (hasMoreResults()){
      readPacket(results);
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
  void AbstractQueryProtocol::readPacket(Shared::Results& results)
  {
    Buffer buffer;
    try {
      buffer= reader.getPacket(true);
    }catch (IOException& e){
      throw handleIoException(e);
    }

    switch (buffer.getByteAt(0)){




      case OK:
        readOkPacket(buffer,results);
        break;




      case ERROR:
        throw readErrorPacket(buffer,results);




      case LOCAL_INFILE:
        readLocalInfilePacket(buffer,results);
        break;




      default:
        readResultSet(buffer,results);
        break;
    }
  }

  /**
   * Read OK_Packet.
   *
   * @param buffer current buffer
   * @param results result object
   * @see <a href="https://mariadb.com/kb/en/mariadb/ok_packet/">OK_Packet</a>
   */
  void AbstractQueryProtocol::readOkPacket(Buffer* buffer, Shared::Results& results)
  {
    buffer.skipByte();
    const int64_t updateCount= buffer.getLengthEncodedNumeric();
    const int64_t insertId= buffer.getLengthEncodedNumeric();

    serverStatus= buffer.readShort();
    hasWarnings= (buffer.readShort()>0);

    if ((serverStatus & ServerStatus::SERVER_SESSION_STATE_CHANGED)!=0){
      handleStateChange(buffer,results);
    }

    results->addStats(updateCount,insertId,hasMoreResults());
  }

  void AbstractQueryProtocol::handleStateChange(Buffer* buf, Shared::Results& results)
  {
    buf.skipLengthEncodedBytes();
    while (buf.remaining()>0){
      Buffer stateInfo= buf.getLengthEncodedBuffer();
      if (stateInfo.remaining()>0){
        switch (stateInfo.readByte()){
          case StateChange.SESSION_TRACK_SYSTEM_VARIABLES:
            Buffer sessionVariableBuf= stateInfo.getLengthEncodedBuffer();
            SQLString variable= sessionVariableBuf.readStringLengthEncoded(StandardCharsets.UTF_8);
            SQLString value= sessionVariableBuf.readStringLengthEncoded(StandardCharsets.UTF_8);
            logger.debug("System variable change :  {} = {}",variable,value);


            switch (variable){
              case "auto_increment_increment":
                autoIncrementIncrement= int32_t.parseInt(value);
                results->setAutoIncrement(autoIncrementIncrement);
                break;

              default:

            }
            break;

          case StateChange.SESSION_TRACK_SCHEMA:
            Buffer sessionSchemaBuf= stateInfo.getLengthEncodedBuffer();
            database= sessionSchemaBuf.readStringLengthEncoded(StandardCharsets.UTF_8);
            logger.debug("Database change : now is '{}'",database);
            break;

          default:
            stateInfo.skipLengthEncodedBytes();
        }
      }
    }
  }

  /**
   * Get current auto increment increment. *** no lock needed ****
   *
   * @return auto increment increment.
   * @throws SQLException if cannot retrieve auto increment value
   */
  int32_t AbstractQueryProtocol::getAutoIncrementIncrement()
  {
    if (autoIncrementIncrement == 0){
      std::lock_guard<std::mutex> localScopeLock(*lock);
      try {
        Results results= new Results();
        executeQuery(true,results,"select @@auto_increment_increment");
        results->commandEnd();
        ResultSet* rs= results->getResultSet();
        rs->next();
        autoIncrementIncrement= rs->getInt(1);
      }catch (SQLException& e){
        if (e.getSQLState().startsWith("08")){
          throw e;
        }
        autoIncrementIncrement= 1;
      }/* TODO: something with the finally was once here */ {
        lock.unlock();
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
  SQLException AbstractQueryProtocol::readErrorPacket(Buffer* buffer, Shared::Results& results)
  {
    removeHasMoreResults();
    this->hasWarnings= false;
    buffer.skipByte();
    int32_t errorNumber= buffer.readShort();
    SQLString message;
    SQLString sqlState;
    if (buffer.readByte()=='#'){
      sqlState= new SQLString(buffer.readRawBytes(5));
      message= buffer.readStringNullEnd(StandardCharsets.UTF_8);
    }else {

      buffer.position -=1;
      message =
        new SQLString(
            buffer.buf,buffer.position,buffer.limit -buffer.position,StandardCharsets.UTF_8);
      sqlState= "HY000";
    }
    results->addStatsError(false);



    serverStatus |= ServerStatus::IN_TRANSACTION;

    removeActiveStreamingResult();
    return new SQLException(message,sqlState,errorNumber);
  }

  /**
   * Read Local_infile Packet.
   *
   * @param buffer current buffer
   * @param results result object
   * @throws SQLException if sub-result connection fail
   * @see <a href="https://mariadb.com/kb/en/mariadb/local_infile-packet/">local_infile packet</a>
   */
  void AbstractQueryProtocol::readLocalInfilePacket(Buffer* buffer, Shared::Results& results)
  {

    int32_t seq= 2;
    buffer.getLengthEncodedNumeric();
    SQLString fileName= buffer.readStringNullEnd(StandardCharsets.UTF_8);
    try {



      InputStream is;
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


        ServiceLoader<LocalInfileInterceptor>loader =
          ServiceLoader.load(typeid(LocalInfileInterceptor));
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

        if (results->getSql()/*.empty() == true*/){
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
              SqlStates.INVALID_AUTHORIZATION.getSqlState(),
              -1);
        }

        try {
          URL url= new URL(fileName);
          is= url.openStream();
        }catch (IOException& ioe){
          try {
            is= new FileInputStream(fileName);
          }catch (IOException& ioe){
            writer.writeEmptyPacket();
            reader.getPacket(true);
            throw SQLException("Could not send file : "+f.getMessage(),"22000",-1,f);
          }
        }
      }else {
        is= localInfileInputStream;
        localInfileInputStream= NULL;
      }

      try {

        sql::bytes buf= new int8_t[8192];
        int32_t len;
        while ((len= is.read(buf))>0){
          writer.startPacket(seq++);
          writer.write(buf,0,len);
          writer.flush();
        }
        writer.writeEmptyPacket();

      }catch (IOException& ioe){
        throw handleIoException(ioe);
      }/* TODO: something with the finally was once here */ {
        is.close();
      }

      getResult(results);

    }catch (IOException& ioe){
      throw handleIoException(e);
    }
  }

  /**
   * Read ResultSet Packet.
   *
   * @param buffer current buffer
   * @param results result object
   * @throws SQLException if sub-result connection fail
   * @see <a href="https://mariadb.com/kb/en/mariadb/resultset/">resultSet packets</a>
   */
  void AbstractQueryProtocol::readResultSet(Buffer* buffer, Shared::Results& results)
  {
    int64_t fieldCount= buffer->getLengthEncodedNumeric();

    try {


      ColumnInformation[] ci= new ColumnInformation[static_cast<int32_t>(fieldCount)];
      for (int32_t i= 0;i <fieldCount;i++){
        ci[i] =new ColumnInformation(reader.getPacket(false));
      }

      bool callableResult= false;
      if (!eofDeprecated){






        Buffer bufferEof= reader.getPacket(true);
        if (bufferEof.readByte()!=EOF){

          throw IOException(
              "Packets out of order when reading field packets, expected was EOF stream."
              +((options.enablePacketDebug)
                ?getTraces()
                :"Packet contents (hex) = "
                +Utils::hexdump(
                  options.maxQuerySizeToLog,0,bufferEof.limit,bufferEof.buf)));
        }
        bufferEof.skipBytes(2);
        callableResult= (bufferEof.readShort()& ServerStatus::PS_OUT_PARAMETERS)!=0;
      }


      SelectResultSet* selectResultSet;
      if (results->getResultSetConcurrency()==ResultSet::CONCUR_READ_ONLY){
        selectResultSet =
          new SelectResultSet*(ci,results,this,reader,callableResult,eofDeprecated);
      }else {

        results->removeFetchSize();
        selectResultSet =
          new UpdatableResultSet(ci,results,this,reader,callableResult,eofDeprecated);
      }

      results->addResultSet(selectResultSet,hasMoreResults()||results->getFetchSize()>0);

    }catch (IOException& e){
      throw handleIoException(e);
    }
  }

  void AbstractQueryProtocol::prologProxy(
      ServerPrepareResult* serverPrepareResult,
      int64_t maxRows,
      bool hasProxy,
      MariaDbConnection* connection, MariaDbConnection* statement)

  {
    prolog(maxRows,hasProxy,connection,statement);
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
  void AbstractQueryProtocol::prolog(int64_t maxRows, bool hasProxy, MariaDbConnection* connection, MariaDbConnection* statement)
  {
    if (explicitClosed){
      throw SQLNonTransientConnectionException("execute() is called on closed connection");
    }

    if (!hasProxy &&shouldReconnectWithoutProxy()){
      try {
        connectWithoutProxy();
      }catch (SQLException& qe){
        ExceptionMapper::throwException(qe,connection,statement);
      }
    }

    try {
      setMaxRows(maxRows);
    }catch (SQLException& qe){
      ExceptionMapper::throwException(qe,connection,statement);
    }

    connection->reenableWarnings();
  }

  ServerPrepareResult* AbstractQueryProtocol::addPrepareInCache(const SQLString& key,ServerPrepareResult* serverPrepareResult)
  {
    return serverPrepareStatementCache.put(key,serverPrepareResult);
  }

  void AbstractQueryProtocol::cmdPrologue()
  {


    if (activeStreamingResult/*.empty() != true*/){
      activeStreamingResult.loadFully(false,this);
      activeStreamingResult= NULL;
    }

    if (activeFutureTask/*.empty() != true*/){

      try {
        activeFutureTask.get();
      }catch (ExecutionException& executionException){

      }catch (ExecutionException& executionException){
        Thread.currentThread().interrupt();
        throw SQLException(
            "Interrupted reading remaining batch response ",
            INTERRUPTED_EXCEPTION.getSqlState(),
            -1,
            interruptedException);
      }/* TODO: something with the finally was once here */ {



        forceReleaseWaitingPrepareStatement();
      }
      activeFutureTask= NULL;
    }

    if (!this->connected){
      throw SQLException("Connection* is closed","08000",1220);
    }
    interrupted= false;
  }

  // TODO set all client affected variables when implementing CONJ-319
  void AbstractQueryProtocol::resetStateAfterFailover(
      int64_t maxRows,int32_t transactionIsolationLevel, const SQLString& database,bool autocommit)

  {
    setMaxRows(maxRows);

    if (transactionIsolationLevel != 0){
      setTransactionIsolation(transactionIsolationLevel);
    }

    if (database/*.empty() != true*/ &&!"".equals(database)&&!(getDatabase().compare(database) == 0)){
      setCatalog(database);
    }

    if (getAutocommit()!=autocommit){
      executeQuery("set autocommit="+(autocommit ?"1":"0"));
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
  SQLException AbstractQueryProtocol::handleIoException(std::runtime_error& initialException)
  {
    bool mustReconnect= options.autoReconnect;
    bool maxSizeError;

    if (INSTANCEOF(initialException, MaxAllowedPacketException)){
      maxSizeError= true;
      if (((MaxAllowedPacketException)initialException).isMustReconnect()){
        mustReconnect= true;
      }else {
        return new SQLNonTransientConnectionException(
            initialException.getMessage()+getTraces(),
            UNDEFINED_SQLSTATE.getSqlState(),
            initialException);
      }
    }else {
      maxSizeError= writer.exceedMaxLength();
      if (maxSizeError){
        mustReconnect= true;
      }
    }

    if (mustReconnect &&!explicitClosed){
      try {
        connect();

        try {
          resetStateAfterFailover(
              getMaxRows(),getTransactionIsolationLevel(),getDatabase(),getAutocommit());

          if (maxSizeError){
            return new SQLTransientConnectionException(
                "Could not send query: query size is >= to max_allowed_packet ("
                +writer.getMaxAllowedPacket()
                +")"
                +getTraces(),
                UNDEFINED_SQLSTATE.getSqlState(),
                initialException);
          }

          return new SQLNonTransientConnectionException(
              initialException.getMessage()+getTraces(),
              UNDEFINED_SQLSTATE.getSqlState(),
              initialException);

        }catch (SQLException& queryException){
          return new SQLNonTransientConnectionException(
              "reconnection succeed, but resetting previous state failed",
              UNDEFINED_SQLSTATE.getSqlState()+getTraces(),
              initialException);
        }

      }catch (SQLException& queryException){
        connected= false;
        return new SQLNonTransientConnectionException(
            initialException.getMessage()+"\nError during reconnection"+getTraces(),
            CONNECTION_EXCEPTION.getSqlState(),
            initialException);
      }
    }
    connected= false;
    return new SQLNonTransientConnectionException(
        initialException.getMessage()+getTraces(),
        CONNECTION_EXCEPTION.getSqlState(),
        initialException);
  }


  void AbstractQueryProtocol::setActiveFutureTask(FutureTask* activeFutureTask)
  {
    this->activeFutureTask= activeFutureTask;
  }


  void AbstractQueryProtocol::interrupt()
  {
    interrupted= true;
  }


  bool AbstractQueryProtocol::isInterrupted()
  {
    return interrupted;
  }

  /**
   * Throw TimeoutException if timeout has been reached.
   *
   * @throws SQLTimeoutException to indicate timeout exception.
   */
  void AbstractQueryProtocol::stopIfInterrupted()
  {
    if (isInterrupted()){

      throw SQLTimeoutException("Timeout during batch execution");
    }
  }

}
}
