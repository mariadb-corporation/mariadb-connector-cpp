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

#include "Timestamp.h"
#include "conncpp/Exception.hpp"


namespace sql
{
namespace mariadb
{
  Timestamp nullTs(0, 0, 0);
}

  std::array<uint32_t, 7> parseTimestamp(const char* begin, const char* end)
  {
    std::array<uint32_t, 7> timestampPart{0,0,0,0,0,0,0};
    const std::size_t nanosIdx= 6;
    const char* nanoBegin= nullptr;
    
    int32_t partIdx= 0;

    for (auto b= begin; b < end; ++b) {
      if (*b == '-' || *b == ' ' || *b == ':') {
        ++partIdx;
        // Last element is for nanos and if we got to them here - smth is wrong
        if (partIdx >= timestampPart.size() - 1) {
          throw SQLException(
            "Cannot parse data in timestamp string '"
            + SQLString(begin, end - begin)
            + "'");
        }
        continue;
      }
      if (*b == '.') {
        ++partIdx;
        if (partIdx == timestampPart.size()) {
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
          "Cannot parse data in timestamp string '"
          + SQLString(begin, end - begin)
          + "'");
      }
      timestampPart[partIdx]= timestampPart[partIdx] * 10 + (*b - '0');
    }
    // fix nanoseconds if we have less than 9 digits
    if (nanoBegin > 0) {
      for (uint32_t pos= 0; pos < 9 - (end - nanoBegin - 1); ++pos) {
        timestampPart[6]= timestampPart[6] * 10;
      }
    }
    return timestampPart;
  }


  TimestampImp::TimestampImp() {
    std::memset(&ts, 0, sizeof(mariadb::capi::MYSQL_TIME));
  }


  mariadb::TimestampStruct& TimestampImp::get(sql::Timestamp& ts) {
    return ts.internal->ts;
  }

  const mariadb::TimestampStruct& TimestampImp::get(const sql::Timestamp& timestamp) {
    return timestamp.internal->ts;
  }

  // right can't be null
  Timestamp& TimestampImp::assign(Timestamp& left, const mariadb::TimestampStruct* right) {
    std::memcpy(&left.internal->ts, right, sizeof(mariadb::TimestampStruct));
    left.internal->ts.time_type= mariadb::capi::MYSQL_TIMESTAMP_DATETIME;
    return left;
  }


  bool TimestampImp::before(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
    return (us.year < them.year || (us.year == them.year && us.month < them.month) ||
      (us.month == them.month && us.day < them.day) || (us.day == them.day && us.hour < them.hour) ||
      (us.hour == them.hour && us.minute < them.minute) || (us.minute == them.minute && us.second < them.second) ||
      (us.second == them.second && us.second_part < them.second_part));
  }


  bool TimestampImp::after(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
    return (us.year > them.year || (us.year == them.year && us.month > them.month) ||
      (us.month == them.month && us.day > them.day) || (us.day == them.day && us.hour > them.hour) ||
      (us.hour == them.hour && us.minute > them.minute) || (us.minute == them.minute && us.second > them.second) ||
      (us.second == them.second && us.second_part > them.second_part));
  }


  bool TimestampImp::equals(const mariadb::TimestampStruct& us, const mariadb::TimestampStruct& them) {
    return (us.year == them.year && us.month == them.month && us.day == them.day && us.hour == them.hour &&
      us.minute == them.minute && us.second == them.second && us.second_part == them.second_part);
  }


  SQLString TimestampImp::toString(mariadb::TimestampStruct& ts, std::size_t decimals) {
    SQLString result;
    auto& real= StringImp::get(result);
    // 32 would be enough for ts with nanos.
    real.reserve(19 + (ts.second_part ? decimals + 1 : 0) + (ts.neg ? 1 : 0));
    if (ts.neg) {
      real.append(1, '-');
    }
    real.append(std::to_string(ts.year)).append(1, '-');
    if (ts.month < 10) {
      real.append(1, '0');// .append(1, '0' + ts.month);
    }
    real.append(std::to_string(ts.month)).append(1, '-');
    if (ts.day < 10) {
      real.append(1, '0');// .append(1, '0' + ts.month);
    }
    real.append(std::to_string(ts.day)).append(1, ' ');
    if (ts.hour < 10) {
      real.append(1, '0');// .append(1, '0' + ts.hour);
    }
    real.append(std::to_string(ts.hour)).append(1, ':');
    if (ts.minute < 10) {
      real.append(1, '0');
    }
    real.append(std::to_string(ts.minute)).append(1, ':');
    if (ts.second < 10) {
      real.append(1, '0');
    }
    real.append(std::to_string(ts.second));
    if (ts.second_part) {
      real.append(1, '.');
      auto nanos= std::to_string(ts.second_part * 1000);
      for (auto i= nanos.length(); i < decimals; ++i) {
        real.append(1, '0');
      }
      real.append(nanos.begin(), nanos.length() > decimals ? nanos.begin() + decimals : nanos.end());
    }
    return result;
  }

// ------------------------- end of TimestampImp -> begin of Timestamp ---------------

