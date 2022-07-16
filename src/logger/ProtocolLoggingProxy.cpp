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



#include "ProtocolLoggingProxy.h"
#include "logger/LoggerFactory.h"

namespace sql
{
namespace mariadb
{
  Shared::Logger ProtocolLoggingProxy::logger= LoggerFactory::getLogger(typeid(ProtocolLoggingProxy));

  ServerPrepareResult* ProtocolLoggingProxy::prepare(const SQLString& sql, bool executeOnMaster)
  {
    return protocol->prepare(sql, executeOnMaster);
  }


  bool ProtocolLoggingProxy::getAutocommit()
	{
		/* Add here logging if needed */
	  return protocol->getAutocommit();
	}


  bool ProtocolLoggingProxy::noBackslashEscapes()
	{
		/* Add here logging if needed */
    return protocol->noBackslashEscapes();
	}


  void ProtocolLoggingProxy::connect()
	{
		/* Add here logging if needed */
	  protocol->connect();
	}


  const UrlParser& ProtocolLoggingProxy::getUrlParser() const
	{
		/* Add here logging if needed */
	  return protocol->getUrlParser();
	}


  bool ProtocolLoggingProxy::inTransaction()
	{
		/* Add here logging if needed */
    return protocol->inTransaction();
	}


  FailoverProxy* ProtocolLoggingProxy::getProxy()
	{
		/* Add here logging if needed */
    return protocol->getProxy();
	}


  void ProtocolLoggingProxy::setProxy(FailoverProxy* proxy)
	{
		/* Add here logging if needed */
	  protocol->setProxy(proxy);
	}


  const Shared::Options& ProtocolLoggingProxy::getOptions() const
	{
		/* Add here logging if needed */
    return protocol->getOptions();
	}


  bool ProtocolLoggingProxy::hasMoreResults()
	{
		/* Add here logging if needed */
    return protocol->hasMoreResults();
	}


  void ProtocolLoggingProxy::close()
	{
		/* Add here logging if needed */
	  protocol->close();
	}


  void ProtocolLoggingProxy::reset()
	{
		/* Add here logging if needed */
	  protocol->reset();
	}


  void ProtocolLoggingProxy::closeExplicit()
	{
		/* Add here logging if needed */
	  protocol->closeExplicit();
	}


  bool ProtocolLoggingProxy::isClosed()
	{
		/* Add here logging if needed */
    return protocol->isClosed();
	}


  void ProtocolLoggingProxy::resetDatabase()
	{
		/* Add here logging if needed */
	  protocol->resetDatabase();
	}


  SQLString ProtocolLoggingProxy::getCatalog()
	{
		/* Add here logging if needed */
    return protocol->getCatalog();
	}


  void ProtocolLoggingProxy::setCatalog(const SQLString& database)
	{
		/* Add here logging if needed */
	  protocol->setCatalog(database);
	}


  const SQLString& ProtocolLoggingProxy::getServerVersion() const
	{
		/* Add here logging if needed */
    return protocol->getServerVersion();
	}


  bool ProtocolLoggingProxy::isConnected()
	{
		/* Add here logging if needed */
    return protocol->isConnected();
	}


  bool ProtocolLoggingProxy::getReadonly() const
	{
		/* Add here logging if needed */
    return protocol->getReadonly();
	}


  void ProtocolLoggingProxy::setReadonly(bool readOnly)
	{
		/* Add here logging if needed */
	  protocol->setReadonly(readOnly);
	}


  bool ProtocolLoggingProxy::isMasterConnection()
	{
		/* Add here logging if needed */
    return protocol->isMasterConnection();
	}


  bool ProtocolLoggingProxy::mustBeMasterConnection()
	{
		/* Add here logging if needed */
    return protocol->mustBeMasterConnection();
	}


  const HostAddress& ProtocolLoggingProxy::getHostAddress() const
	{
		/* Add here logging if needed */
    return  protocol->getHostAddress();
	}


  void ProtocolLoggingProxy::setHostAddress(const HostAddress& hostAddress)
	{
		/* Add here logging if needed */
	  protocol->setHostAddress(hostAddress);
	}


  const SQLString& ProtocolLoggingProxy::getHost() const
	{
		/* Add here logging if needed */
    return protocol->getHost();
	}

  int32_t ProtocolLoggingProxy::getPort() const
  {
    /* Add here logging if needed */
    return protocol->getPort();
  }

  void ProtocolLoggingProxy::rollback()
	{
		/* Add here logging if needed */
	  protocol->rollback();
	}


