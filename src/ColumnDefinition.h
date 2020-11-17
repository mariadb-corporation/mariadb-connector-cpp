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


#ifndef _COLUMNINFORMATIONIFACE_H_
#define _COLUMNINFORMATIONIFACE_H_

#include "ColumnType.h"

namespace sql
{

namespace mariadb
{

class ColumnDefinition {

protected:
  ColumnDefinition(const ColumnDefinition&other)=default;
  ColumnDefinition()=default;
public:

  static Shared::ColumnDefinition create(const SQLString& name, const ColumnType& _type);

  virtual ~ColumnDefinition()=default;

  virtual SQLString getDatabase() const=0;
  virtual SQLString getTable() const=0;
  virtual SQLString getOriginalTable() const=0;
  virtual SQLString getName() const=0;
  virtual SQLString getOriginalName() const=0;
  virtual short getCharsetNumber() const=0;
  virtual SQLString getCollation() const=0;
  /* Length of the column */
  virtual uint32_t getLength() const=0;
  /* Max length of the column in the resultset(if available) */
  virtual uint32_t getMaxLength() const=0;
  virtual int64_t getPrecision() const=0;
  virtual uint32_t getDisplaySize() const=0;
  virtual uint8_t getDecimals() const=0;
  virtual const ColumnType& getColumnType() const=0;
  virtual short getFlags() const=0;
  virtual bool isSigned() const=0;
  virtual bool isNotNull() const=0;
  virtual bool isPrimaryKey() const=0;
  virtual bool isUniqueKey() const=0;
  virtual bool isMultipleKey() const=0;
  virtual bool isBlob() const=0;
  virtual bool isZeroFill() const=0;
  virtual bool isBinary() const=0;
  virtual bool isReadonly() const=0;
};

}
}
#endif

