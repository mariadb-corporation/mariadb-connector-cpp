/*
 * Copyright (c) 2024 MariaDB Corporation plc
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

#include "Warning.hpp"
#include "Connection.hpp"
#include "Exception.hpp"
#include "MariaDbDataSource.hpp"

#include "pool2.h"
#include "timer.h"

using namespace std::chrono;

namespace testsuite
{
namespace classes
{

class Client : public mariadb::EntityTimer
{
  static constexpr uint32_t valMaxLen= 64, maxAttrVal= 25;
  static const sql::Driver *const driver;

  std::mt19937 rng;
  sql::Properties &p;
  std::thread t;
  Connection c;
  Statement stmt;
  PreparedStatement pstmt;
  ResultSet rs;
  //std::map<uint32_t, std::pair<uint32_t, uint32_t>> stats;

  void connect()
  {
    //TestsListener::messagesLog() << "->Resetting with new connection";
    c.reset();
    start(doConnect);
    while (!c)
    {
      try
      {
        c.reset(driver->connect(p));
      }
      catch (sql::SQLException &e)
      {
        // To remember number of refusals
        start(connectRefused);
        TestsListener::messagesLog() << e.getMessage() << std::endl;
        stop(connectRefused);
      }
    }
    stop(doConnect);
    ASSERT(c.get() != nullptr);
    //TestsListener::messagesLog() << "->New object:" << std::hex << c.get() << std::dec;
    ASSERT(!c->isClosed());
  }

  void rndString(sql::SQLString& str, uint32_t words= 0)
  {
    static const std::string word[]={"I", "test", "MariaDB", "we", "still", "like", "db", "should", "value", "be",
      "better", "pool", "connection", "not", "always", "again", "me", "us", "funny", "nonsense"};
    std::size_t idx;
    char sep= '%';

    if (words == 0)
    {
      words= rng() % 10 + 1;
      sep= ' ';
    }

    for (uint32_t i= 0; i < words && str.length() + 2 < valMaxLen; ++i)
    {
      idx= rng() % (sizeof(word) / sizeof(std::string));
      if (str.length() + 1/*sep*/ + word[idx].length() <= valMaxLen)
      {
        if (i)
          str.append(sep);
        str.append(word[idx]);
      }
    }
  }

  void insert()
  {
    sql::SQLString val;
    if (!c || c->isClosed())
      connect();

    val.reserve(valMaxLen);

    start(doInsert);
    pstmt.reset(c->prepareStatement("INSERT INTO ccpp_pool2_test(ref_id, val) VALUES(?,?)"));

    pstmt->setInt(1, rng() % maxAttrVal + 1);
    pstmt->setString(2, val);
    // Should really try/catch
    pstmt->execute();
    stop(doInsert);
  }

  int32_t getMaxId()
  {
    start(doSelect);
    stmt.reset(c->createStatement());
    rs.reset(stmt->executeQuery("SELECT max(id) FROM ccpp_pool2_test"));
    stop(doSelect);
    rs->next();
    auto res= rs->getInt(1);
    if (res == 0)
    {
      res= 1;
    }
    return res;
  }

  void update()
  {
    sql::SQLString val;
    if (!c || c->isClosed())
      connect();

    val.reserve(valMaxLen);

    start(doUpdate);
    pstmt.reset(c->prepareStatement("UPDATE ccpp_pool2_test SET ref_id=?,val=? WHERE id=?"));

    pstmt->setInt(1, rng() % maxAttrVal + 1);
    pstmt->setString(2, val);
    pstmt->setInt(3, rng() % getMaxId() + 1);
    // Should really try/catch
    pstmt->execute();
    stop(doUpdate);
  }

  void delete_row()
  {
    if (!c || c->isClosed())
      connect();

    start(doDelete);
    pstmt.reset(c->prepareStatement("DELETE FROM ccpp_pool2_test WHERE id=?"));

    pstmt->setInt(1, rng() % getMaxId() + 1);
    // Should really try/catch
    pstmt->execute();
    stop(doDelete);
  }

  void select(sql::PreparedStatement *query)
  {
    // start should be done before prepare
    //start(doSelect);
    rs.reset(query->executeQuery());
    stop(doSelect);
    while (rs->next())
    {
      rs->getInt(1);
      rs->getInt(2);
      rs->getString(3);
    }
  }

  void rndCmpSign(sql::SQLString &query)
  {
    switch (rng() % 3)
    {
    case 0: query.append('='); break;
    case 1: query.append('>'); break;
    default: query.append('<'); break;
    }
  }

  void select()
  {
    sql::SQLString val, query("SELECT id, ref_id, val FROM ccpp_pool2_test WHERE ");
    if (!c || c->isClosed())
      connect();

    val.reserve(valMaxLen);

    switch (rng() % 3)
    {
    case 0:
    {
      auto maxId= getMaxId();
      query.reserve(query.length() + 5);
      query.append("id");
      rndCmpSign(query);
      query.append('?');

      start(doSelect);
      pstmt.reset(c->prepareStatement(query));
      pstmt->setInt(1, rng() % maxId + 1);
      break;
    }
    case 1:
      query.reserve(query.length() + 9);
      query.append("ref_id=?");
      start(doSelect);
      pstmt.reset(c->prepareStatement(query));
      pstmt->setInt(1, rng() % maxAttrVal + 1);
      break;
    default:
      query.reserve(query.length() + 7);
      query.append("val like '%");
      rndString(query, 3);
      query.append("val like %'");
      start(doSelect);
      pstmt.reset(c->prepareStatement(query));
      break;
    }
    select(pstmt.get());
  }

