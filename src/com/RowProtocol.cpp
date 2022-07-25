/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB

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


#include "RowProtocol.h"

#include "ColumnDefinition.h"

namespace sql
{
namespace mariadb
{
  Date nullDate("0000-00-00");

  int32_t RowProtocol::BIT_LAST_FIELD_NOT_NULL= 0b000000;
  int32_t RowProtocol::BIT_LAST_FIELD_NULL= 0b000001;
  int32_t RowProtocol::BIT_LAST_ZERO_DATE= 0b000010;
  int32_t RowProtocol::TINYINT1_IS_BIT= 1;
  int32_t RowProtocol::YEAR_IS_DATE_TYPE= 2;

  std::regex RowProtocol::isIntegerRegex("^-?\\d+\\.[0-9]+$", std::regex_constants::ECMAScript);
  std::regex RowProtocol::dateRegex("^-?\\d{4}-\\d{2}-\\d{2}", std::regex_constants::ECMAScript);
  std::regex RowProtocol::timeRegex("^(-?)(\\d{2}):(\\d{2}):(\\d{2})(\\.\\d+)?", std::regex_constants::ECMAScript);
  std::regex RowProtocol::timestampRegex("^-?\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.?", std::regex_constants::ECMAScript);

  int32_t RowProtocol::NULL_LENGTH_= -1;

#ifdef WE_HAVE_JAVA_TYPES_IMPLEMENTED
  const RowProtocol::TEXT_LOCAL_DATE_TIME= new DateTimeFormatterBuilder()
    .parseCaseInsensitive()
    .append(DateTimeFormatter.ISO_LOCAL_DATE)
    .appendLiteral(' ')
    .append(DateTimeFormatter.ISO_LOCAL_TIME)
    .toFormatter();
  const RowProtocol::DateTimeFormatter TEXT_OFFSET_DATE_TIME= new DateTimeFormatterBuilder()
    .parseCaseInsensitive()
    .append(TEXT_LOCAL_DATE_TIME)
    .appendOffsetId()
    .toFormatter();
  const RowProtocol::DateTimeFormatter TEXT_ZONED_DATE_TIME= new DateTimeFormatterBuilder()
    .append(TEXT_OFFSET_DATE_TIME)
    .optionalStart()
    .appendLiteral('[')
    .parseCaseSensitive()
    .appendZoneRegionId()
    .appendLiteral(']')
    .toFormatter();
#endif


  long double RowProtocol::stringToDouble(const char* str, uint32_t len)
  {
    std::string doubleAsString(str, len);
    std::istringstream convStream(doubleAsString);
    std::locale C("C");
    long double result;
    convStream.imbue(C);
    convStream >> result;

    return result;
  }

  RowProtocol::RowProtocol(uint32_t _maxFieldSize, Shared::Options options)
    : maxFieldSize(_maxFieldSize)
    , options(options)
    , buf(nullptr)
    , fieldBuf()
    , length(0)
    , lastValueNull(0)
    , index(0)
    , pos(0)
  {
  }


  void RowProtocol::resetRow(std::vector<sql::bytes>& _buf)
  {
    buf= &_buf;
  }

  uint32_t RowProtocol::getLengthMaxFieldSize()
  {
    return maxFieldSize != 0 && maxFieldSize < length ? maxFieldSize : length;
  }


  uint32_t RowProtocol::getMaxFieldSize()
  {
    return maxFieldSize;
  }


  bool RowProtocol::lastValueWasNull()
  {
    return (lastValueNull & BIT_LAST_FIELD_NULL) != 0;
  }


  SQLString RowProtocol::zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation)
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
  int32_t RowProtocol::getInternalTinyInt(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    int32_t value = fieldBuf[0];//buf[pos];
    if (!columnInfo->isSigned()) {
      value = (fieldBuf[0]/*buf[pos]*/ & 0xff);
    }
    return value;
  }

  int64_t RowProtocol::parseBit()
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

  int32_t RowProtocol::getInternalSmallInt(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    int value = (fieldBuf[0] & 0xff) + ((fieldBuf[1] & 0xff) << 8);
    if (!columnInfo->isSigned()) {
      return value & 0xffff;
    }
    // short cast here is important : -1 will be received as -1, -1 -> 65535
    return static_cast<int16_t>(value);
  }

  int64_t RowProtocol::getInternalMediumInt(ColumnDefinition* columnInfo)
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
      value = value & 0xffffffff;
    }
    return /*static_cast<int32_t>*/(value);
  }

  /* We interprete string as `false` if it equals exactly "0" or "false". Otherwise - true
  */
  bool RowProtocol::convertStringToBoolean(const char* str, std::size_t len)
  {
    if (len > 0) {
      // String may be not null-terminated or binary
      if (str[0] == '0' && (len == 1 || str[1] == '\0')) {
        return false;
      }

      if (len == 5 || (len > 5 && str[5] == '\0')) {
        SQLString rawVal(str, 5);
        return (rawVal.toLowerCase().compare("false") != 0);
      }
    }
    return true;
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  void RowProtocol::rangeCheck(const sql::SQLString& className, int64_t minValue, int64_t maxValue, BigDecimal value, ColumnDefinition* columnInfo)
  {
    if (value.compareTo(BigDecimal.valueOf(minValue))<0
      ||value.compareTo(BigDecimal.valueOf(maxValue))>0) {
      throw SQLException(
        "Out of range value for column '"
        +columnInfo->getName()
        +"' : value "
        +value
        +" is not in "
        +className
        +" range",
        "22003",
        1264);
    }
  }
#endif


  void RowProtocol::rangeCheck(const sql::SQLString& className, int64_t minValue, int64_t maxValue, int64_t value, ColumnDefinition* columnInfo)
  {
    if ((value < 0 && !columnInfo->isSigned()) || value < minValue || value > maxValue) {
      throw SQLException(
        "Out of range value for column '"
        + columnInfo->getName()
        + "' : value "
        + std::to_string(value)
        + " is not in "
        + className
        + " range",
        "22003",
        1264);
    }
  }

  int32_t RowProtocol::extractNanos(const SQLString& timestring)
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
        if (value <'0'||value >'9') {
          throw SQLException(
            "cannot parse sub-second part in timestamp string '"+timestring +"'");
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
  bool RowProtocol::wasNull()
  {
    return (lastValueNull & BIT_LAST_FIELD_NULL)!=0 || (lastValueNull & BIT_LAST_ZERO_DATE)!=0;
  }
}
}
