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


#ifndef _CALLABLESTATEMENTCACHE_H_
#define _CALLABLESTATEMENTCACHE_H_

#include <unordered_map>

#include "CallableStatement.hpp"

#include "CallableStatementCacheKey.h"
#include "Consts.h"

namespace sql
{

namespace mariadb
{
/* TODO: need means max size to enforce */
class CallableStatementCache
{
  int32_t maxSize;

  std::unordered_map<CallableStatementCacheKey, Shared::CallableStatement> Cache;

  CallableStatementCache(int32_t size);

public:

  typedef std::unordered_map<CallableStatementCacheKey, Shared::CallableStatement>::iterator iterator;
  typedef std::unordered_map<CallableStatementCacheKey, Shared::CallableStatement>::const_iterator const_iterator;

  static CallableStatementCache* newInstance(int32_t size);

  iterator find(const CallableStatementCacheKey& key);
  iterator end() { return Cache.end(); }
  void insert(const CallableStatementCacheKey& key, CallableStatement* callable);
};

}
}
#endif