  const SQLString& ProtocolLoggingProxy::getDatabase() const
	{
		/* Add here logging if needed */
    return protocol->getDatabase();
	}


  const SQLString& ProtocolLoggingProxy::getUsername() const
	{
		/* Add here logging if needed */
    return protocol->getUsername();
	}


  bool ProtocolLoggingProxy::ping()
	{
		/* Add here logging if needed */
    return protocol->ping();
	}


  bool ProtocolLoggingProxy::isValid(int32_t timeout)
	{
		/* Add here logging if needed */
    return protocol->isValid(timeout);
	}


  void ProtocolLoggingProxy::executeQuery(const SQLString& sql)
	{
		/* Add here logging if needed */
	  protocol->executeQuery(sql);
	}


  void ProtocolLoggingProxy::executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql)
  {
    /* Add here logging if needed */
    protocol->executeQuery(mustExecuteOnMaster, results, sql);
  }


  void ProtocolLoggingProxy::executeQuery(bool mustExecuteOnMaster, Shared::Results& results, const SQLString& sql, const Charset* charset)
  {
    /* Add here logging if needed */
    protocol->executeQuery(mustExecuteOnMaster, results, sql, charset);
  }


  void ProtocolLoggingProxy::executeQuery(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* clientPrepareResult,
    std::vector<Shared::ParameterHolder>& parameters)
  {
    /* Add here logging if needed */
    protocol->executeQuery(mustExecuteOnMaster, results, clientPrepareResult, parameters);
  }


  void ProtocolLoggingProxy::executeQuery(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* clientPrepareResult,
    std::vector<Shared::ParameterHolder>& parameters, int32_t timeout)
  {
    /* Add here logging if needed */
    protocol->executeQuery(mustExecuteOnMaster, results, clientPrepareResult, parameters, timeout);
  }


  bool ProtocolLoggingProxy::executeBatchClient(bool mustExecuteOnMaster, Shared::Results& results, ClientPrepareResult* prepareResult,
    std::vector<std::vector<Shared::ParameterHolder>>& parametersList, bool hasLongData)
	{
		/* Add here logging if needed */
    return protocol->executeBatchClient(mustExecuteOnMaster, results, prepareResult, parametersList, hasLongData);
	}


  void ProtocolLoggingProxy::executeBatchStmt(bool mustExecuteOnMaster, Shared::Results& results, const std::vector<SQLString>& queries)
  {
    /* Add here logging if needed */
    protocol->executeBatchStmt(mustExecuteOnMaster, results, queries);
  }


  void ProtocolLoggingProxy::executePreparedQuery(bool mustExecuteOnMaster, ServerPrepareResult* serverPrepareResult, Shared::Results& results,
    std::vector<Shared::ParameterHolder>& parameters)
  {
    /* Add here logging if needed */
    protocol->executePreparedQuery(mustExecuteOnMaster, serverPrepareResult, results, parameters);
  }


  bool ProtocolLoggingProxy::executeBatchServer(bool mustExecuteOnMaster, ServerPrepareResult* serverPrepareResult, Shared::Results& results,
    const SQLString& sql, std::vector<std::vector<Shared::ParameterHolder>>& parameterList, bool hasLongData)
  {
    /* Add here logging if needed */
    return protocol->executeBatchServer(mustExecuteOnMaster, serverPrepareResult, results, sql, parameterList, hasLongData);
  }


	void ProtocolLoggingProxy::moveToNextResult(Results* results, ServerPrepareResult* spr)
	{
		/* Add here logging if needed */
	  protocol->getResult(results, spr);
	}


  void ProtocolLoggingProxy::getResult(Results* results, ServerPrepareResult* spr, bool readAllResults)
	{
		/* Add here logging if needed */
	  protocol->getResult(results, spr, readAllResults);
	}


  void ProtocolLoggingProxy::cancelCurrentQuery()
	{
		/* Add here logging if needed */
	  protocol->cancelCurrentQuery();
	}


  void ProtocolLoggingProxy::interrupt()
	{
		/* Add here logging if needed */
	  protocol->interrupt();
	}


  void ProtocolLoggingProxy::skip()
	{
		/* Add here logging if needed */
	  protocol->skip();
	}


  bool ProtocolLoggingProxy::checkIfMaster()
	{
		/* Add here logging if needed */
	  return protocol->checkIfMaster();
	}


  bool ProtocolLoggingProxy::hasWarnings()
	{
		/* Add here logging if needed */
	  return protocol->hasWarnings();
	}


  int64_t ProtocolLoggingProxy::getMaxRows()
	{
		/* Add here logging if needed */
	  return protocol->getMaxRows();
	}


