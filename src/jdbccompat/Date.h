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


#ifndef _CONCPPMARIADBDATE_H_
#define _CONCPPMARIADBDATE_H_

#include "conncpp/Date.hpp"
#include "Timestamp.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"
}
extern const sql::Date nullDate;
}
class DateImp {

public:
  mariadb::TimestampStruct date;
  DateImp();
  DateImp(uint32_t y, uint32_t mo= 1, uint32_t d= 1);
  static mariadb::TimestampStruct& get(sql::Date& timestamp);
  static const mariadb::TimestampStruct& get(const sql::Date& timestamp);
  static bool before(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static bool after(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static bool equals(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them);
  static SQLString toString(mariadb::TimestampStruct& date);
};
}
#endif
