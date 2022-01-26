/*
 * Copyright (c) 2022 MariaDB Corporation AB
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0, as
 * published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an
 * additional permission to link the program and your derivative works
 * with the separately licensed software that they have included with
 * MySQL.
 *
 * Without limiting anything contained in the foregoing, this file,
 * which is part of MySQL Connector/C++, is also subject to the
 * Universal FOSS Exception, version 1.0, a copy of which can be found at
 * http://oss.oracle.com/licenses/universal-foss-exception.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <memory>
#include <list>
#include <array>
#include <random>
#include <thread>
#include <chrono>

#include "Warning.hpp"
#include "Connection.hpp"
#include "Exception.hpp"
#include "MariaDbDataSource.hpp"

#include "pool.h"

namespace testsuite
{
namespace classes
{

void pool::pool_simple()
{
  constexpr std::size_t minPoolSize= 2, maxPoolSize= 4;
  std::size_t i;
  sql::Properties p;
  std::array<Connection, maxPoolSize> c;
  std::vector<int32_t> connection_id(maxPoolSize);
  bool verbosity= TestsListener::setVerbose(true);

  p["user"] = user;
  p["password"] = passwd;
  p["useTls"] = useTls ? "true" : "false";
  p["pool"] = "true";
  p["minPoolSize"]= std::to_string(minPoolSize);
  p["maxPoolSize"]= std::to_string(maxPoolSize);
  p["testMinRemovalDelay"]= "10";

  for (i = 0; i < maxPoolSize; ++i) {
    c[i].reset(driver->connect(url, p));
    ASSERT(c[i].get());
    stmt.reset(c[i]->createStatement());
    res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
    ASSERT(res->next());
    connection_id[i]= (res->getInt(1));
    TestsListener::messagesLog() << std::hex << c[i].get() << i << ":" << res->getInt(1) << std::endl;
  }

  c[3]->close();
  ASSERT(c[3]->isClosed());
  c[1]->close();
  ASSERT(c[1]->isClosed());

  c[3].reset(sql::DriverManager::getConnection(url, p));
  ASSERT(c[3].get());
  stmt.reset(c[3]->createStatement());
  res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
  ASSERT(res->next());
  int32_t connid= res->getInt(1);
  TestsListener::messagesLog() << std::hex << c[3].get() << "3:" << connid << std::dec << std::endl;

  if (isSkySqlHA()) {
    if (!contains(connection_id, connid)) {
      connection_id.push_back(res->getInt(1));
    }
  }
  else {
    ASSERT(contains(connection_id, res->getInt(1)));
  }

  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  for (i = 0; i < 500; ++i) {
    std::size_t rnd= std::rand() % 4;
    //TestsListener::messagesLog() << "Iteration:" << i << ", c:" << rnd << "," << std::hex << c[rnd].get() << std::dec;
    if (std::rand() % 2) {
      if (!c[rnd] || c[rnd]->isClosed()) {
        //TestsListener::messagesLog() << "->Resetting with new connection";
        c[rnd].reset(driver->connect(url, p));
        ASSERT(c[rnd].get() != nullptr);
        //TestsListener::messagesLog() << "->New object:" << std::hex << c[rnd].get() << std::dec;
        ASSERT(!c[rnd]->isClosed());
      }
      stmt.reset(c[rnd]->createStatement());
      res.reset(stmt->executeQuery("SELECT 1, CONNECTION_ID()"));
      ASSERT(res->next());
      //TestsListener::messagesLog() << " ThreadId:" << res->getInt(2);
      if (isSkySqlHA()) {
        connid= res->getInt(2);
        if (!contains(connection_id, connid)) {
          connection_id.push_back(connid);
          // Assuming maxscale has 2 servers behind it
          ASSERT(connection_id.size() <= maxPoolSize*2);
        }
      }
      else {
        ASSERT(contains(connection_id, res->getInt(2)));
      }
      ASSERT(!res->next());
    }
    switch (std::rand() % 3) {
    case 0:
      if (c[rnd]) {
        c[rnd]->close();
        //TestsListener::messagesLog() << "-> closing()" << std::endl;
        break;
      }
    case 1:
      //TestsListener::messagesLog() << "-> destructed" << std::endl;
      c[rnd].reset(nullptr);
      break;
    default:
      //TestsListener::messagesLog() << "-> just sleeping" << std::endl;
      std::this_thread::sleep_for(std::chrono::microseconds(5));
    }
  }
}


void pool::pool_datasource()
{
  sql::SQLString localUrl(url);

  if (localUrl.find_first_of('?') == sql::SQLString::npos) {
    localUrl.append('?');
  }
  localUrl.append("minPoolSize=1&maxPoolSize=1&connectTimeout=5000&testMinRemovalDelay=6");

  sql::mariadb::MariaDbDataSource ds(localUrl);
  int32_t connId;

  TestsListener::messagesLog() << "Requesting pooled connection with DataSource" << std::endl;
  con.reset(ds.getConnection(user, passwd));
  stmt.reset(con->createStatement());
  res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
  ASSERT(res->next());
  connId= res->getInt(1);
  TestsListener::messagesLog() << "ThreadId:" << connId  << std::endl;

  try {
    TestsListener::messagesLog() << "Requesting connection above maxPoolSize" << std::endl;
    Connection c2(ds.getConnection(user, passwd));
    if (c2) {
      stmt.reset(c2->createStatement());
      res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
      ASSERT(res->next());
      connId = res->getInt(1);
      TestsListener::messagesLog() << "Unexpected ThreadId:" << connId << std::endl;
    }
    FAIL("Requesting connection beyond pool max capacity should yield an exception");
  }
  catch (sql::SQLException&) {
    // Fine
  }

  TestsListener::messagesLog() << "Closing pool" << std::endl;
  // Connection should be deleted(or even just released) before pool closed
  con.reset(nullptr);
  // This supposed close the pool, thus new connection should be not from there.
  ds.close();

  TestsListener::messagesLog() << "Requesting the connection from the new pool" << std::endl;
  con.reset(ds.getConnection());
  stmt.reset(con->createStatement());
  res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
  ASSERT(res->next());
  TestsListener::messagesLog() << "ThreadId:" << res->getInt(1) << std::endl;
  ASSERT(connId != res->getInt(1));
  con.reset(nullptr);
  // We don't need too many pools
  ds.close();
}


void pool::pool_idle()
{
  constexpr std::size_t maxPoolSize = 2, minPoolSize= 1, maxIdleTime= 60;
  sql::SQLString localUrl(url);

  if (localUrl.find_first_of('?') == sql::SQLString::npos) {
    localUrl.append('?');
  }
  localUrl.append("minPoolSize=1&maxPoolSize=" + std::to_string(maxPoolSize) + "&testMinRemovalDelay=4&maxIdleTime=" + std::to_string(maxIdleTime));

  sql::mariadb::MariaDbDataSource ds(localUrl);
  std::size_t i;
  std::array<Connection, maxPoolSize> c;
  std::vector<int32_t> connection_id(maxPoolSize);
  bool verbosity = TestsListener::setVerbose(true);

  for (i = 0; i < maxPoolSize; ++i) {
    c[i].reset(ds.getConnection(user, passwd));
    ASSERT(c[i].get());
    stmt.reset(c[i]->createStatement());
    res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
    ASSERT(res->next());
    connection_id[i]= (res->getInt(1));
    TestsListener::messagesLog() << std::hex << c[i].get() << i << ":" << res->getInt(1) << std::endl;
  }

  c[1].reset(nullptr);
  std::this_thread::sleep_for(std::chrono::seconds(maxIdleTime + 10));

  c[1].reset(ds.getConnection());
  ASSERT(c[1].get());
  stmt.reset(c[1]->createStatement());
  res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
  ASSERT(res->next());
  int32_t newConnId= res->getInt(1);
  TestsListener::messagesLog() << std::hex << c[1].get() << "1:" << newConnId << std::dec << std::endl;

  ASSERT(connection_id[1] != newConnId);
  ASSERT(!contains(connection_id, newConnId));

  for (auto& cnx : c) cnx.reset(nullptr);
  ds.close();
}

} /* namespace connection */
} /* namespace testsuite */
