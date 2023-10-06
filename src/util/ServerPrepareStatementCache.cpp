/************************************************************************************
   Copyright (C) 2020, 2023 MariaDB Corporation AB

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

namespace sql
{
namespace mariadb
{
  
  /*SQLString ServerPrepareStatementCache::toString()
  {
    SQLString stringBuilder("ServerPrepareStatementCache.map[");
    for (const auto& entry: this->cache){
      stringBuilder
        .append("\n")
        .append(entry.first)
        .append("-")
        .append(std::to_string(entry.second->getShareCounter()));
    }
    stringBuilder.append("]");
    return stringBuilder;
  }*/
}
}
