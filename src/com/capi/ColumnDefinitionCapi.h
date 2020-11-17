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
  static uint8_t maxCharlen[];
  MYSQL_FIELD* metadata;
  /** Fow "hand-made" RS we need to take care of freeing memory, while for "natural" MYSQL_FIELD
    we have to use pointer to C/C structures(to automatically get max_length when it's calculated -
    that happens later, than the object is created).
    It has to be shared since we have copy-copyconstructor(not sure why we have it)
  */
  std::shared_ptr<MYSQL_FIELD> owned;
  const ColumnType& type;
  uint32_t length;
  //SQLString db;

public:
  ColumnDefinitionCapi(const ColumnDefinitionCapi& other);
  ColumnDefinitionCapi(capi::MYSQL_FIELD* metadata, bool ownshipPassed= false);

public:
  SQLString getDatabase() const;
  SQLString getTable() const;
  SQLString getOriginalTable() const;
  SQLString getName() const;
  SQLString getOriginalName() const;
  int16_t getCharsetNumber() const;
  SQLString getCollation() const;
  uint32_t getLength() const;
  uint32_t getMaxLength() const;
  int64_t getPrecision() const;
  uint32_t getDisplaySize() const;
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
  bool isReadonly() const;
};

}
}
}
#endif
