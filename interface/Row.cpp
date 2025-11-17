/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

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

#include <cmath>
#include "Row.h"
#include "ColumnDefinition.h"

namespace mariadb
{
  //
  int64_t core_strtoll(const char* str, uint32_t len, const char **stopChar) {

    int64_t result=0, digit= 0;
    const char* end= str + len;

    while (str < end) {
      switch (*str) {
      case '0':
        digit= 0;
        break;
      case '1':
        digit= 1;
        break;
      case '2':
        digit= 2;
        break;
      case '3':
        digit= 3;
        break;
      case '4':
        digit= 4;
        break;
      case '5':
        digit= 5;
        break;
      case '6':
        digit= 6;
        break;
      case '7':
        digit= 7;
        break;
      case '8':
        digit= 8;
        break;
      case '9':
        digit= 9;
        break;
      default:
        if (stopChar) {
          *stopChar= str;
        }
        return result;
      }
      result= result * 10 + digit;
      ++str;
    }
    if (stopChar) {
      *stopChar= str;
    }
    return result;
  }

  int64_t safer_strtoll(const char* str, uint32_t len, const char **stopChar) {

    int64_t sign= 1;

    while (*str == ' ') {
      ++str;
      --len;
    }

    if (*str == '-') {
      sign= -1;
      ++str;
      --len;
    }
    else if (*str == '+') {
      ++str;
      --len;
    }

    return core_strtoll(str, len, stopChar)*sign;
  }


  uint64_t stoull(const SQLString& str, std::size_t* pos)
  {
    bool negative= false;
    std::string::const_iterator ci= str.begin();
    while (std::isblank(*ci) && ci < str.end()) ++ci;

    if (*str.c_str() == '-')
    {
      negative= true;
    }

    uint64_t result= std::stoull(str, pos);

    if (negative && result != 0)
    {
      throw std::out_of_range("String represents number beyond uint64_t range");
    }
    return result;
  }


  long double safer_strtod(const char* str, uint32_t len)
  {
    char *notConstStop= nullptr;
    const char *stop= nullptr, *end= str + len;
    int64_t sign= 1;

    return strtold(str, &notConstStop);
    while (*str == ' ') {
      ++str;
      --len;
    }

    if (*str == '-') {
      sign= -1;
      ++str;
      --len;
    }
    else if (*str == '+') {
      ++str;
      --len;
    }

    int64_t intPart= sign*core_strtoll(str, len, &stop), fractional= 0;
    long double result= static_cast<long double>(intPart);

    if (stop < end && *stop == '.') { // locale?
      str= stop + 1;
      long double digit;
      long double den= 1e-1 * sign;
      bool breakLoop= false;
      while (str < end) {
        switch (*str) {
        case '0':
          digit= 0.;
          break;
        case '1':
          digit= 1.;
          break;
        case '2':
          digit= 2.;
          break;
        case '3':
          digit= 3.;
          break;
        case '4':
          digit= 4.;
          break;
        case '5':
          digit= 5.;
          break;
        case '6':
          digit= 6.;
          break;
        case '7':
          digit= 7.;
          break;
        case '8':
          digit= 8.;
          break;
        case '9':
          digit= 9.;
          break;
        default:
          stop= str;
          breakLoop= true;
        }
        if (breakLoop) {
          break;
        }
        result+= den * digit;
        den/= 10.;
        ++str;
      }
    }
    if (stop < end && (*stop == 'e' || *stop == 'E')) {
      str= stop + 1;
      //reusing fractional for exponent
      fractional= 0;
      fractional= safer_strtoll(str, static_cast<uint32_t>(end - str), &stop);
      result*= std::pow(10.0, fractional);
    }
    return result;
  }


  uint64_t stoull(const char* str, std::size_t len, std::size_t* pos)
  {
    len= len == static_cast<std::size_t>(-1) ? std::strlen(str) : len;
    return mariadb::stoull(SQLString(str, len), pos);
  }

  Date nullDate("0000-00-00");
  const SQLString emptyStr("");

