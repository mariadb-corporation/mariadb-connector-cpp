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
  constexpr std::size_t maxPoolSize= 4;
  std::size_t i;
  sql::Properties p;
  std::array<Connection, maxPoolSize> c;
  std::vector<int32_t> connection_id(4);
  bool verbosity= TestsListener::setVerbose(true);

  p["user"] = user;
  p["password"] = passwd;
  p["useTls"] = useTls ? "true" : "false";
  p["pool"] = "true";
  p["minPoolSize"]= "2";
  p["maxPoolSize"]= std::to_string(maxPoolSize);
  p["testMinRemovalDelay"]= "10";

  for (i = 0; i < maxPoolSize; ++i) {
    c[i].reset(driver->connect(url, p));
    ASSERT(c[i].get());
    stmt.reset(c[i]->createStatement());
    res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
    ASSERT(res->next());
    connection_id.push_back(res->getInt(1));
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
  TestsListener::messagesLog() << std::hex << c[3].get() << res->getInt(1) << std::endl;

  ASSERT(contains(connection_id, res->getInt(1)));

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
      ASSERT(contains(connection_id, res->getInt(2)));
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
  localUrl.append("minPoolSize=1&maxPoolSize=1&connectTimeout=6000&testMinRemovalDelay=6");

  sql::mariadb::MariaDbDataSource ds(localUrl);
  int32_t connId;

  con.reset(ds.getConnection(user, passwd));
  stmt.reset(con->createStatement());
  res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
  ASSERT(res->next());
  connId= res->getInt(1);
  TestsListener::messagesLog() << "ThreadId:" << connId  << std::endl;

  try {
    Connection c2(ds.getConnection(user, passwd));
    FAIL("Requesting connection beyond pool max capacity should yield an exception");
  }
  catch (sql::SQLException&) {
    // Fine
  }

  // This supposed close the pool, thus new connection should be not from there.
  ds.close();
  // That also means, that all connections in the pool are closed. Thus we should
  // get rid of those pointers. TODO: maybe this should still be cared of by the connector? not sure if that is possible
  con.release(); // or reset() before ds.close()

  con.reset(ds.getConnection());
  stmt.reset(con->createStatement());
  res.reset(stmt->executeQuery("SELECT CONNECTION_ID()"));
  ASSERT(res->next());
  TestsListener::messagesLog() << "ThreadId:" << res->getInt(1) << std::endl;
  ASSERT(connId != res->getInt(1));
}

} /* namespace connection */
} /* namespace testsuite */
