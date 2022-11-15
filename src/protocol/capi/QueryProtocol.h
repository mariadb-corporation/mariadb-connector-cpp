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


#ifndef _ABSTRACTQUERYPROTOCOL_H_
#define _ABSTRACTQUERYPROTOCOL_H_

#include <istream>
#include <vector>

#include "Consts.h"

#include "protocol/capi/ConnectProtocol.h"
#include "Exception.hpp"

namespace sql
{
namespace mariadb
{
  class ClientPrepareResult;
  class ServerPrepareResult;
  class FutureTask;
  class LogQueryTool;
namespace capi
{
  class QueryProtocol : public ConnectProtocol
  {
    typedef capi::ConnectProtocol super;

    static const Shared::Logger logger;
    static const SQLString CHECK_GALERA_STATE_QUERY; /*"show status like 'wsrep_local_state'"*/
    std::unique_ptr<LogQueryTool> logQuery;
    Tokens galeraAllowedStates;
    //ThreadPoolExecutor readScheduler; /*NULL*/
    int32_t transactionIsolationLevel; /*0*/
    std::unique_ptr<std::istream> localInfileInputStream;
    int64_t maxRows;
    /*volatile*/
    MYSQL_STMT* statementIdToRelease; /*-1*/
    FutureTask* activeFutureTask;
    bool interrupted= false;

  protected:
    QueryProtocol(std::shared_ptr<UrlParser>& urlParser, GlobalStateInfo* globalInfo, Shared::mutex& lock);
    virtual ~QueryProtocol() {}

  public:
    void reset();
    void executeQuery(const SQLString& sql);
    void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql);
    void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql, const Charset* charset);

    void executeQuery(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<Shared::ParameterHolder>& parameters);

    /*void assemblePreparedQueryForExec(
      SQLString& out,
      ClientPrepareResult* clientPrepareResult,
      std::vector<Shared::ParameterHolder>& parameters,
      int32_t queryTimeout);*/

    void executeQuery(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<Shared::ParameterHolder>& parameters,
      int32_t queryTimeout);

    bool executeBatchClient(
      bool mustExecuteOnMaster,
      Shared::Results& results,
      ClientPrepareResult* prepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList,
      bool hasLongData);

  private:

    bool executeBulkBatch(
      Shared::Results& results, const SQLString& sql,
      ServerPrepareResult* serverPrepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList);
    void initializeBatchReader();

    void executeBatchMulti(
      Shared::Results& results,
      ClientPrepareResult* clientPrepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList);

  public:
    void executeBatchStmt(bool mustExecuteOnMaster, Shared::Results& results, const std::vector<SQLString>& queries);

  private:
    void executeBatch(Shared::Results& results, const std::vector<SQLString>& queries);
    /* Does actual prepare job w/out locking, i.e. is good to use if lock has been already acquired */
    ServerPrepareResult* prepareInternal(const SQLString& sql, bool executeOnMaster);
  public:
    ServerPrepareResult* prepare(const SQLString& sql, bool executeOnMaster);

  private:
    void executeBatchAggregateSemiColon(Shared::Results& results, const std::vector<SQLString>& queries, std::size_t totalLenEstimation= 0);

    void executeBatchRewrite(
      Shared::Results& results,
      ClientPrepareResult* prepareResult,
      std::vector<std::vector<Shared::ParameterHolder>>& parameterList,
      bool rewriteValues);

  public:

    bool executeBatchServer(
      bool mustExecuteOnMaster,
      ServerPrepareResult* serverPrepareResult,
      Shared::Results& results, const SQLString& sql,
      std::vector<std::vector<Shared::ParameterHolder>>& parametersList,
      bool hasLongData);

    void executePreparedQuery(
      bool mustExecuteOnMaster,
      ServerPrepareResult* serverPrepareResult,
      Shared::Results& results,
      std::vector<Shared::ParameterHolder>& parameters);
    void rollback();
    bool forceReleasePrepareStatement(capi::MYSQL_STMT* statementId);
    void forceReleaseWaitingPrepareStatement();
    bool ping();
    bool isValid(int32_t timeout);
    SQLString getCatalog();
    void setCatalog(const SQLString& database);
    void resetDatabase();
    void cancelCurrentQuery();
    bool getAutocommit();
    bool inTransaction();
    void closeExplicit();

    void releasePrepareStatement(ServerPrepareResult* serverPrepareResult);
    int64_t getMaxRows();
    void setMaxRows(int64_t max);
    void setLocalInfileInputStream(std::istream& inputStream);
    int32_t getTimeout();
    void setTimeout(int32_t timeout);
    void setTransactionIsolation(int32_t level);
    int32_t getTransactionIsolationLevel();

  private:
    void checkClose();

  public:
    void moveToNextResult(Results* results, ServerPrepareResult* spr);
    void getResult(Results* results, ServerPrepareResult *pr=nullptr, bool readAllResults= false);

  private:
    void readPacket(Results* results, ServerPrepareResult *pr);
    void readOkPacket(Results* results, ServerPrepareResult *pr);
    void handleStateChange(Results* results);
    uint32_t errorOccurred(ServerPrepareResult *pr);
    uint32_t fieldCount(ServerPrepareResult *pr);

  public:
    int32_t getAutoIncrementIncrement();

  private:
    SQLException readErrorPacket(Results* results, ServerPrepareResult *pr= nullptr);
    void readLocalInfilePacket(Shared::Results& results);
    void readResultSet(Results* results, ServerPrepareResult *pr);

  public:

    void prologProxy(
      ServerPrepareResult* serverPrepareResult,
      int64_t maxRows,
      bool hasProxy,
      MariaDbConnection* connection,
      MariaDbStatement* statement);
    void prolog(int64_t maxRows, bool hasProxy, MariaDbConnection* connection, MariaDbStatement* statement);
    ServerPrepareResult* addPrepareInCache(const SQLString& key, ServerPrepareResult* serverPrepareResult);

  private:
    void cmdPrologue();

  public:
    void resetStateAfterFailover(int64_t maxRows, int32_t transactionIsolationLevel, const SQLString& database, bool autocommit);
    MariaDBExceptionThrower handleIoException(std::runtime_error& initialException, bool throwRightAway=true);
    void setActiveFutureTask(FutureTask* activeFutureTask);
    void interrupt();
    bool isInterrupted();
    void stopIfInterrupted();
  };

}  //capi
}  //mariadb
}  //sql
#endif
