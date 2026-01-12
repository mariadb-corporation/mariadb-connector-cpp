/************************************************************************************
   Copyright (C) 2026 MariaDB Corporation plc

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


#ifndef _CONCPPDATE_HPP_
#define _CONCPPDATE_HPP_

#include <chrono>
#include "SQLString.hpp"

namespace sql
{
class DateImp;

class Date {
  friend class DateImp;
  DateImp* internal= nullptr;

public:
  MARIADB_EXPORTED Date();
  MARIADB_EXPORTED Date(const char* date, std::size_t len);
  MARIADB_EXPORTED Date(uint32_t y, uint32_t mo= 1, uint32_t d= 1);
  Date(const SQLString& date) : Date(date.c_str(), date.length()) {
  }

  MARIADB_EXPORTED Date(const std::chrono::time_point<std::chrono::system_clock> ts);
  MARIADB_EXPORTED Date(Date&& other);
  MARIADB_EXPORTED Date(const Date& other);
  MARIADB_EXPORTED ~Date();

  MARIADB_EXPORTED uint32_t getYear() const;
  MARIADB_EXPORTED uint32_t getMonth() const;
  MARIADB_EXPORTED uint32_t getDate() const;

  MARIADB_EXPORTED void setYear(uint32_t year);
  MARIADB_EXPORTED void setMonth(uint32_t month);
  MARIADB_EXPORTED void setDate(uint32_t day);

  MARIADB_EXPORTED bool isNull() const;
  MARIADB_EXPORTED SQLString toString() const;

  MARIADB_EXPORTED bool before(const Date& other) const;
  MARIADB_EXPORTED bool after(const Date& other) const;
  MARIADB_EXPORTED bool equals(const Date& other) const;
  inline int32_t compareTo(const Date& other) const {
    if (before(other)) {
      return -1;
    }
    else if (after(other)) {
      return 1;
    }
    return 0;
  }
};
}
#endif
