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


#ifndef _PROTOCOLLOGGINGPROXY_H_
#define _PROTOCOLLOGGINGPROXY_H_


#include "Protocol.h"
#include "Consts.h"

namespace sql
{
namespace mariadb
{
//class NumberFormat;
class LogQueryTool;

class ProtocolLoggingProxy : public Protocol
{
  Shared::Protocol protocol;
  static Shared::Logger logger/*= LoggerFactory.getLogger(ProtocolLoggingProxy.class)*/;
  //NumberFormat* numberFormat; /* I don't think we will need it(any time soon) */
  bool profileSql;
  int64_t slowQueryThresholdNanos;
  int32_t maxQuerySizeToLog;
  LogQueryTool* logQuery;

  ProtocolLoggingProxy() {}

public:
  ProtocolLoggingProxy(Shared::Protocol &realProtocol, const Shared::Options& options) : protocol(realProtocol),
    profileSql(options->profileSql), slowQueryThresholdNanos(options->slowQueryThresholdNanos),
    maxQuerySizeToLog(options->maxQuerySizeToLog), logQuery(nullptr)
  {}

  ServerPrepareResult* prepare(const SQLString& sql, bool executeOnMaster);
  bool getAutocommit();
  bool noBackslashEscapes();
  void connect();
  const UrlParser& getUrlParser() const;
  bool inTransaction();
  FailoverProxy* getProxy();
  void setProxy(FailoverProxy* proxy);
  const Shared::Options& getOptions() const;
  bool hasMoreResults();
  void close();
  void reset();
  void closeExplicit();
  bool isClosed();
  void resetDatabase();
  SQLString getCatalog();
  void setCatalog(const SQLString& database);
  const SQLString& getServerVersion() const;
  bool isConnected();
  bool getReadonly() const;
  void setReadonly(bool readOnly);
  bool isMasterConnection();
  bool mustBeMasterConnection();
  const HostAddress& getHostAddress() const;
  void setHostAddress(const HostAddress& hostAddress);
  const SQLString& getHost() const;
  int32_t getPort() const;
  void rollback();
  const SQLString& getDatabase() const;
  const SQLString& getUsername() const;
  bool ping();
  bool isValid(int32_t timeout);
  void executeQuery(const SQLString& sql);
  void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql);
  void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql, const Charset* charset);
  void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* clientPrepareResult, std::vector<Shared::ParameterHolder>& parameters);
  void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* clientPrepareResult, std::vector<Shared::ParameterHolder>& parameters,
    int32_t timeout);
  bool executeBatchClient(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* prepareResult,
    std::vector<std::vector<Shared::ParameterHolder>>& parametersList, bool hasLongData);
  void executeBatchStmt(bool mustExecuteOnMaster,Shared::Results& results, const std::vector<SQLString>& queries);
  void executePreparedQuery(bool mustExecuteOnMaster, ServerPrepareResult* serverPrepareResult, Shared::Results& results, std::vector<Shared::ParameterHolder>& parameters);
  bool executeBatchServer(bool mustExecuteOnMaster, ServerPrepareResult* serverPrepareResult, Shared::Results& results, const SQLString& sql,
                          std::vector<std::vector<Shared::ParameterHolder>>& parameterList, bool hasLongData);
  void moveToNextResult(Results* results, ServerPrepareResult* spr=nullptr);
  void getResult(Results* results, ServerPrepareResult *pr=nullptr, bool readAllResults=false);
  void cancelCurrentQuery();
  void interrupt();
  void skip();
  bool checkIfMaster();
  bool hasWarnings();
  int64_t getMaxRows();
  void setMaxRows(int64_t max);
  uint32_t getMajorServerVersion();
  uint32_t getMinorServerVersion();
  uint32_t getPatchServerVersion();
  bool versionGreaterOrEqual(uint32_t major, uint32_t minor, uint32_t patch) const;
  void setLocalInfileInputStream(std::istream& inputStream);
  int32_t getTimeout();
  void setTimeout(int32_t timeout);
  bool getPinGlobalTxToPhysicalConnection() const;
  int64_t getServerThreadId();
  //Socket* getSocket();
  void setTransactionIsolation(int32_t level);
  int32_t getTransactionIsolationLevel();
  bool isExplicitClosed();
  void connectWithoutProxy();
  bool shouldReconnectWithoutProxy();
  void setHostFailedWithoutProxy();
  void releasePrepareStatement(ServerPrepareResult* serverPrepareResult);
  bool forceReleasePrepareStatement(capi::MYSQL_STMT* statementId);
  void forceReleaseWaitingPrepareStatement();
  ServerPrepareStatementCache* prepareStatementCache();
  TimeZone* getTimeZone();
  void prolog(int64_t maxRows, bool hasProxy, MariaDbConnection* connection, MariaDbStatement* statement);
  void prologProxy( ServerPrepareResult* serverPrepareResult, int64_t maxRows, bool hasProxy, MariaDbConnection* connection, MariaDbStatement* statement);
  Shared::Results getActiveStreamingResult();
  void setActiveStreamingResult(Shared::Results& mariaSelectResultSet);
  Shared::mutex& getLock();
  void setServerStatus(uint32_t serverStatus);
  uint32_t getServerStatus();
  void removeHasMoreResults();
  void setHasWarnings(bool hasWarnings);
  ServerPrepareResult* addPrepareInCache(const SQLString& key, ServerPrepareResult* serverPrepareResult);
  void readEofPacket();
  void skipEofPacket();
  void changeSocketTcpNoDelay(bool setTcpNoDelay);
  void changeSocketSoTimeout(int32_t setSoTimeout);
  void removeActiveStreamingResult();
  void resetStateAfterFailover(int64_t maxRows,int32_t transactionIsolationLevel, const SQLString& database, bool autocommit);
  bool isServerMariaDb();
  void setActiveFutureTask(FutureTask* activeFutureTask);
  MariaDBExceptionThrower handleIoException(std::runtime_error& initialException, bool throwRightAway= true);
  //PacketInputistream* getReader();
  //PacketOutputStream* getWriter();
  bool isEofDeprecated();
  int32_t getAutoIncrementIncrement();
  bool sessionStateAware();
  SQLString getTraces();
  bool isInterrupted();
  void stopIfInterrupted();
  void reconnect();
  };

}
}
#endif
