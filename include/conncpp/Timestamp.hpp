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


#ifndef _CONCPPTIMESTAMP_HPP_
#define _CONCPPTIMESTAMP_HPP_

#include <chrono>
#include "buildconf.hpp"
#include "SQLString.hpp"

namespace sql
{
  class TimestampImp;

class Timestamp final {
  friend class TimestampImp;
  TimestampImp* internal;

public:
  MARIADB_EXPORTED Timestamp();
  MARIADB_EXPORTED Timestamp(const char* timestamp, std::size_t length);
  Timestamp(const SQLString& timestamp) : Timestamp(timestamp.c_str(), timestamp.length()) {
  }
  MARIADB_EXPORTED Timestamp(uint32_t y, uint32_t mo, uint32_t d, uint32_t h= 0, uint32_t mi= 0,
    uint32_t s= 0, uint64_t n= 0);

  MARIADB_EXPORTED Timestamp(const std::chrono::time_point<std::chrono::system_clock> ts);
  MARIADB_EXPORTED Timestamp(const Timestamp& other);
  MARIADB_EXPORTED Timestamp& operator=(const Timestamp& other);

  MARIADB_EXPORTED ~Timestamp();

  MARIADB_EXPORTED uint32_t getYear() const;
  MARIADB_EXPORTED uint32_t getMonth() const;
  MARIADB_EXPORTED uint32_t getDate() const;
  MARIADB_EXPORTED uint32_t getHours() const;
  MARIADB_EXPORTED uint32_t getMinutes() const;
  MARIADB_EXPORTED uint32_t getSeconds() const;
  MARIADB_EXPORTED uint64_t getNanos() const;

  MARIADB_EXPORTED void setYear(uint32_t year);
  MARIADB_EXPORTED void setMonth(uint32_t month);
  MARIADB_EXPORTED void setDate(uint32_t day);
  MARIADB_EXPORTED void setHours(uint32_t hours);
  MARIADB_EXPORTED void setMinutes(uint32_t minutes);
  MARIADB_EXPORTED void setSeconds(uint32_t seconds);
  MARIADB_EXPORTED void setNanos(uint64_t n);

  MARIADB_EXPORTED bool isNull() const;
  MARIADB_EXPORTED SQLString toString(std::size_t decimals= 9) const;

  MARIADB_EXPORTED bool before(const Timestamp& other) const;
  MARIADB_EXPORTED bool after(const Timestamp& other) const;
  MARIADB_EXPORTED bool equals(const Timestamp& other) const;
  inline int32_t compareTo(const Timestamp& other) const {
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