public:
  static const uint32_t doNothing, closeConn, delConn, doInsert, doUpdate, doDelete,
    doSelect, doConnect, connectRefused;

  Client(uint32_t seed, sql::Properties &props, uint32_t ttlSeconds)
    : mariadb::EntityTimer(std::chrono::seconds(100))
    , rng(seed)
    , p(props)
    , t(std::bind(&Client::Worker, this))
  {

  }

  bool joinable() { return t.joinable(); }
  void join()
  {
    t.join();
    c.reset();
    rs.reset();
    pstmt.reset();
    stmt.reset();

  }

  ~Client()
  {
    if (t.joinable())
      t.join();
  }

private:
  void Worker();
};

const sql::Driver *const Client::driver= sql::mariadb::get_driver_instance();
const uint32_t Client::doNothing= 0, Client::closeConn= 1, Client::delConn= 2, Client::doInsert= 3, Client::doUpdate= 4, Client::doDelete= 5,
Client::doSelect= 6, Client::doConnect= 7, Client::connectRefused= 8;


void Client::Worker()
{
  // To let pool to warm up
  std::this_thread::sleep_for(std::chrono::seconds(1));

  while (!over())
  {
    switch (rng() % doConnect)
    {
    case doNothing:
      // Well could just memorize this time :innocent:
      start(doNothing);
      std::this_thread::sleep_for(std::chrono::microseconds(rng() % 1000));
      stop(doNothing);
      break;
    case closeConn:
      if (c)
      {
        start(closeConn);
        c->close();
        stop(closeConn);
        //TestsListener::messagesLog() << "-> closing()" << std::endl;
      }
      break;
    case delConn:
      if (c)
      {
        start(delConn);
        c.reset(nullptr);
        stop(delConn);
      }
      break;
    case doInsert:
      insert();
      break;
    case doUpdate:
      update();
      break;
    case doDelete:
      delete_row();
      break;
    case doSelect:
    default:
      select();
    }
  }
}


