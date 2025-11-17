/************************************************************************************
   Copyright (C) 2022,2023 MariaDB Corporation AB

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


#ifndef _ROW_H_
#define _ROW_H_

#include <sstream>
#include <vector>
#include <memory>
#include <cstdint>

#include "mysql.h"

#include "SQLString.h"
#include "CArray.h"


namespace mariadb
{
extern Date nullDate;
extern const SQLString emptyStr;
class ColumnDefinition;

int64_t safer_strtoll(const char* str, uint32_t len, const char **stopChar= nullptr);
uint64_t stoull(const SQLString& str, std::size_t* pos= nullptr);
uint64_t stoull(const char* str, std::size_t len= -1, std::size_t* pos= nullptr);
long double safer_strtod(const char* str, uint32_t len);

struct memBuf : public std::streambuf
{
  memBuf(char* begin, char* end)
  {
    this->setg(begin, begin, end);
  }

  pos_type seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode which= std::ios_base::in) override
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

bool isDate(const SQLString& str);
bool isTime(const SQLString& str);
bool parseTime(const SQLString& str, std::vector<SQLString>& time);

class Row  {

public:
  static const int32_t BIT_LAST_FIELD_NOT_NULL= 0b000000;
  static const int32_t BIT_LAST_FIELD_NULL= 0b000001;
  static const int32_t BIT_LAST_ZERO_DATE= 0b000010;
  static const int32_t TINYINT1_IS_BIT= 1;
  static const int32_t YEAR_IS_DATE_TYPE= 2;

protected:
  static const int32_t NULL_LENGTH_= -1;
  // Should be const
  static Timestamp nullTs;//= "0000-00-00 00:00:00"
  static Time nullTime; //("00:00:00")

public:
  int32_t lastValueNull;
  std::vector<bytes_view>* buf;
  bytes_view fieldBuf;
  int32_t pos;
  uint32_t length;

protected:
  int32_t index;

public:
  Row();
  virtual ~Row() {}

  void resetRow(std::vector<bytes_view>& buf);
  virtual void setPosition(int32_t position)=0;
  uint32_t getLengthMaxFieldSize();
  uint32_t getMaxFieldSize();

  virtual int32_t fetchNext()=0;
  virtual void installCursorAtPosition(int32_t rowPtr)=0;

  virtual Date getInternalDate(const ColumnDefinition* columnInfo)=0;
  virtual Time getInternalTime(const ColumnDefinition* columnInfo, MYSQL_TIME* dest= nullptr)=0;
  virtual Timestamp getInternalTimestamp(const ColumnDefinition* columnInfo)=0;
  virtual SQLString getInternalString(const ColumnDefinition* columnInfo)=0;
  virtual int32_t getInternalInt(const ColumnDefinition* columnInfo)=0;
  virtual int64_t getInternalLong(const ColumnDefinition* columnInfo)=0;
  virtual uint64_t getInternalULong(const ColumnDefinition* columnInfo)=0;
  virtual float getInternalFloat(const ColumnDefinition* columnInfo)=0;
  virtual long double getInternalDouble(const ColumnDefinition* columnInfo)=0;
  virtual BigDecimal getInternalBigDecimal(const ColumnDefinition* columnInfo)=0;

  virtual bool getInternalBoolean(const ColumnDefinition* columnInfo)=0;
  virtual int8_t getInternalByte(const ColumnDefinition* columnInfo)=0;
  virtual int16_t getInternalShort(const ColumnDefinition* columnInfo)=0;
  virtual SQLString getInternalTimeString(const ColumnDefinition* columnInfo)=0;

  virtual bool isBinaryEncoded()=0;
  virtual void cacheCurrentRow(std::vector<bytes_view>& rowData, std::size_t columnCount)=0;
  bool lastValueWasNull();

protected:
  SQLString zeroFillingIfNeeded(const SQLString& value, const ColumnDefinition* columnInformation);
  int32_t getInternalTinyInt(const ColumnDefinition* columnInfo);
  int64_t parseBit();
  int32_t getInternalSmallInt(const ColumnDefinition* columnInfo);
  int64_t getInternalMediumInt(const ColumnDefinition* columnInfo);

  bool convertStringToBoolean(const char* str, std::size_t len);

public:
  void rangeCheck(const char* className,int64_t minValue, int64_t maxValue, int64_t value, const ColumnDefinition* columnInfo);

protected:
  int32_t extractNanos(const SQLString& timestring);

public:
  bool wasNull();
  };

namespace Unique
{
  typedef std::unique_ptr<mariadb::Row> Row;
}

} // namespace mariadb
#endif
