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
#include "Time.h"
#include "conncpp/Exception.hpp"
#include "Timestamp.h"

namespace sql
{
namespace mariadb
{
  Time nullTime;
}

std::array<uint32_t, 5> parseTime(const char* begin, const char* end)
{
  std::array<uint32_t, 5> timePart{ 0,0,0,0,0 };
  const std::size_t nanosIdx= 6;
  const char* nanoBegin= nullptr;
  auto b= begin;

  int32_t partIdx= 1;

  if (*b == '-') {
    // negative
    timePart[0]= 1;
    ++b;
  }

  for (; b < end; ++b) {
    if (*b == ':') {
      ++partIdx;
      if (partIdx >= timePart.size() - 1) {
        throw SQLException(
          "Cannot parse data in timestamp string '"
          + SQLString(begin, end - begin)
          + "'");
      }
      continue;
    }
    if (*b == '.') {
      ++partIdx;
      if (partIdx == timePart.size()) {
        throw SQLException(
          "Cannot parse data in timestamp string '"
          + SQLString(begin, end - begin)
          + "'");
      }
      nanoBegin= b;
      continue;
    }
    if (*b < '0' || *b > '9') {
      throw SQLException(
        "cannot parse data in timestamp string '"
        + SQLString(begin, end - begin)
        + "'");
    }
    timePart[partIdx]= timePart[partIdx] * 10 + (*b - '0');
  }
  // fix nanoseconds if we have less than 9 digits
  if (nanoBegin > 0) {
    for (uint32_t pos= 0; pos < 9 - (end - nanoBegin - 1); ++pos) {
      timePart[4]= timePart[4] * 10;
    }
  }
  return timePart;
}


TimeImp::TimeImp() {
  memset(&time, 0, sizeof(mariadb::TimestampStruct));
}


mariadb::TimestampStruct& TimeImp::get(sql::Time& t) {
  return t.internal->time;
}

const mariadb::TimestampStruct& TimeImp::get(const sql::Time& t) {
  return t.internal->time;
}


TimeImp::TimeImp(bool negative, uint32_t h, uint32_t mi, uint32_t s, uint64_t n)
  : time({ 0, 0, 0, h, mi, s, static_cast<unsigned long>(n / 1000), negative ? '\1' : '\0',
                mariadb::capi::MYSQL_TIMESTAMP_TIME }) {
}


bool TimeImp::before(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
  return (us.hour < them.hour ||
    (us.hour == them.hour && us.minute < them.minute) || (us.minute == them.minute && us.second < them.second) ||
    (us.second == them.second && us.second_part < them.second_part));
}


bool TimeImp::after(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
  return (us.hour > them.hour ||
    (us.hour == them.hour && us.minute > them.minute) || (us.minute == them.minute && us.second > them.second) ||
    (us.second == them.second && us.second_part > them.second_part));
}


bool TimeImp::equals(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
  return (us.hour == them.hour && us.minute == them.minute && us.second == them.second &&
    us.second_part == them.second_part);
}


SQLString TimeImp::toString(mariadb::TimestampStruct& time, std::size_t decimals) {
  SQLString result;
  auto& real= StringImp::get(result);
  // Reserving place for "-" always.
  real.reserve(9 + (time.second_part ? 10 : 0) + (time.hour > 99 ? 1 : 0));
  if (time.neg) {
    real.append(1, '-');
  }
  if (time.hour < 10) {
    real.append(1, '0');// .append(1, '0' + time.hour);
  }
  real.append(std::to_string(time.hour)).append(1, ':');
  if (time.minute < 10) {
    real.append(1, '0');
  }
  real.append(std::to_string(time.minute)).append(1, ':');
  if (time.second < 10) {
    real.append(1, '0');
  }
  real.append(std::to_string(time.second));
  if (time.second_part) {
    real.append(1, '.');
    auto nanos= std::to_string(time.second_part*1000);
    for (auto i= nanos.length(); i < decimals; ++i) {
      real.append(1, '0');
    }
    real.append(nanos.begin(), nanos.length() > decimals ? nanos.begin() + decimals : nanos.end());
  }
  return result;
}

// ===================== end of TimeImp -> begin of Time =====================

// This is sort of NULL time construector
Time::Time() : internal(new TimeImp()) {
  // Maybe it's better drop nullness idea alltogether but for know using month field as flag of nullness
  // Could also not allocating internal and use it as nullness but I kinda like this better.
  internal->time.month= static_cast<unsigned int>(-1);
}


Time::~Time() {
  delete internal;
}

Time::Time(const char* t, std::size_t len) {
  auto parts= parseTime(t, t + len);
  internal= new TimeImp(parts[0], parts[1], parts[2], parts[3], parts[4]);
}


Time::Time(uint32_t h, uint32_t mi, uint32_t s, uint64_t n, bool negative)
  : internal(new TimeImp(negative, h, mi, s, n)) {
}


Time::Time(const std::chrono::time_point<std::chrono::system_clock> time) {

  auto astimet= std::chrono::system_clock::to_time_t(time);
  tm* lt= std::localtime(&astimet);
  auto duration= time.time_since_epoch();
  auto nanos= std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
  internal= new TimeImp(false, lt->tm_hour, lt->tm_min, lt->tm_sec, nanos.count() % 1000000000);
}

uint32_t  Time::getHours() const {
  return internal->time.hour;
}

uint32_t  Time::getMinutes() const {
  return internal->time.minute;
}

uint32_t  Time::getSeconds() const {
  return internal->time.second;
}

uint64_t  Time::getNanos() const {
  return internal->time.second_part * 1000;
}

void Time::setHours(uint32_t h) {
  internal->time.month= 0;
  internal->time.hour= h;
}

void Time::setMinutes(uint32_t m) {
  internal->time.month= 0;
  internal->time.minute= m;
}

void Time::setSeconds(uint32_t s) {
  internal->time.month= 0;
  internal->time.second= s;
}

void Time::setNanos(uint64_t n) {
  internal->time.month= 0;
  internal->time.second_part= static_cast<unsigned long>(n / 1000);
}

void Time::setNegative(bool negative) {
  internal->time.month= 0;
  internal->time.neg= negative ? '\1' : '\0';
}

bool Time::isNull() const {
  return internal->time.month == static_cast<unsigned int>(-1);
}

bool Time::isNegative() const {
  return internal->time.neg;
}

SQLString Time::toString(std::size_t decimals) const {
  return TimeImp::toString(internal->time, decimals);
}

bool Time::before(const Time& other) const {
  auto& us= TimeImp::get(*this);
  auto& them= TimeImp::get(other);

  return TimeImp::before(us, them);
}


bool Time::after(const Time& other) const {
  auto& us= TimeImp::get(*this);
  auto& them= TimeImp::get(other);

  return TimeImp::after(us, them);
}

bool Time::equals(const Time& other) const {
  auto& us= TimeImp::get(*this);
  auto& them= TimeImp::get(other);

  return TimeImp::equals(us, them);
}
}

