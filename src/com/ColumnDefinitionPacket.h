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


#ifndef _COLUMNINFORMATION_H_
#define _COLUMNINFORMATION_H_

#include "ColumnType.h"
#include "Consts.h"

#include "ColumnDefinition.h"

namespace sql
{
namespace mariadb
{
/* TODO: that  probably has to be interface, and/or this class implementation of that interface */
class ColumnDefinitionPacket : public ColumnDefinition
{
  static int32_t maxCharlen[];
  Shared::Buffer buffer;
  int16_t charsetNumber;
  int64_t length;
  ColumnType type;
  uint8_t decimals;
  int16_t flags;

public:
  ColumnDefinitionPacket(const ColumnDefinitionPacket& other);
  ColumnDefinitionPacket(Buffer* buffer);

  static Shared::ColumnDefinition create(const SQLString& name, const ColumnType& type);

private:
  SQLString getString(int32_t idx);

public:
  SQLString getDatabase() const;
  SQLString getTable() const;
  SQLString getOriginalTable() const;
  SQLString getName() const;
  SQLString getOriginalName() const;
  int16_t getCharsetNumber() const;
  int64_t getLength() const;
  int64_t getPrecision() const;
  int32_t getDisplaySize() const;
  uint8_t getDecimals() const;
  ColumnType getColumnType() const;
  int16_t getFlags() const;
  bool isSigned() const;
  bool isNotNull() const;
  bool isPrimaryKey() const;
  bool isUniqueKey() const;
  bool isMultipleKey() const;
  bool isBlob() const;
  bool isZeroFill() const;
  bool isBinary() const;
};

}
}
#endif