  /*int32_t Row::BIT_LAST_FIELD_NOT_NULL= 0b000000;
  int32_t Row::BIT_LAST_FIELD_NULL= 0b000001;
  int32_t Row::BIT_LAST_ZERO_DATE= 0b000010;
  int32_t Row::TINYINT1_IS_BIT= 1;
  int32_t Row::YEAR_IS_DATE_TYPE= 2;*/

  Timestamp Row::nullTs("0000-00-00 00:00:00");
  Time Row::nullTime("00:00:00");

  bool isDate(SQLString::const_iterator it)
  {
    if (*it == '-') {
      ++it;
    }

    if (std::isdigit(*it) && std::isdigit(*++it) && std::isdigit(*++it) && std::isdigit(*++it) &&
      *++it == '-' &&
      std::isdigit(*it) && std::isdigit(*++it) && std::isdigit(*++it) && std::isdigit(*++it) &&
      *++it == '-' &&
      std::isdigit(*it) && std::isdigit(*++it) && std::isdigit(*++it) && std::isdigit(*it)) {
      return true;
    }
    return false;
  }

  bool isDate(const SQLString& str)
  {
    if (str.length() < 10) {
      return false;
    }
    return isDate(str.cbegin());
  }


  bool isTime(SQLString::const_iterator it, bool canBeNegative= true)
  {
    if (canBeNegative && *it == '-') {
      ++it;
    }

    if (std::isdigit(*it) && std::isdigit(*++it) &&
      *++it == ':' &&
      std::isdigit(*it) && std::isdigit(*++it) &&
      *++it == ':' &&
      std::isdigit(*it) && std::isdigit(*it)) {
      return true;
    }
    return false;
  }


  bool isTime(const SQLString& str)
  {
    if (str.length() < 8) {
      return false;
    }
    return isTime(str.cbegin());
  }


  bool parseTime(const SQLString& str, std::vector<SQLString>& time)
  {
    constexpr std::size_t minTimeLength= 5; /*N:N:N*/
    std::string::const_iterator it= str.cbegin(), colon= it + str.find(':'), colon2= str.cbegin();

    if (str.length() < minTimeLength || colon >= str.cend()) {
      return false;
    }
    colon2+= str.find(':', colon - str.cbegin() + 1);
    if (colon2 >= str.cend() || colon2 - colon > 3) {
      return false;
    }

    // Reserving first element for complete time string
    time.push_back(emptyStr);

    std::size_t offset= 0;
    if (*it == '-') {
      time.push_back("-");
      offset= 1;
      ++it;
    }
    else {
      time.push_back(emptyStr);
    }

    while (it < colon && std::isdigit(*it)) {
      ++it;
    }
    if (it < colon) {
      return false;
    }

    if (std::isdigit(*++it) && (std::isdigit(*++it) || it == colon2))
    {
      time.emplace_back(str.cbegin() + offset, colon);
      time.emplace_back(colon + 1, colon2);
      it= colon2;
      while (++it < str.cend() && std::isdigit(*it));

      if (it - colon2 > 3) {
        return false;
      }
      if (it - colon2 == 1) {
        time.emplace_back("");
      }
      else {
        time.emplace_back(colon2 + 1, it);
      }
      if (it < str.cend() && *it == '.') {
        auto secondPartsBegin= ++it;
        while (it < str.cend() && std::isdigit(*it)) {
          ++it;
        }
        if (it > secondPartsBegin) {
          time.push_back(SQLString(secondPartsBegin, it));
        }
        else {
          time.push_back(emptyStr);
        }
      }
      else {
        time.push_back(emptyStr);
      }
      time[0].assign(str.begin(), it);
      return true;
    }
    return false;
  }


  Row::Row()
    : buf(nullptr)
    , fieldBuf()
    , length(0)
    , lastValueNull(0)
    , index(0)
    , pos(0)
  {
  }


  void Row::resetRow(std::vector<mariadb::bytes_view>& _buf)
  {
    buf= &_buf;
  }

  uint32_t Row::getLengthMaxFieldSize()
  {
    return length;
  }


  uint32_t Row::getMaxFieldSize()
  {
    return 0;
  }


  bool Row::lastValueWasNull()
  {
    return (lastValueNull & BIT_LAST_FIELD_NULL) != 0;
  }


