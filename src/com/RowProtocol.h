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


#ifndef _ROWPROTOCOL_H_
#define _ROWPROTOCOL_H_

#include <regex>
#include <iostream>

#include "Consts.h"

namespace sql
{
namespace mariadb
{

extern Date nullDate;

class DateTimeFormatter;
class Calendar;
class TimeZone;

struct memBuf : std::streambuf
{
  memBuf(char* begin, char* end)
  {
    this->setg(begin, begin, end);
  }

  pos_type seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode which = std::ios_base::in) override
  {
    if (direction == std::ios_base::cur) {
      gbump(static_cast<int32_t>(offset));
    }
    else if (direction == std::ios_base::end) {
      setg(eback(), egptr() + offset, egptr());
    }
    else if (direction == std::ios_base::beg) {
      setg(eback(), eback() + offset, egptr());
    }
    return gptr() - eback();
  }

  pos_type seekpos(pos_type position, std::ios_base::openmode which) override
  {
    return seekoff(position - pos_type(off_type(0)), std::ios_base::beg, which);
  }
};


class RowProtocol  {

public:
  static int32_t BIT_LAST_FIELD_NOT_NULL ; /*0b000000*/
  static int32_t BIT_LAST_FIELD_NULL ; /*0b000001*/
  static int32_t BIT_LAST_ZERO_DATE ; /*0b000010*/
  static int32_t TINYINT1_IS_BIT ; /*1*/
  static int32_t YEAR_IS_DATE_TYPE ; /*2*/
  static const DateTimeFormatter* TEXT_LOCAL_DATE_TIME;
  static const DateTimeFormatter* TEXT_OFFSET_DATE_TIME;
  static const DateTimeFormatter* TEXT_ZONED_DATE_TIME;
  static std::regex isIntegerRegex ; /*Pattern.compile("^-?\\d+\\.[0-9]+$")*/
  static std::regex dateRegex;
  static std::regex timeRegex;
  static std::regex timestampRegex;
  static long double stringToDouble(const char* str, uint32_t len);

protected:
  static int32_t NULL_LENGTH_ ; /*-1*/
  uint32_t maxFieldSize;
  const Shared::Options options;

public:
  int32_t lastValueNull;
  std::vector<sql::bytes>* buf;
  sql::bytes fieldBuf; // I actually don't remember why is it a ref
  int32_t pos;
  uint32_t length;

protected:
  int32_t index;

public:
  RowProtocol(uint32_t maxFieldSize, Shared::Options options);
  virtual ~RowProtocol() {}

  void resetRow(std::vector<sql::bytes>& buf);
  virtual void setPosition(int32_t position)=0;
  uint32_t getLengthMaxFieldSize();
  uint32_t getMaxFieldSize();

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  SQLString getInternalString(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr)=0;
  Object getInternalObject(ColumnDefinition* columnInfo,TimeZone timeZone)=0;
  ZonedDateTime getInternalZonedDateTime(ColumnDefinition* columnInfo,Class clazz,TimeZone timeZone)=0;
  OffsetTime getInternalOffsetTime(ColumnDefinition* columnInfo,TimeZone timeZone)=0;
  LocalTime getInternalLocalTime(ColumnDefinition* columnInfo,TimeZone timeZone)=0;
  LocalDate getInternalLocalDate(ColumnDefinition* columnInfo,TimeZone timeZone)=0;
  BigInteger getInternalBigInteger(ColumnDefinition* columnInfo)=0;
  void rangeCheck(const sql::SQLString& className, int64_t minValue, int64_t maxValue, int64_t value, ColumnDefinition* columnInfo);
#endif
  virtual int32_t fetchNext()=0;
  virtual void installCursorAtPosition(int32_t rowPtr)=0;

  virtual Date getInternalDate(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr)=0;
  virtual std::unique_ptr<Time>  getInternalTime(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr)=0;
  virtual std::unique_ptr<Timestamp> getInternalTimestamp(ColumnDefinition* columnInfo, Calendar* userCalendar=nullptr, TimeZone* timeZone=nullptr)=0;
  virtual std::unique_ptr<SQLString> getInternalString(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr)=0;
  virtual int32_t getInternalInt(ColumnDefinition* columnInfo)=0;
  virtual int64_t getInternalLong(ColumnDefinition* columnInfo)=0;
  virtual uint64_t getInternalULong(ColumnDefinition* columnInfo)=0;
  virtual float getInternalFloat(ColumnDefinition* columnInfo)=0;
  virtual long double getInternalDouble(ColumnDefinition* columnInfo)=0;
  virtual std::unique_ptr<BigDecimal> getInternalBigDecimal(ColumnDefinition* columnInfo)=0;

  virtual bool getInternalBoolean(ColumnDefinition* columnInfo)=0;
  virtual int8_t getInternalByte(ColumnDefinition* columnInfo)=0;
  virtual int16_t getInternalShort(ColumnDefinition* columnInfo)=0;
  virtual SQLString getInternalTimeString(ColumnDefinition* columnInfo)=0;


  virtual bool isBinaryEncoded()=0;
  bool lastValueWasNull();

protected:
  SQLString zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation);
  int32_t getInternalTinyInt(ColumnDefinition* columnInfo);
  int64_t parseBit();
  int32_t getInternalSmallInt(ColumnDefinition* columnInfo);
  int64_t getInternalMediumInt(ColumnDefinition* columnInfo);

  bool convertStringToBoolean(const char* str, std::size_t len);

public:
  void rangeCheck(const sql::SQLString& className,int64_t minValue, int64_t maxValue, int64_t value, ColumnDefinition* columnInfo);

protected:
  int32_t extractNanos(const SQLString& timestring);

public:
  bool wasNull();
  };
}
}
#endif