void pool2::pool_threaded()
{
  constexpr std::size_t minPoolSize= 3, maxPoolSize= 5, maxClientsCount= 5; // Initially was 30, 40 and 43. Probbaly too much. Possibly makes sense to make configurable
  std::array<std::unique_ptr<Client>, maxClientsCount> c;
  bool verbosity= TestsListener::setVerbose(true);
  std::random_device rd;
  std::seed_seq seq{rd(),rd(), rd()};
  std::array<uint32_t, maxClientsCount> seed;
  sql::Properties p{{"hostName", url},
                    {"user", user},
                    {"password", passwd},
                    {"useTls", useTls ? "true" : "false"},
                    {"pool", "true"},
                    {"minPoolSize", std::to_string(minPoolSize)},
                    {"maxPoolSize", std::to_string(maxPoolSize)},
                    {"testMinRemovalDelay", "10"},
                    {"connectTimeout", "10000"},
                    /*,{"log", "5"}
                    {"logname", "e.log"}*/
                   };

  createSchemaObject("TABLE", "ccpp_pool2_test", "(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,"
                                                 "ref_id INT NOT NULL,"
                                                 "val VARCHAR(64) NOT NULL, INDEX forSearch(val))");
  seq.generate(seed.begin(), seed.end());

  for (std::size_t i= 0; i < maxClientsCount; ++i)
  {
    c[i].reset(new Client(seed[i], p, 1000));
  }
  std::size_t joined= 0;
  while (joined < maxClientsCount) {
    for (std::size_t i= 0; i < maxClientsCount; ++i)
    {
      if (c[i]->joinable()) {
        ++joined;
        c[i]->join();
        const Client::Stats& stats= c[i]->getStats();

        TestsListener::messagesLog() << "Client #" << i + 1 << std::endl;
        const auto& connects= stats.find(Client::doConnect);
        if (connects != stats.end())
        {
          TestsListener::messagesLog() << " Connects count " << connects->second.size() << std::endl;
          TestsListener::messagesLog() << "   avg " << duration_cast<microseconds>(Client::average(connects->second)).count() << "mks" << std::endl;
          TestsListener::messagesLog() << "   max " << duration_cast<microseconds>(Client::maxVal(connects->second)).count() << "mks" << std::endl;
          TestsListener::messagesLog() << "   min " << duration_cast<microseconds>(Client::minVal(connects->second)).count() << "mks" << std::endl;
        }
        const auto& refusals= stats.find(Client::connectRefused);
        if (refusals != stats.end())
        {
          TestsListener::messagesLog() << " Connection refusals " << connects->second.size() << std::endl;
        }
      }
    }
  }
}


void pool2::pool_concpp97()
{
  bool verbosity= TestsListener::setVerbose(true);
  sql::Properties p{{"hostName", url},
                    {"user", user},
                    {"password", "--wrongPwd--"},
                    {"useTls", useTls ? "true" : "false"},
                    {"pool", "true"},
                    {"minPoolSize", "1"},
                    {"maxPoolSize", "2"},
                    {"testMinRemovalDelay", "10"},
                    {"connectTimeout", "10000"}
                   };
  ::mariadb::StopTimer t;
  try {
    con.reset(driver->connect(p));
  }
  catch (sql::SQLException &e)
  {
    ASSERT(std::chrono::duration_cast<std::chrono::milliseconds>(t.stop()).count() < 5000);
    if (e.getErrorCode() != 1045) {
      ASSERT_EQUALS(1698, e.getErrorCode());
    }
  }
  p["password"]= passwd;
  con.reset(driver->connect(p));
  p["password"]= "--wrongPwd--";

  t.reset();
  try {
    con.reset(driver->connect(p));
  }
  catch (sql::SQLException &e)
  {
    ASSERT(std::chrono::duration_cast<std::chrono::milliseconds>(t.stop()).count() < 5000);
    if (e.getErrorCode() != 1045) {
      ASSERT_EQUALS(1698, e.getErrorCode());
    }
  }
}
} /* namespace connection */
} /* namespace testsuite */