  SQLString Row::zeroFillingIfNeeded(const SQLString& value, const ColumnDefinition* columnInformation)
  {
    if (columnInformation->isZeroFill()) {
      SQLString zeroAppendStr;
      int64_t zeroToAdd= columnInformation->getDisplaySize() - value.size();

      while ((zeroToAdd--) > 0) {
        zeroAppendStr.append("0");
      }
      return zeroAppendStr.append(value);
    }
    return value;
  }

  /* Following 3 methods are used only for binary protocol, and work only for it. Thus makes sense to move them to that class. At some point */
  int32_t Row::getInternalTinyInt(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    int32_t value= fieldBuf[0];//buf[pos];
    if (!columnInfo->isSigned()) {
      value= (fieldBuf[0]/*buf[pos]*/ & 0xff);
    }
    return value;
  }

  int64_t Row::parseBit()
  {
    if (length == 1) {
      return fieldBuf[0]; //(*buf)[pos][0];
    }
    int64_t val= 0;
    uint32_t ind= 0;
    do {
      val+= (static_cast<int64_t>(fieldBuf[ind] & 0xff)) << (8 * (length - ind - 1));
      ++ind;
           //(static_cast<int64_t>((*buf)[pos][ind] & 0xff))<<(8 *(length - ++ind));
    } while (ind < length);
    return val;
  }

  int32_t Row::getInternalSmallInt(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    int value= (fieldBuf[0] & 0xff) + ((fieldBuf[1] & 0xff) << 8);
    if (!columnInfo->isSigned()) {
      return value & 0xffff;
    }
    // short cast here is important : -1 will be received as -1, -1 -> 65535
    return static_cast<int16_t>(value);
  }

  int64_t Row::getInternalMediumInt(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    int64_t value =
      ((fieldBuf[0] & 0xff)
        + ((fieldBuf[1] & 0xff) << 8)
        + ((fieldBuf[2] & 0xff) << 16)
        + ((fieldBuf[3] & 0xff) << 24));
    if (!columnInfo->isSigned()) {
      value= value & 0xffffffff;
    }
    return /*static_cast<int32_t>*/(value);
  }

  /* We interprete string as `false` if it equals exactly "0" or "false". Otherwise - true
  */
  bool Row::convertStringToBoolean(const char* str, std::size_t len)
  {
    if (len > 0) {
      // String may be not null-terminated or binary
      if (str[0] == '0' && (len == 1 || str[1] == '\0')) {
        return false;
      }

      if (len == 5 || (len > 5 && str[5] == '\0')) {
        SQLString rawVal(str, 5);
        return (toLowerCase(rawVal).compare("false") != 0);
      }
    }
    return true;
  }


  void Row::rangeCheck(const char* /*className*/, int64_t minValue, int64_t maxValue, int64_t value, const ColumnDefinition* columnInfo)
  {
    if ((value < 0 && !columnInfo->isSigned()) || value < minValue || value > maxValue) {
      throw MYSQL_DATA_TRUNCATED;
    }
  }


  int32_t Row::extractNanos(const SQLString& timestring)
  {
    size_t index= timestring.find_first_of('.');
    if (index == std::string::npos) {
      return 0;
    }
    int32_t nanos= 0;
    for (size_t i= index +1; i <index +10; i++) {
      int32_t digit;
      if (i >= timestring.size()) {
        digit= 0;
      }
      else {
        char value= timestring.at(i);
        if (value < '0'||value > '9') {
          throw 1;
        }
        digit= value -'0';
      }
      nanos= nanos *10 +digit;
    }
    return nanos;
  }

  /**
    * Reports whether the last column read had a value of Null. Note that you must first call one of
    * the getter methods on a column to try to read its value and then call the method wasNull to see
    * if the value read was Null.
    *
    * @return true true if the last column value read was null and false otherwise
    */
  bool Row::wasNull()
  {
    return (lastValueNull & BIT_LAST_FIELD_NULL)!=0 || (lastValueNull & BIT_LAST_ZERO_DATE)!=0;
  }

} // namespace mariadb
