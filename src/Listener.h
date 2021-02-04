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


#ifndef _LISTENER_H_
#define _LISTENER_H_

#include <vector>

#include "Connection.hpp"

#include "UrlParser.h"
#include "HostAddress.h"

namespace sql
{
namespace mariadb
{
class FailoverProxy;
class SearchFilter;

class Listener
{
  Listener(const Listener &);
  void operator=(Listener &);
public:
  Listener() {}
  virtual ~Listener(){}

  virtual FailoverProxy* getProxy()=0;
  virtual void setProxy(FailoverProxy* proxy)=0;
  virtual void initializeConnection()=0;
  virtual void preExecute()=0;
  virtual void preClose()=0;
  virtual void preAbort()=0;
  virtual void reconnectFailedConnection(SearchFilter& filter)=0;
  virtual void switchReadOnlyConnection(bool readonly)=0;

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /* In this case unlikely implementation, but translating to something usable */
  virtual HandleErrorResult primaryFail(Method method,sql::sql::Object** args,bool killCmd,bool wasClosed) =0;
  virtual sql::Object* invoke(Method method,sql::sql::Object** args, Shared::Protocol& specificProtocol)=0;
  virtual sql::Object* invoke(Method method,sql::sql::Object** args)=0;
  virtual HandleErrorResult handleFailover( SQLException qe,Method method,sql::sql::Object** args, Shared::Protocol& protocol,bool wasClosed) =0;
#endif
  virtual void foundActiveMaster(Shared::Protocol& protocol)=0;
  virtual std::vector<HostAddress>& getBlacklistKeys()=0;
  virtual void addToBlacklist(HostAddress hostAddress)=0;
  virtual void removeFromBlacklist(const HostAddress& hostAddress)=0;
  virtual void syncConnection(Shared::Protocol& from, Shared::Protocol& to)=0;
  virtual std::shared_ptr<UrlParser>& getUrlParser()=0;
  virtual void throwFailoverMessage(const HostAddress& failHostAddress, bool wasMaster, SQLException& queryException, bool reconnected) =0;
  virtual bool isAutoReconnect()=0;
  virtual int32_t getRetriesAllDown()=0;
  virtual bool isExplicitClosed()=0;
  virtual void reconnect()=0;
  virtual bool isReadOnly()=0;
  virtual bool inTransaction()=0;
  virtual int32_t getMajorServerVersion()=0;
  virtual bool isMasterConnection()=0;
  virtual bool isClosed()=0;
  virtual bool sessionStateAware()=0;
  virtual bool noBackslashEscapes()=0;
  virtual bool isValid(int32_t timeout)=0;
  virtual void prolog(int64_t maxRows, Connection* connection, MariaDbStatement* statement) =0;
  virtual SQLString getCatalog()=0;
  virtual int32_t getTimeout()=0;
  virtual Shared::Protocol& getCurrentProtocol()=0;
  virtual bool hasHostFail()=0;
  virtual bool canRetryFailLoop()=0;
  virtual SearchFilter* getFilterForFailedHost()=0;
  virtual bool isMasterConnected()=0;
  virtual bool setMasterHostFail()=0;
  virtual bool isMasterHostFail()=0;
  virtual int64_t getLastQueryNanos()=0;
  virtual bool checkMasterStatus(SearchFilter& searchFilter)=0;
  virtual void rePrepareOnSlave(ServerPrepareResult* oldServerPrepareResult, bool mustExecuteOnMaster) =0;
  virtual void reset()=0;
};

}
}
#endif
