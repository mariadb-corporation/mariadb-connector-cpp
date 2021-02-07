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


#ifndef _MARIADBPOOLEDCONNECTION_H_
#define _MARIADBPOOLEDCONNECTION_H_

#include <atomic>
#include "Consts.h"

#include "MariaDbConnection.h"

namespace sql
{
class Executor;
namespace mariadb
{
class ConnectionEventListener;
class StatementEventListener;

class MariaDbPooledConnection //  : public PooledConnection {
{
  MariaDbConnection* connection;
  std::vector<ConnectionEventListener*>connectionEventListeners;
  std::vector<StatementEventListener*>statementEventListeners;
  std::atomic<std::int64_t> lastUsed;

public:
  MariaDbPooledConnection(MariaDbConnection* connection);
  MariaDbConnection* getConnection();
  void close();
  void abort(sql::Executor* executor);
  void addConnectionEventListener(ConnectionEventListener& listener);
  void removeConnectionEventListener(ConnectionEventListener& listener);
  void addStatementEventListener(StatementEventListener& listener);
  void removeStatementEventListener(StatementEventListener& listener);
  void fireStatementClosed(Statement* st);
  void fireStatementErrorOccured(Statement* st, MariaDBExceptionThrower& ex);
  void fireConnectionClosed();
  void fireConnectionErrorOccured(SQLException ex);
  bool noStmtEventListeners();
  int64_t getLastUsed();
  void lastUsedToNow();
  };
}
}
#endif
