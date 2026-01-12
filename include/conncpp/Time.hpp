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


#ifndef _CONCPPTIME_HPP_
#define _CONCPPTIME_HPP_

#include <chrono>
#include "buildconf.hpp"
#include "SQLString.hpp"

namespace sql
{

class TimeImp;

class Time {
  friend class TimeImp;
  TimeImp* internal;

public:
  MARIADB_EXPORTED Time();
  MARIADB_EXPORTED Time(const char* time, std::size_t len);
  Time(const SQLString& time) : Time(time.c_str(), time.length()) {
  }

  MARIADB_EXPORTED Time(uint32_t h, uint32_t mi, uint32_t s, uint64_t n= 0, bool negative= false);

  MARIADB_EXPORTED Time(const std::chrono::time_point<std::chrono::system_clock> time);

  MARIADB_EXPORTED ~Time();

  MARIADB_EXPORTED uint32_t getHours() const;
  MARIADB_EXPORTED uint32_t getMinutes() const;
  MARIADB_EXPORTED uint32_t getSeconds() const;
  MARIADB_EXPORTED uint64_t getNanos() const;

  MARIADB_EXPORTED void setHours(uint32_t hours);
  MARIADB_EXPORTED void setMinutes(uint32_t minutes);
  MARIADB_EXPORTED void setSeconds(uint32_t seconds);
  MARIADB_EXPORTED void setNanos(uint64_t nanos);
  MARIADB_EXPORTED void setNegative(bool negative= true);

  MARIADB_EXPORTED bool isNull() const;
  MARIADB_EXPORTED bool isNegative() const;

  MARIADB_EXPORTED SQLString toString(std::size_t decimals= 9) const;

  MARIADB_EXPORTED bool before(const Time& other) const;
  MARIADB_EXPORTED bool after(const Time& other) const;
  MARIADB_EXPORTED bool equals(const Time& other) const;
  inline int32_t compareTo(const Time& other) const {
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
