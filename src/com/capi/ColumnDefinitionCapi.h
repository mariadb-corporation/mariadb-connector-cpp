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


#ifndef _COLUMNINFORMATIONCAPI_H_
#define _COLUMNINFORMATIONCAPI_H_

#include "ColumnType.h"
#include "Consts.h"

#include "ColumnDefinition.h"

namespace sql
{
namespace mariadb
{
namespace capi
{

#include "mysql.h"

class ColumnDefinitionCapi : public sql::mariadb::ColumnDefinition
{
  static int32_t maxCharlen[];
  std::shared_ptr<MYSQL_FIELD> metadata;
  const ColumnType& type;
  int64_t length;
  //SQLString db;

public:
  ColumnDefinitionCapi(const ColumnDefinitionCapi& other);
  ColumnDefinitionCapi(capi::MYSQL_FIELD* metadata);

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
  const ColumnType& getColumnType() const;
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
}
#endif
