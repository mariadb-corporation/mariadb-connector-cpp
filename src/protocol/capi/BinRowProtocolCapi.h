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


#ifndef _BINROWPROTOCOLCAPI_H_
#define _BINROWPROTOCOLCAPI_H_

#include "Consts.h"

#include "com/RowProtocol.h"

namespace sql
{
namespace mariadb
{
class ColumnDefinition;

namespace capi
{
#include "mysql.h"

class BinRowProtocolCapi : public RowProtocol {

  const std::vector<Shared::ColumnDefinition>& columnInformation;
  int32_t columnInformationLength;
  MYSQL_STMT* stmt;
  std::vector<MYSQL_BIND> bind;

  SQLString * convertToString(const char * asChar, ColumnDefinition * columnInfo);
public:

  BinRowProtocolCapi(
    std::vector<Shared::ColumnDefinition>& columnInformation,
    int32_t columnInformationLength,
    uint32_t maxFieldSize,
    Shared::Options options,
    MYSQL_STMT* stmt);

  virtual ~BinRowProtocolCapi();

  void setPosition(int32_t newIndex);

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  sql::Object* getInternalObject(ColumnDefinition* columnInfo,TimeZone* timeZone);
  ZonedDateTime getInternalZonedDateTime( ColumnDefinition* columnInfo,Class clazz,TimeZone* timeZone);
  OffsetTime getInternalOffsetTime(ColumnDefinition* columnInfo,TimeZone* timeZone);
  LocalTime getInternalLocalTime(ColumnDefinition* columnInfo,TimeZone* timeZone);
  LocalDate getInternalLocalDate(ColumnDefinition* columnInfo,TimeZone* timeZone);
  BigInteger getInternalBigInteger(ColumnDefinition* columnInfo);
#endif
  int32_t fetchNext();
  void installCursorAtPosition(int32_t rowPtr);

  std::unique_ptr<SQLString> getInternalString(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);
  Date getInternalDate(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);
  std::unique_ptr<Time> getInternalTime(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);
  std::unique_ptr<Timestamp> getInternalTimestamp( ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);

  int32_t getInternalInt(ColumnDefinition* columnInfo);
  int64_t getInternalLong(ColumnDefinition* columnInfo);
  uint64_t getInternalULong(ColumnDefinition* columnInfo);
  float getInternalFloat(ColumnDefinition* columnInfo);
  long double getInternalDouble(ColumnDefinition* columnInfo);
  std::unique_ptr<BigDecimal> getInternalBigDecimal(ColumnDefinition* columnInfo);

  bool getInternalBoolean(ColumnDefinition* columnInfo);
  int8_t getInternalByte(ColumnDefinition* columnInfo);
  int16_t getInternalShort(ColumnDefinition* columnInfo);
  SQLString getInternalTimeString(ColumnDefinition* columnInfo);


  bool isBinaryEncoded();
  };

}
}
}
#endif