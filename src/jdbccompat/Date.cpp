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

#include <array>
#include "conncpp/Exception.hpp"
#include "Date.h"
#include "Timestamp.h"

namespace sql
{
namespace mariadb
{
  const sql::Date nullDate;
}

std::array<uint32_t, 3> parseDate(const char* begin, const char* end)
{
  std::array<uint32_t, 3> datePart{ 0,0,0 };

  int32_t partIdx= 0;

  for (auto b= begin; b < end; ++b) {
    if (*b == '-') {
      ++partIdx;
      if (partIdx == datePart.size()) {
        throw SQLException(
          "Cannot parse data in date string '"
          + SQLString(begin, end - begin)
          + "'");
      }
      continue;
    }
    if (*b < '0' || *b > '9') {
      throw SQLException(
        "Cannot parse data in date string '"
        + SQLString(begin, end - begin)
        + "'");
    }
    datePart[partIdx]= datePart[partIdx] * 10 + (*b - '0');
  }
  return datePart;
}


DateImp::DateImp() {
  memset(&date, 0, sizeof(mariadb::capi::MYSQL_TIME));
}

DateImp::DateImp(uint32_t y, uint32_t mo, uint32_t d)
  : date({ y,mo, d, 0, 0, 0, 0, '\0', mariadb::capi::MYSQL_TIMESTAMP_DATE }) {

}

mariadb::capi::MYSQL_TIME& DateImp::get(sql::Date& ts) {
  return ts.internal->date;
}

bool DateImp::before(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
  return (us.year < them.year || (us.year == them.year && us.month < them.month) ||
    (us.month == them.month && us.day < them.day));
}

bool DateImp::after(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
  return (us.year > them.year || (us.year == them.year && us.month > them.month) ||
    (us.month == them.month && us.day > them.day));
}

bool DateImp::equals(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
  return (us.year == them.year && us.month == them.month && us.day == them.day);
}

SQLString DateImp::toString(mariadb::TimestampStruct& time) {
  SQLString result;
  auto& real= StringImp::get(result);
  // 32 would be enough for time with nanos.
  real.reserve(12);

  real.append(std::to_string(time.year)).append(1, '-');
  if (time.month < 10) {
    real.append(1, '0');
  }
  real.append(std::to_string(time.month)).append(1, '-');
  if (time.day < 10) {
    real.append(1, '0');
  }
  real.append(std::to_string(time.day));

  return result;
}

//-------------- end of DateImp -> begin of Date -----------------

const mariadb::capi::MYSQL_TIME& DateImp::get(const sql::Date& timestamp) {
  return timestamp.internal->date;
}

Date::Date() : internal(new DateImp()) {
}

Date::Date(const std::chrono::time_point<std::chrono::system_clock> ts) {
  auto astimet = std::chrono::system_clock::to_time_t(ts);
  tm* lt= std::localtime(&astimet);
  internal= new DateImp(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
}


Date::Date(Date&& other)
{
  internal= other.internal;
  other.internal= nullptr; // we don't want it to be destroyed.
}

Date::Date(const Date& other)
  :internal(new DateImp(*other.internal)) {
}

Date::~Date() {
  delete internal;
}

Date::Date(const char* ts, std::size_t len) {
  auto parts= parseDate(ts, ts + len);
  internal= new DateImp(parts[0], parts[1], parts[2]);
}

Date::Date(uint32_t y, uint32_t mo, uint32_t d)
  : internal(new DateImp(y, mo, d)) {
}


uint32_t  Date::getYear() const {
  return internal->date.year;
}

uint32_t  Date::getMonth() const {
  return internal->date.month;
}

uint32_t  Date::getDate() const {
  return internal->date.day;
}


void Date::setYear(uint32_t  y) {
  internal->date.year = y;
}

void Date::setMonth(uint32_t m) {
  internal->date.month= m - 1;
}

void Date::setDate(uint32_t d) {
  internal->date.day= d;
}

bool Date::isNull() const {
  return internal->date.year == 0 && internal->date.month == 0 && internal->date.day == 0;
}

SQLString Date::toString() const {
  return DateImp::toString(internal->date);
}


bool Date::before(const Date& other) const {
  auto& us= DateImp::get(*this);
  auto& them= DateImp::get(other);

  return DateImp::before(us, them);
}


bool Date::after(const Date& other) const {
  auto& us= DateImp::get(*this);
  auto& them= DateImp::get(other);

  return DateImp::after(us, them);
}

bool Date::equals(const Date& other) const {
  auto& us= DateImp::get(*this);
  auto& them= DateImp::get(other);

  return DateImp::equals(us, them);
}

}