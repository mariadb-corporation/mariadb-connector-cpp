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


#ifndef _PSCACHE_H_
#define _PSCACHE_H_

#include <string>
#include "lrucache.h"

namespace mariadb
{
  template <class T> struct PsRemover
  {
    // std::mutex lock;
    void operator()(T* removedCacheEntry)
    {
      if (removedCacheEntry->canBeDeallocate()) {
        delete removedCacheEntry;
      }
      else {
        removedCacheEntry->decrementShareCounter();
      }
    }
  };

  template <class VT> class PsCache : public LruCache<std::string,VT, PsRemover<VT>>
  {
    typedef LruCache<std::string, VT, PsRemover<VT>> parentLru;
    std::size_t maxKeyLen;

  public:
    virtual ~PsCache()
    {
    }

    PsCache(std::size_t maxCacheSize, std::size_t _maxKeyLen= static_cast<std::size_t>(-1))
      : parentLru(maxCacheSize)
      , maxKeyLen(_maxKeyLen)
    {
    }


    virtual VT* put(const std::string& key, VT* obj2cache)
    {
      if (key.length() > maxKeyLen) {
        return nullptr;
      }

      VT* hadCached= this->parentLru::put(key, obj2cache);
      
      if (hadCached == nullptr)
      {
        // Putting to cache creates additional reference
        obj2cache->incrementShareCounter();
      }

      return hadCached;
    }


    virtual VT* get(const std::string& key)
    {
      auto result= this->parentLru::get(key);
      if (result != nullptr)
      {
        result->incrementShareCounter();
      }
      return result;
    }
  };

}

#endif
