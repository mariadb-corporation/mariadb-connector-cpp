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


#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <vector>
#include <mutex>

#include "Consts.h"

#include "UrlParser.h"
#include "HostAddress.h"
#include "options/Options.h"
#include "parameters/ParameterHolder.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"
}

class ServerPrepareResult;
class ClientPrepareResult;
class FailoverProxy;
class Results;
class Charset;
//class ParameterHolder;
class TimeZone;
class ServerPrepareStatementCache;
class MariaDbStatement;
class FutureTask;

class Protocol
{
public:
  virtual ~Protocol() {}
  virtual ServerPrepareResult* prepare(const SQLString& sql, bool executeOnMaster)=0;
  virtual bool getAutocommit()=0;
  virtual bool noBackslashEscapes()=0;
  virtual void connect()=0;
  virtual const UrlParser& getUrlParser() const=0;
  virtual bool inTransaction()=0;
  virtual FailoverProxy* getProxy()=0;
  virtual void setProxy(FailoverProxy* proxy)=0;
  virtual const Shared::Options& getOptions() const=0;
  virtual bool hasMoreResults()=0;
  virtual void close()=0;
  virtual void reset()=0;
  virtual void closeExplicit()=0;
  virtual bool isClosed()=0;
  virtual void resetDatabase()=0;
  virtual SQLString getCatalog()=0;
  virtual void setCatalog(const SQLString& database)=0;
  virtual const SQLString& getServerVersion() const=0;
  virtual bool isConnected()=0;
  virtual bool getReadonly() const=0;
  virtual void setReadonly(bool readOnly)=0;
  virtual bool isMasterConnection()=0;
  virtual bool mustBeMasterConnection()=0;
  virtual const HostAddress& getHostAddress() const=0;
  virtual void setHostAddress(const HostAddress& hostAddress)=0;
  virtual const SQLString& getHost() const=0;
  virtual int32_t getPort() const=0;
  virtual void rollback()=0;
  virtual const SQLString& getDatabase() const=0;
  virtual const SQLString& getUsername() const=0;
  virtual bool ping()=0;
  virtual bool isValid(int32_t timeout)=0;
  virtual void executeQuery(const SQLString& sql)=0;
  virtual void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql)= 0;
  virtual void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql, const Charset* charset)= 0;
  virtual void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* clientPrepareResult,
    std::vector<Shared::ParameterHolder>& parameters)= 0;
  virtual void executeQuery(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* clientPrepareResult,
    std::vector<Shared::ParameterHolder>& parameters,
    int32_t timeout)= 0;
  virtual bool executeBatchClient(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* prepareResult,
    std::vector<std::vector<Shared::ParameterHolder>>& parametersList, bool hasLongData)=0;
  virtual void executeBatchStmt(bool mustExecuteOnMaster, Shared::Results& results, const std::vector<SQLString>& queries)= 0;
  virtual void executePreparedQuery(bool mustExecuteOnMaster, ServerPrepareResult* serverPrepareResult, Shared::Results& results,
    std::vector<Shared::ParameterHolder>& parameters)= 0;
  virtual bool executeBatchServer(bool mustExecuteOnMaster, ServerPrepareResult* serverPrepareResult, Shared::Results& results, const SQLString& sql,
                                  std::vector<std::vector<Shared::ParameterHolder>>& parameterList, bool hasLongData)= 0;
  virtual void moveToNextResult(Results* results, ServerPrepareResult* spr= nullptr)=0;
  virtual void getResult(Results* results, ServerPrepareResult *pr=nullptr, bool readAllResults= false)=0;
  virtual void cancelCurrentQuery()=0;
  virtual void interrupt()=0;
  virtual void skip()=0;
  virtual bool checkIfMaster()=0;
  virtual bool hasWarnings()=0;
  virtual int64_t getMaxRows()=0;
  virtual void setMaxRows(int64_t max)=0;
  virtual uint32_t getMajorServerVersion()=0;
  virtual uint32_t getMinorServerVersion()=0;
  virtual uint32_t getPatchServerVersion()=0;
  virtual bool versionGreaterOrEqual(uint32_t major, uint32_t minor, uint32_t patch) const=0;
  virtual void setLocalInfileInputStream(std::istream& inputStream)=0;
  virtual int32_t getTimeout()=0;
  virtual void setTimeout(int32_t timeout)=0;
  virtual bool getPinGlobalTxToPhysicalConnection() const=0;
  virtual int64_t getServerThreadId()=0;
  //virtual Socket* getSocket()=0;
  virtual void setTransactionIsolation(int32_t level)=0;
  virtual int32_t getTransactionIsolationLevel()=0;
  virtual bool isExplicitClosed()=0;
  virtual void connectWithoutProxy()=0;
  virtual bool shouldReconnectWithoutProxy()=0;
  virtual void setHostFailedWithoutProxy()=0;
  virtual void releasePrepareStatement(ServerPrepareResult* serverPrepareResult)=0;
  virtual bool forceReleasePrepareStatement(capi::MYSQL_STMT* statementId)=0;
  virtual void forceReleaseWaitingPrepareStatement()=0;
  virtual ServerPrepareStatementCache* prepareStatementCache()=0;
  virtual TimeZone* getTimeZone()=0;
  virtual void prolog(int64_t maxRows, bool hasProxy, MariaDbConnection* connection, MariaDbStatement* statement)= 0;
  virtual void prologProxy(ServerPrepareResult* serverPrepareResult, int64_t maxRows, bool hasProxy, MariaDbConnection* connection,
    MariaDbStatement* statement)= 0;
  virtual Shared::Results getActiveStreamingResult()=0;
  virtual void setActiveStreamingResult(Shared::Results& mariaSelectResultSet)=0;
  virtual Shared::mutex& getLock()=0;
  virtual void setServerStatus(uint32_t serverStatus)=0;
  virtual uint32_t getServerStatus()=0;
  virtual void removeHasMoreResults()=0;
  virtual void setHasWarnings(bool hasWarnings)=0;
  virtual ServerPrepareResult* addPrepareInCache(const SQLString& key, ServerPrepareResult* serverPrepareResult)=0;
  virtual void readEofPacket()=0;
  virtual void skipEofPacket()=0;
  virtual void changeSocketTcpNoDelay(bool setTcpNoDelay)=0;
  virtual void changeSocketSoTimeout(int32_t setSoTimeout)=0;
  virtual void removeActiveStreamingResult()=0;
  virtual void resetStateAfterFailover( int64_t maxRows,int32_t transactionIsolationLevel, const SQLString& database,bool autocommit)= 0;
  virtual bool isServerMariaDb()=0;
  virtual void setActiveFutureTask(FutureTask* activeFutureTask)=0;
  virtual MariaDBExceptionThrower handleIoException(std::runtime_error& initialException, bool throwRightAway=true)=0;
  //virtual PacketInputistream* getReader()=0;
  //virtual PacketOutputStream* getWriter()=0;
  virtual bool isEofDeprecated()=0;
  virtual int32_t getAutoIncrementIncrement()=0;
  virtual bool sessionStateAware()=0;
  virtual SQLString getTraces()=0;
  virtual bool isInterrupted()=0;
  virtual void stopIfInterrupted()=0;
  virtual void reconnect()=0;
  /* I guess at some point we will need it */
  //virtual Protocol* clone();
  };

}
}
#endif
