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


/* Very simplified version, as I don't think we need it */
#ifndef _POOLS_H_
#define _POOLS_H_

#include <atomic>
#include <map>
#include <memory>

#include "UrlParser.h"
#include "Pool.h"

namespace sql
{
namespace mariadb
{
//class Pool;

/*Stub to enable build */
class ScheduledThreadPoolExecutor
{
public:
  //ScheduledThreadPoolExecutor(int32_t, MariaDbThreadFactory*) {}
  void shutdown() {}
  void awaitTermination(int32_t time, enum TimeUnit unit) {}
};


template <class HASHABLEKEY, class VT> class HashMap
{
  std::map<int64_t, VT> realMap;

public:
  typedef typename std::map<int64_t, VT>::iterator iterator;
  typedef typename std::map<int64_t, VT>::const_iterator const_iterator;

  void put(HASHABLEKEY& hashableObj, VT& valueObj)
  {
    realMap[hashableObj.hashCode()]= valueObj;
  }

  void insert(const HASHABLEKEY& hashableObj, VT& valueObj)
  {
    realMap.insert({ hashableObj.hashCode(), valueObj });
  }

  void remove(HASHABLEKEY& hashableObj)
  {
    realMap.erase(hashableObj.hashCode());
  }

  void clear()
  {
    realMap.clear();
  }

  bool empty() const
  {
    return realMap.empty();
  }
  typename std::map<int64_t, VT>::iterator find(const HASHABLEKEY& hashableObj)
  {
    return realMap.find(hashableObj.hashCode());
  }

  typename std::map<int64_t, VT>::iterator begin()
  {
    return realMap.begin();
  }

  typename std::map<int64_t, VT>::iterator end()
  {
    return realMap.end();
  }

  typename std::map<int64_t, VT>::const_iterator find(const HASHABLEKEY& hashableObj) const
  {
    return realMap.find(hashableObj.hashCode());
  }

  typename std::map<int64_t, VT>::const_iterator cbegin() const
  {
    return realMap.cbegin();
  }

  typename std::map<int64_t, VT>::const_iterator cend() const
  {
    return realMap.cend();
  }
};


class Pools
{
    static std::atomic<int32_t> poolIndex ; /*new std::atomic<int32_t>()*/
    static HashMap<UrlParser,Shared::Pool> poolMap; /*new ConcurrentHashMap<>()*/
    static std::shared_ptr<ScheduledThreadPoolExecutor> poolExecutor; /*NULL*/

  public:
    static Shared::Pool retrievePool(std::shared_ptr<UrlParser>& urlParser);
    static void remove(Pool& pool);
    static void close();
    static void close(const SQLString& poolName);

  private:
    static void shutdownExecutor();
};

}
}
#endif