  void ProtocolLoggingProxy::setMaxRows(int64_t max)
	{
		/* Add here logging if needed */
	  protocol->setMaxRows(max);
	}


  uint32_t ProtocolLoggingProxy::getMajorServerVersion()
	{
		/* Add here logging if needed */
	  return protocol->getMajorServerVersion();
	}


  uint32_t ProtocolLoggingProxy::getMinorServerVersion()
	{
		/* Add here logging if needed */
	  return protocol->getMinorServerVersion();
	}

  uint32_t ProtocolLoggingProxy::getPatchServerVersion()
	{
	  return protocol->getPatchServerVersion();
	}


  bool ProtocolLoggingProxy::versionGreaterOrEqual(uint32_t major, uint32_t minor, uint32_t patch) const
	{
		/* Add here logging if needed */
	  return protocol->versionGreaterOrEqual(major, minor, patch);
	}

  void ProtocolLoggingProxy::setLocalInfileInputStream(std::istream& inputStream)
  {
    protocol->setLocalInfileInputStream(inputStream);
  }

  int32_t ProtocolLoggingProxy::getTimeout()
	{
		/* Add here logging if needed */
	  return protocol->getTimeout();
	}


  void ProtocolLoggingProxy::setTimeout(int32_t timeout)
	{
		/* Add here logging if needed */
	  protocol->setTimeout(timeout);
	}


  bool ProtocolLoggingProxy::getPinGlobalTxToPhysicalConnection() const
	{
		/* Add here logging if needed */
    return protocol->getPinGlobalTxToPhysicalConnection();
	}

  int64_t ProtocolLoggingProxy::getServerThreadId()
  {
    return protocol->getServerThreadId();
  }


  //Socket* ProtocolLoggingProxy::getSocket()
	//{
	//	/* Add here logging if needed */
	//  protocol->getSocket();
	//}


  void ProtocolLoggingProxy::setTransactionIsolation(int32_t level)
	{
		/* Add here logging if needed */
	  protocol->setTransactionIsolation(level);
	}


  int32_t ProtocolLoggingProxy::getTransactionIsolationLevel()
	{
		/* Add here logging if needed */
    return protocol->getTransactionIsolationLevel();
	}


  bool ProtocolLoggingProxy::isExplicitClosed()
	{
		/* Add here logging if needed */
    return protocol->isExplicitClosed();
	}


  void ProtocolLoggingProxy::connectWithoutProxy()
	{
		/* Add here logging if needed */
    protocol->connectWithoutProxy();
	}


  bool ProtocolLoggingProxy::shouldReconnectWithoutProxy()
	{
		/* Add here logging if needed */
    return protocol->shouldReconnectWithoutProxy();
	}

  void ProtocolLoggingProxy::setHostFailedWithoutProxy()
  {
    /* Add here logging if needed */
    protocol->setHostFailedWithoutProxy();
  }
  void ProtocolLoggingProxy::releasePrepareStatement(ServerPrepareResult* serverPrepareResult)
	{
		/* Add here logging if needed */
	  protocol->releasePrepareStatement(serverPrepareResult);
	}


  bool ProtocolLoggingProxy::forceReleasePrepareStatement(capi::MYSQL_STMT* statementId)
	{
		/* Add here logging if needed */
    return protocol->forceReleasePrepareStatement(statementId);
	}


  void ProtocolLoggingProxy::forceReleaseWaitingPrepareStatement()
	{
		/* Add here logging if needed */
	  protocol->forceReleaseWaitingPrepareStatement();
	}


  ServerPrepareStatementCache* ProtocolLoggingProxy::prepareStatementCache()
	{
		/* Add here logging if needed */
    return protocol->prepareStatementCache();
	}


  TimeZone* ProtocolLoggingProxy::getTimeZone()
	{
		/* Add here logging if needed */
    return protocol->getTimeZone();
	}


  void ProtocolLoggingProxy::prolog(int64_t maxRows, bool hasProxy, MariaDbConnection* connection, MariaDbStatement* statement)
  {
    /* Add here logging if needed */
    protocol->prolog(maxRows, hasProxy, connection, statement);
  }


  void ProtocolLoggingProxy::prologProxy(ServerPrepareResult* serverPrepareResult, int64_t maxRows, bool hasProxy, MariaDbConnection* connection,
    MariaDbStatement* statement)
  {
    /* Add here logging if needed */
    protocol->prologProxy(serverPrepareResult, maxRows, hasProxy, connection, statement);
  }


  Shared::Results ProtocolLoggingProxy::getActiveStreamingResult()
	{
		/* Add here logging if needed */
	  return protocol->getActiveStreamingResult();
	}


