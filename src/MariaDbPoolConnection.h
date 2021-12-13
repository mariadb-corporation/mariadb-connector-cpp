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


#ifndef _MARIADBPOOLCONNECTION_H_
#define _MARIADBPOOLCONNECTION_H_

#include <atomic>
#include "Consts.h"

#include "MariaDbConnection.h"
#include "PooledConnection.hpp"
#include "pool/ConnectionEventListener.h"

namespace sql
{
class ConnectionEvent;
class ConnectionEventListener;
class StatementEventListener;
class ScheduledThreadPoolExecutor;

namespace mariadb
{

class MariaDbPoolConnection : public PooledConnection {

  std::vector<std::unique_ptr<ConnectionEventListener>> connectionEventListeners;
  std::vector<StatementEventListener*> statementEventListeners;

protected:
  MariaDbConnection* connection;

public:
  virtual ~MariaDbPoolConnection();
  MariaDbPoolConnection(MariaDbConnection* connection);
  sql::Connection* getConnection();
  void close();
  void returnToPool();
  void abort(sql::ScheduledThreadPoolExecutor* executor);
  void addConnectionEventListener(ConnectionEventListener* listener);
  void removeConnectionEventListener(ConnectionEventListener* listener);
  void addStatementEventListener(StatementEventListener* listener);
  void removeStatementEventListener(StatementEventListener* listener);
  void fireStatementClosed(Statement* st);
  void fireStatementErrorOccured(Statement* st, MariaDBExceptionThrower& ex);
  void fireConnectionClosed(ConnectionEvent* event);
  void fireConnectionErrorOccured(SQLException& ex);
  bool noStmtEventListeners();
  /* Since destroying of Connection objects is under application's control, we should construct new Connection objects around old Protocol object
     to store in the Pool */
  virtual MariaDbConnection* makeFreshConnectionObj();
  };
}
}
#endif
