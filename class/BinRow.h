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


#ifndef _BINROW_H_
#define _BINROW_H_

#include "Row.h"
#include "mysql.h"

namespace mariadb
{
class ColumnDefinition;

class BinRow : public Row {

  const std::vector<ColumnDefinition>& columnInformation;
  int32_t columnInformationLength;
  MYSQL_STMT* stmt;
  std::vector<MYSQL_BIND> bind;

  SQLString convertToString(const char* asChar, const ColumnDefinition* columnInfo);

public:

  BinRow(
    const std::vector<ColumnDefinition>& columnInformation,
    int32_t columnInformationLength,
    MYSQL_STMT* stmt);

  virtual ~BinRow();

  void setPosition(int32_t newIndex);

  int32_t fetchNext();
  void installCursorAtPosition(int32_t rowPtr);

  SQLString getInternalString(const ColumnDefinition* columnInfo);
  Date getInternalDate(const ColumnDefinition* columnInfo);
  Time getInternalTime(const ColumnDefinition* columnInfo, MYSQL_TIME* dest= nullptr);
  Timestamp getInternalTimestamp(const ColumnDefinition* columnInfo);

  int32_t getInternalInt(const ColumnDefinition* columnInfo);
  int64_t getInternalLong(const ColumnDefinition* columnInfo);
  uint64_t getInternalULong(const ColumnDefinition* columnInfo);
  float getInternalFloat(const ColumnDefinition* columnInfo);
  long double getInternalDouble(const ColumnDefinition* columnInfo);
  BigDecimal getInternalBigDecimal(const ColumnDefinition* columnInfo);

  bool getInternalBoolean(const ColumnDefinition* columnInfo);
  int8_t getInternalByte(const ColumnDefinition* columnInfo);
  int16_t getInternalShort(const ColumnDefinition* columnInfo);
  SQLString getInternalTimeString(const ColumnDefinition* columnInfo);

  bool isBinaryEncoded();
  void cacheCurrentRow(std::vector<bytes_view>& rowDataCache, std::size_t columnCount);
  MYSQL_BIND* getDefaultBind() { return bind.data(); }
  };

}
#endif