  Timestamp::Timestamp() : internal(new TimestampImp()) {
  }

  Timestamp::~Timestamp() {
    delete internal;
  }

  Timestamp::Timestamp(const char* ts, std::size_t len) {
    auto parts= parseTimestamp(ts, ts + len);
    internal= new TimestampImp(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], parts[6]);
  }


  TimestampImp::TimestampImp(uint32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s, uint64_t n)
    : ts({y,mo, d, h, mi, s, static_cast<unsigned long>(n/1000), '\0',
                  mariadb::capi::MYSQL_TIMESTAMP_DATETIME }) {
    
  }

  Timestamp::Timestamp(uint32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s, uint64_t n)
    : internal(new TimestampImp(y, mo, d, h, mi, s, n)) {
  }

  Timestamp::Timestamp(const std::chrono::time_point<std::chrono::system_clock> ts) {
    auto astimet = std::chrono::system_clock::to_time_t(ts);
    tm* lt= std::localtime(&astimet);
    auto duration= ts.time_since_epoch();
    auto nanos= std::chrono::duration_cast<std::chrono::microseconds>(duration);
    internal= new TimestampImp(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec,
      nanos.count() % 1000000000);
  }
  Timestamp::Timestamp(const Timestamp& other) : Timestamp() {
    TimestampImp::assign(*this, &other.internal->ts);
  }

  Timestamp& Timestamp::operator=(const Timestamp& other) {
    return TimestampImp::assign(*this, &other.internal->ts);
  }

  uint32_t  Timestamp::getYear() const {
    return internal->ts.year;
  }

  uint32_t  Timestamp::getMonth() const {
    return internal->ts.month;
  }

  uint32_t  Timestamp::getDate() const {
    return internal->ts.day;
  }
  uint32_t  Timestamp::getHours() const {
    return internal->ts.hour;
  }

  uint32_t  Timestamp::getMinutes() const {
    return internal->ts.minute;
  }
  uint32_t  Timestamp::getSeconds() const {
    return internal->ts.second;
  }
  uint64_t  Timestamp::getNanos() const {
    return internal->ts.second_part*1000;
  }

  void Timestamp::setYear(uint32_t  y) {
    internal->ts.year = y;
  }

  void Timestamp::setMonth(uint32_t m) {
    internal->ts.month= m - 1;
  }

  void Timestamp::setDate(uint32_t d) {
    internal->ts.day= d;
  }

  void Timestamp::setHours(uint32_t h) {
    internal->ts.hour= h;
  }

  void Timestamp::setMinutes(uint32_t m) {
    internal->ts.minute= m;
  }

  void Timestamp::setSeconds(uint32_t s) {
    internal->ts.second= s;
  }

  void Timestamp::setNanos(uint64_t n) {
    internal->ts.second_part= static_cast<unsigned long>(n/1000);
  }

  bool Timestamp::isNull() const {
    return internal->ts.year == 0 && internal->ts.month == 0 && internal->ts.day == 0 &&
      internal->ts.hour == 0 && internal->ts.minute == 0 && internal->ts.second == 0;
  }

  SQLString Timestamp::toString(std::size_t decimals) const {

    return TimestampImp::toString(internal->ts, decimals);;
  }


  bool Timestamp::before(const Timestamp& other) const {
    auto& us= TimestampImp::get(*this);
    auto& them= TimestampImp::get(other);

    return TimestampImp::before(us, them);
  }


  bool Timestamp::after(const Timestamp& other) const {
    auto& us= TimestampImp::get(*this);
    auto& them= TimestampImp::get(other);

    return TimestampImp::after(us, them);
  }

  bool Timestamp::equals(const Timestamp& other) const {
    auto& us= TimestampImp::get(*this);
    auto& them= TimestampImp::get(other);

    return TimestampImp::equals(us, them);
  }
}
