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


#include "ServerPrepareStatementCache.h"
#include "util/ServerPrepareResult.h"
#include "Protocol.h"

namespace sql
{
namespace mariadb
{

  ServerPrepareStatementCache::ServerPrepareStatementCache(uint32_t size, Shared::Protocol& protocol)
    : maxSize(size)
    , protocol(protocol)
  {
  }

  ServerPrepareStatementCache* ServerPrepareStatementCache::newInstance(uint32_t size, Shared::Protocol& protocol)
  {
    return new ServerPrepareStatementCache(size, protocol);
  }

  bool ServerPrepareStatementCache::removeEldestEntry(value_type eldest)
  {
    bool mustBeRemoved= cache.size() > maxSize;

    if (mustBeRemoved){
      ServerPrepareResult* serverPrepareResult= eldest.second;
      serverPrepareResult->setRemoveFromCache();
      if (serverPrepareResult->canBeDeallocate()) {
        try {
          protocol->forceReleasePrepareStatement(serverPrepareResult->getStatementId());
        }catch (SQLException&){

        }
      }
    }
    return mustBeRemoved;
  }

  /**
   * Associates the specified value with the specified key in this map. If the map previously
   * contained a mapping for the key, the existing cached prepared result shared counter will be
   * incremented.
   *
   * @param key key
   * @param result new prepare result.
   * @return the previous value associated with key if not been deallocate, or null if there was no
   *     mapping for key.
   */
  ServerPrepareResult* ServerPrepareStatementCache::put(const SQLString& key, ServerPrepareResult* result)
  {
    std::lock_guard<std::mutex> localScopeLock(lock);

    iterator cachedServerPrepareResult= cache.find(StringImp::get(key));

    if (cachedServerPrepareResult != cache.end() && cachedServerPrepareResult->second->incrementShareCounter()){
      return cachedServerPrepareResult->second;
    }

    result->setAddToCache();
    cache.emplace(StringImp::get(key), result);

    return nullptr;
  }


  /* Calls have to be guarded*/
  ServerPrepareResult* ServerPrepareStatementCache::get(const SQLString& key)
  {

    iterator cachedServerPrepareResult= cache.find(StringImp::get(key));

    if (cachedServerPrepareResult != cache.end() && cachedServerPrepareResult->second->incrementShareCounter()){
      return cachedServerPrepareResult->second;
    }

    return nullptr;
  }
  SQLString ServerPrepareStatementCache::toString()
  {
    SQLString stringBuilder("ServerPrepareStatementCache.map[");
    for (auto& entry :this->cache){
      stringBuilder
        .append("\n")
        .append(entry.first)
        .append("-")
        .append(std::to_string(entry.second->getShareCounter()));
    }
    stringBuilder.append("]");
    return stringBuilder;
  }
}
}
