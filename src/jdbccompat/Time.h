/************************************************************************************
   Copyright (C) 2025 MariaDB Corporation AB

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


#ifndef _CONCPPMARIADBTIME_H_
#define _CONCPPMARIADBTIME_H_

#include "Timestamp.h"
#include "conncpp/Time.hpp"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"
}

extern Time nullTime;
}

class TimeImp {

public:
  mariadb::TimestampStruct time;
  TimeImp();
  TimeImp(bool negative, uint32_t h, uint32_t mi, uint32_t s, uint64_t n);
  static mariadb::TimestampStruct& get(sql::Time& timestamp);
  static const mariadb::TimestampStruct& get(const sql::Time& timestamp);
  static bool before(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static bool after(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static bool equals(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static SQLString toString(mariadb::TimestampStruct& time, std::size_t decimals= 9);
};

} // sql
#endif
