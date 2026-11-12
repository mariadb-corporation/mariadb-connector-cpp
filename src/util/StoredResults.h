/************************************************************************************
   Copyright (C) 2026 MariaDB Corporation plc

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


#ifndef _STOREDRESULTS_H_
#define _STOREDRESULTS_H_

#include <mutex>
#include <unordered_map>
#include <vector>

#include "SelectResultSet.h"

namespace sql
{
namespace mariadb
{

/* Registry of binary resultsets, whose rows are still owned by the C/C statement handle(i.e. they have been
   read with mysql_stmt_store_result, but have not been copied to the connector's memory).
   Those rows stop being accessible once the handle is executed again, closed, moved to the next result, or
   its connection is closed/reset. Thus everything doing that, has to call cache/cacheAll first, to make
   affected resultsets to copy their data. That lets not to make that copy for each resultset(CONCPP-138)

   The object is shared between the protocol and resultsets, since a resultset may outlive the connection */
class StoredResults
{
  // Recursive since resultset unregisters itself in the course of the caching
  std::recursive_mutex lock;
  std::unordered_map<capi::MYSQL_STMT*, SelectResultSet*> pending;

  static void makeLocal(SelectResultSet* rs)
  {
    try {
      rs->cacheCompleteLocally();
    }
    catch (std::exception&) {
      // Nothing we can do here about it
    }
  }

public:
  void add(capi::MYSQL_STMT* stmt, SelectResultSet* rs)
  {
    std::lock_guard<std::recursive_mutex> guard(lock);
    auto it= pending.find(stmt);

    if (it != pending.end()) {
      SelectResultSet* previous= it->second;
      if (previous == rs) {
        return;
      }
      // Normally should not happen - whatever makes previous result unavailable, has to care about its caching
      pending.erase(it);
      makeLocal(previous);
    }
    pending.emplace(stmt, rs);
  }

  void remove(capi::MYSQL_STMT* stmt, SelectResultSet* rs)
  {
    std::lock_guard<std::recursive_mutex> guard(lock);
    auto it= pending.find(stmt);

    if (it != pending.end() && it->second == rs) {
      pending.erase(it);
    }
  }

  /* Makes resultset of the stmt(if any) to copy its data */
  void cache(capi::MYSQL_STMT* stmt)
  {
    std::lock_guard<std::recursive_mutex> guard(lock);
    if (pending.empty()) {
      return;
    }
    auto it= pending.find(stmt);

    if (it != pending.end()) {
      SelectResultSet* rs= it->second;
      pending.erase(it);
      makeLocal(rs);
    }
  }

  /* Makes all registered resultsets to copy their data */
  void cacheAll()
  {
    std::lock_guard<std::recursive_mutex> guard(lock);
    if (pending.empty()) {
      return;
    }
    std::vector<SelectResultSet*> all;

    all.reserve(pending.size());
    for (auto& it : pending) {
      all.push_back(it.second);
    }
    pending.clear();
    for (auto rs : all) {
      makeLocal(rs);
    }
  }
};

namespace Shared
{
  typedef std::shared_ptr<sql::mariadb::StoredResults> StoredResults;
}

}
}
#endif
