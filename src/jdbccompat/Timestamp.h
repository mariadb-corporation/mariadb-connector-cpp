/************************************************************************************
   Copyright (C) 2026 MariaDB Corporation AB

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


#ifndef _CONCPPMARIADBTIMESTAMP_H_
#define _CONCPPMARIADBTIMESTAMP_H_

#include "StringImp.h"
#include "conncpp/Timestamp.hpp"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"
}
extern Timestamp nullTs;
typedef mariadb::capi::MYSQL_TIME TimestampStruct;

}

class TimestampImp {
  
public:
  
  mariadb::TimestampStruct ts;
  TimestampImp();
  TimestampImp(uint32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s, uint64_t n);
  static mariadb::TimestampStruct& get(sql::Timestamp& timestamp);
  static const mariadb::TimestampStruct& get(const sql::Timestamp& timestamp);
  static Timestamp& assign(Timestamp& left, const mariadb::TimestampStruct* right);
  static bool before(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static bool after(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static bool equals(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static SQLString toString(mariadb::TimestampStruct& time, std::size_t decimals= 9);
};

}
#endif
