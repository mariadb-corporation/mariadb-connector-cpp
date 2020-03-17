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

namespace sql
{
namespace mariadb
{
  std::atomic<int32_t> Pools::poolIndex;
  std::shared_ptr<ScheduledThreadPoolExecutor> Pools::poolExecutor;
  /* TODO: change to std::unordered_map */
  HashMap<UrlParser, Shared::Pool> Pools::poolMap;

  //const std::map<UrlParser&, Pool>poolMap;// =new ConcurrentHashMap<>();
  /* ScheduledThreadPoolExecutor* Pools::poolExecutor= NULL; */

  /**
    * Get existing pool for a configuration. Create it if doesn't exists.
    *
    * @param urlParser configuration parser
    * @return pool
    */
  Shared::Pool Pools::retrievePool(std::shared_ptr<UrlParser>& urlParser)
  {
    auto cit= poolMap.find(*urlParser);
    if (cit == poolMap.end())
    {
      //synchronized(poolMap)
      {
        cit= poolMap.find(*urlParser);

        if (cit == poolMap.end())
        {
          if (!poolExecutor)
          {
            poolExecutor.reset(new ScheduledThreadPoolExecutor()); //1, new MariaDbThreadFactory("MariaDbPool-maxTimeoutIdle-checker"));
          }
          Shared::Pool pool(new Pool(urlParser, ++poolIndex, poolExecutor));
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
    if (poolMap.find(*pool.getUrlParser()) != poolMap.end())
    {
      //synchronized(poolMap)
      {
        if (poolMap.find(*pool.getUrlParser()) != poolMap.end())
        {
          poolMap.remove(*pool.getUrlParser());
          shutdownExecutor();
        }
      }
    }
  }

  /** Close all pools. */
  void Pools::close()
  {
    //synchronized(poolMap)
    {
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
    //synchronized(poolMap)
    {
      for (auto it : poolMap)
      {
        if (poolName.compare(it.second->getUrlParser()->getOptions()->poolName) == 0)
        {
          try
          {
            it.second->close();
          }
          catch (std::exception&)
          {
          }
          poolMap.remove(*it.second->getUrlParser());
          return;
        }
      }

      if (poolMap.empty())
      {
        shutdownExecutor();
      }
    }
  }

  void Pools::shutdownExecutor()
  {
    poolExecutor->shutdown();
    try
    {
      poolExecutor->awaitTermination(10, TimeUnit::SECONDS);
    }
    catch (std::exception&)
    {
    }
    poolExecutor= NULL;
  }

}
}