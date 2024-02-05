/************************************************************************************
   Copyright (C) 2020,2023 MariaDB Corporation AB

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
#include <mutex>

#include "Consts.h"
#include "UrlParser.h"
#include "GlobalStateInfo.h"
#include "MariaDbConnection.h"
#include "ThreadPoolExecutor.h"
#include "MariaDbInnerPoolConnection.h"
#include "util/BlockingQueue.h"
#include "ConnectionEventListener.h"

namespace sql
{
class ExecutorService;

namespace mariadb
{
class MariaDbConnection;

class Pool
{
  typedef sql::blocking_deque<MariaDbInnerPoolConnection*> Idles;
  static Logger* logger;
  static const int32_t POOL_STATE_OK= 0;
  static const int32_t POOL_STATE_CLOSING= 1;
  std::atomic<int32_t> poolState;

  Shared::UrlParser urlParser;
  const Shared::Options options;
  std::atomic<int32_t> pendingRequestNumber;
  std::atomic<int32_t> totalConnection;
  Idles idleConnections;
  /* Queue must go before appender */
  sql::blocking_deque<Runnable> connectionAppenderQueue;
  // poolTag must be before connectionAppender
  std::string poolTag;
  ThreadPoolExecutor connectionAppender;
  ScheduledThreadPoolExecutor& poolExecutor;
  std::unique_ptr<ScheduledFuture> scheduledFuture;
  /*GlobalStateInfo globalInfo;
  int32_t maxIdleTime;
  int64_t timeToConnectNanos;
  int64_t connectionTime;*/
  std::mutex listsLock;
  uint32_t waitTimeout= 28800;

public:
  Pool(Shared::UrlParser& _urlParser, int32_t poolIndex, ScheduledThreadPoolExecutor& poolExecutor);
  ~Pool();

private:
  void addConnectionRequest();
  void removeIdleTimeoutConnection();
  void addConnection();
  MariaDbInnerPoolConnection* getIdleConnection();
  MariaDbInnerPoolConnection* getIdleConnection(const ::mariadb::Timer::Clock::duration& timeout);
  void silentCloseConnection(MariaDbConnection& item);
  void silentAbortConnection(MariaDbInnerPoolConnection& item);

  //MariaDbInnerPoolConnection& createPoolConnection(MariaDbConnection* connection);

public:
  MariaDbInnerPoolConnection* getPoolConnection();
  MariaDbInnerPoolConnection* getPoolConnection(const SQLString& username, const SQLString& password);

private:
  std::string generatePoolTag(int32_t poolIndex);

public:
  const UrlParser& getUrlParser();
  void close();

private:
  void closeAll(Idles& collection);
  //void initializePoolGlobalState(MariaDbConnection& connection);

public:
  std::string getPoolTag();
  //bool equals(sql::Object* obj);
  //int64_t hashCode();
  //GlobalStateInfo getGlobalInfo();
  int64_t getActiveConnections();
  int64_t getTotalConnections();
  int64_t getIdleConnections();
  int64_t getConnectionRequests();

  std::vector<int64_t> testGetConnectionIdleThreadIds();

  // Possibly it would be better to make these lambdas
  void connectionClosed(ConnectionEvent& event);
  void connectionErrorOccurred(ConnectionEvent& event);
};

}
}
#endif
