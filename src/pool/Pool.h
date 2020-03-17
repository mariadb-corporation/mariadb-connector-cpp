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


#ifndef _POOL_H_
#define _POOL_H_

#include <vector>
#include <atomic>
#include <deque>
#include <list>

#include "Consts.h"
#include "UrlParser.h"
#include "GlobalStateInfo.h"
#include "MariaDbConnection.h"

namespace sql
{
namespace mariadb
{
class ThreadPoolExecutor;
class Runnable
{
  int32_t dummy;
};
class ScheduledFuture;
class ExecutorService;

class ScheduledThreadPoolExecutor;
enum TimeUnit {
  SECONDS
};

class MariaDbConnection;

class Pool
{
  static const Shared::Logger logger; /*LoggerFactory.getLogger(Pool.class)*/
  static int32_t POOL_STATE_OK; /*0*/
  static int32_t POOL_STATE_CLOSING; /*1*/
  std::atomic<int32_t> poolState; /*new std::atomic<int32_t>()*/

  std::shared_ptr<UrlParser> urlParser;
  const Shared::Options options;
  std::atomic<int32_t> pendingRequestNumber ; /*new std::atomic<int32_t>()*/
  std::atomic<int32_t> totalConnection ; /*new std::atomic<int32_t>()*/
  const std::deque<std::unique_ptr<MariaDbPooledConnection>>idleConnections;
  const ThreadPoolExecutor* connectionAppender;
  const std::vector<Runnable> connectionAppenderQueue;
  const SQLString poolTag;
  const ScheduledThreadPoolExecutor* poolExecutor;
  const ScheduledFuture* scheduledFuture;
  GlobalStateInfo globalInfo;
  int32_t maxIdleTime;
  int64_t timeToConnectNanos;
  int64_t connectionTime ; /*0*/

public:
  Pool(std::shared_ptr<UrlParser>& _urlParser, int32_t poolIndex, std::shared_ptr<ScheduledThreadPoolExecutor>& poolExecutor) : urlParser(_urlParser) {}

private:
  void addConnectionRequest();
  void removeIdleTimeoutConnection();
  void addConnection();
  MariaDbPooledConnection& getIdleConnection();
  MariaDbPooledConnection& getIdleConnection(int64_t timeout, TimeUnit timeUnit);
  void silentCloseConnection(MariaDbPooledConnection& item);
  void silentAbortConnection(MariaDbPooledConnection& item);
  MariaDbPooledConnection& createPoolConnection(MariaDbConnection* connection);

public:
  MariaDbConnection* getConnection();
  MariaDbConnection* getConnection(SQLString& username, SQLString& password);

private:
  SQLString generatePoolTag(int32_t poolIndex);
public:
  std::shared_ptr<UrlParser>& getUrlParser() { return urlParser; }
  void close() {}
private:
  void closeAll(ExecutorService connectionRemover, std::list<MariaDbPooledConnection>collection);
  void initializePoolGlobalState(MariaDbConnection& connection);
public:
  SQLString getPoolTag();
  bool equals(sql::Object* obj);
  int64_t hashCode();
  GlobalStateInfo getGlobalInfo();
  int64_t getActiveConnections();
  int64_t getTotalConnections();
  int64_t getIdleConnections();
  int64_t getConnectionRequests();

private:
  void registerJmx();
  void unRegisterJmx();
public:
  std::vector<int64_t>testGetConnectionIdleThreadIds();
  void resetStaticGlobal();
};

}
}
#endif
