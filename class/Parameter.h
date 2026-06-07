/************************************************************************************
   Copyright (C) 2023,2026 MariaDB Corporation plc

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


#ifndef _PARAMETER_H_
#define _PARAMETER_H_

#include "mysql.h"
#include "SQLString.h"


namespace mariadb
{

  class Parameter
{
  static void* getBuffer(MYSQL_BIND& params, std::size_t row);
  static unsigned long getLength(MYSQL_BIND& params, std::size_t row);

public:
  // These functions need to MYSQL handle as it contains info about connections charset and escaping
  // cannot be done correctly w/out it. Also, it knows about "no backslash escapes" status, so we don't need to pass it.
  static SQLString& toString(MYSQL* conn, SQLString& query, void* value, enum enum_field_types type, unsigned long length);
  static SQLString& toString(MYSQL* conn, SQLString& query, MYSQL_BIND& param);
  static SQLString& toString(MYSQL* conn, SQLString& query, MYSQL_BIND& param, std::size_t row);

  static std::size_t getApproximateStringLength(MYSQL_BIND& param, std::size_t row);
};
}
#endif