  void ProtocolLoggingProxy::setActiveStreamingResult(Shared::Results& mariaSelectResultSet)
	{
		/* Add here logging if needed */
	  protocol->setActiveStreamingResult(mariaSelectResultSet);
	}


  Shared::mutex& ProtocolLoggingProxy::getLock()
	{
		/* Add here logging if needed */
	  return protocol->getLock();
	}


  void ProtocolLoggingProxy::setServerStatus(uint32_t serverStatus)
	{
		/* Add here logging if needed */
	  protocol->setServerStatus(serverStatus);
	}

  uint32_t ProtocolLoggingProxy::getServerStatus()
	{
		/* Add here logging if needed */
	  return protocol->getServerStatus();
	}

  void ProtocolLoggingProxy::removeHasMoreResults()
	{
		/* Add here logging if needed */
	  protocol->removeHasMoreResults();
	}


  void ProtocolLoggingProxy::setHasWarnings(bool hasWarnings)
	{
		/* Add here logging if needed */
	  protocol->setHasWarnings(hasWarnings);
	}


  ServerPrepareResult* ProtocolLoggingProxy::addPrepareInCache(const SQLString& key, ServerPrepareResult* serverPrepareResult)
	{
		/* Add here logging if needed */
    return protocol->addPrepareInCache(key, serverPrepareResult);
	}


  void ProtocolLoggingProxy::readEofPacket()
	{
		/* Add here logging if needed */
	  protocol->readEofPacket();
	}


  void ProtocolLoggingProxy::skipEofPacket()
	{
		/* Add here logging if needed */
	  protocol->skipEofPacket();
	}


  void ProtocolLoggingProxy::changeSocketTcpNoDelay(bool setTcpNoDelay)
	{
		/* Add here logging if needed */
	  protocol->changeSocketTcpNoDelay(setTcpNoDelay);
	}


  void ProtocolLoggingProxy::changeSocketSoTimeout(int32_t setSoTimeout)
	{
		/* Add here logging if needed */
	  protocol->changeSocketSoTimeout(setSoTimeout);
	}


  void ProtocolLoggingProxy::removeActiveStreamingResult()
	{
		/* Add here logging if needed */
	  protocol->removeActiveStreamingResult();
	}


  void ProtocolLoggingProxy::resetStateAfterFailover(int64_t maxRows, int32_t transactionIsolationLevel, const SQLString& database, bool autocommit)
  {
    /* Add here logging if needed */
    protocol->resetStateAfterFailover(maxRows, transactionIsolationLevel, database, autocommit);
  }


  bool ProtocolLoggingProxy::isServerMariaDb()
	{
		/* Add here logging if needed */
	  return protocol->isServerMariaDb();
	}


  void ProtocolLoggingProxy::setActiveFutureTask(FutureTask* activeFutureTask)
	{
		/* Add here logging if needed */
	  protocol->setActiveFutureTask(activeFutureTask);
	}


	MariaDBExceptionThrower ProtocolLoggingProxy::handleIoException(std::runtime_error& initialException, bool throwRightAway)
	{
		/* Add here logging if needed */
	  return protocol->handleIoException(initialException);
	}


  //PacketInputistream* ProtocolLoggingProxy::getReader()
	//{
	//	/* Add here logging if needed */
	//  protocol->getReader();
	//}


  //PacketOutputStream* ProtocolLoggingProxy::getWriter()
	//{
	//	/* Add here logging if needed */
	//  protocol->getWriter();
	//}


  bool ProtocolLoggingProxy::isEofDeprecated()
	{
		/* Add here logging if needed */
    return protocol->isEofDeprecated();
	}


  int32_t ProtocolLoggingProxy::getAutoIncrementIncrement()
	{
		/* Add here logging if needed */
    return protocol->getAutoIncrementIncrement();
	}


  bool ProtocolLoggingProxy::sessionStateAware()
	{
		/* Add here logging if needed */
    return protocol->sessionStateAware();
	}


  SQLString ProtocolLoggingProxy::getTraces()
	{
		/* Add here logging if needed */
    return protocol->getTraces();
	}


  bool ProtocolLoggingProxy::isInterrupted()
	{
		/* Add here logging if needed */
    return protocol->isInterrupted();
	}


  void ProtocolLoggingProxy::stopIfInterrupted()
	{
		/* Add here logging if needed */
	  protocol->stopIfInterrupted();
	}


	void ProtocolLoggingProxy::reconnect()
	{
		protocol->reconnect();
	}
}
}
