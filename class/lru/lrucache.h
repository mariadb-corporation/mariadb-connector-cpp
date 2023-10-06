/************************************************************************************
   Copyright (C) 2023 MariaDB Corporation plc

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


#ifndef _LRUCACHE_H_
#define _LRUCACHE_H_

#include <unordered_map>
#include <list>
#include <mutex>


namespace mariadb
{

  template <class KT, class VT> struct Cache
  {
    virtual ~Cache() {}

    virtual VT* put(const KT& key, VT* obj2cache) {return nullptr;}
    virtual VT* get(const KT& key) {return nullptr;}
    virtual void clear() {}
  };

  template <class T> struct DefaultRemover
  {
    void operator()(T* removedCacheEntry)
    {
      delete removedCacheEntry;
    }
  };
  // This does not care about the fate of obbjects removed from cache
  template <class KT, class VT, class Remover= DefaultRemover<VT>> class LruCache : public Cache<KT,VT>
  {
    std::mutex lock;
    typedef std::list<std::pair<KT,VT*>> ListType;
    typedef typename ListType::iterator ListIterator;

    std::size_t maxSize;
    ListType lu;
    std::unordered_map<KT, ListIterator> cache;

  protected:
    virtual void remove(ListIterator& it)
    {
      Remover()(it->second);
      cache.erase(it->first);
    }

    virtual ListIterator removeEldestEntry()
    {
      auto victim= lu.end();
      --victim;
      remove(victim);
      lu.splice(lu.begin(), lu, victim);

      return victim;
    }

  public:
    virtual ~LruCache() {}

    
    LruCache(std::size_t maxCacheSize) : maxSize(maxCacheSize)
    {
      cache.reserve(maxSize);
    }


    virtual VT* put(const KT& key, VT* obj2cache)
    {
      std::lock_guard<std::mutex> localScopeLock(lock);

      auto cached= cache.find(key);

      if (cached != cache.end())
      {
        return cached->second->second;
      }

      ListIterator it;
      if (cache.size() == maxSize)
      {
        it= removeEldestEntry();
        it->first= key;
        it->second= obj2cache;
      }
      else
      {
        lu.emplace_front(key, obj2cache);
        it= lu.begin();
      }
      cache.emplace(key, it);

      return nullptr;
    }


    virtual VT* get(const KT& key)
    {
      std::lock_guard<std::mutex> localScopeLock(lock);

      auto cached= cache.find(key);
      if (cached != cache.end())
      {
        lu.splice(lu.begin(), lu, cached->second);
        return cached->second->second;
      }
      return nullptr;
    }


    virtual void clear()
    {
      std::lock_guard<std::mutex> localScopeLock(lock);
      cache.clear();
      for (auto it= lu.begin(); it != lu.end();++it)
      {
        if (it->second != nullptr)
        {
          Remover()(it->second);
        }
      }
      lu.clear();
    }
  };

}

#endif
