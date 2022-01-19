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


#include "Pools.h"
#include "Pool.h"
#include "ThreadPoolExecutor.h"
#include "MariaDbThreadFactory.h"

namespace sql
{
namespace mariadb
{
  std::mutex Pools::mapLock;
  std::atomic<int32_t> Pools::poolIndex;
  /* TODO: change to std::unordered_map */
  HashMap<UrlParser, Shared::Pool> Pools::poolMap;
  std::unique_ptr<ScheduledThreadPoolExecutor> Pools::poolExecutor;
  

  //const std::map<UrlParser&, Pool>poolMap;// =new ConcurrentHashMap<>();
  /* ScheduledThreadPoolExecutor* Pools::poolExecutor= nullptr; */

  /**
    * Get existing pool for a configuration. Create it if doesn't exists.
    *
    * @param urlParser configuration parser
    * @return pool
    */
  Shared::Pool Pools::retrievePool(Shared::UrlParser& urlParser)
  {
    auto cit= poolMap.find(*urlParser);
    if (cit == poolMap.end())
    {
      std::unique_lock<std::mutex> lock(mapLock);
      {
        // TODO: it should also check if the pool is active, i.e. not closing atm
        cit= poolMap.find(*urlParser);

        if (cit == poolMap.end())
        {
          if (!poolExecutor)
          {
            poolExecutor.reset(new ScheduledThreadPoolExecutor(1, new MariaDbThreadFactory("MariaDbPool-maxTimeoutIdle-checker")));
          }
          Shared::Pool pool(new Pool(urlParser, ++poolIndex, *poolExecutor));
          poolMap.insert(*urlParser, pool);

          return pool;
        }
      }
    }

    return cit->second;
  }

  /**
    * Remove pool.
    *
    * @param pool pool to remove
    */
  void Pools::remove(Pool &pool)
  {
    if (poolMap.find(pool.getUrlParser()) != poolMap.end())
    {
      std::unique_lock<std::mutex> lock(mapLock);

      if (poolMap.find(pool.getUrlParser()) != poolMap.end())
      {
        poolMap.remove(pool.getUrlParser());
        if (poolMap.empty()) {
          shutdownExecutor();
        }
      }
    }
  }

  /** Close all pools. */
  void Pools::close()
  {
    std::unique_lock<std::mutex> lock(mapLock);
    for (auto it : poolMap)
    {
      try {
        it.second->close();
      }
      catch (std::exception&) {

      }
    }
    shutdownExecutor();
    poolMap.clear();
  }

  /**
    * Closing a pool with name defined in url.
    *
    * @param poolName the option "poolName" value
    */
  void Pools::close(const SQLString& poolName)
  {
    if (poolName.empty())
    {
      return;
    }

    std::unique_lock<std::mutex> lock(mapLock);
    for (auto it : poolMap)
    {
      if (poolName.compare(it.second->getUrlParser().getOptions()->poolName) == 0)
      {
        try
        {
          it.second->close();
        }
        catch (std::exception&)
        {
        }
        poolMap.remove(it.second->getUrlParser());
        return;
      }
    }

    if (poolMap.empty())
    {
      shutdownExecutor();
    }
  }

  void Pools::shutdownExecutor()
  {
    poolExecutor->shutdown();
    try
    {
      //poolExecutor->awaitTermination(std::chrono::seconds(10));
    }
    catch (std::exception&)
    {
    }
    poolExecutor= nullptr;
  }

}
}