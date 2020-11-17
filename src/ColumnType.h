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


#ifndef _COLUMNTYPE_H_
#define _COLUMNTYPE_H_

#include "Consts.h"

namespace sql
{
namespace mariadb
{

class ColumnType
{
  static const std::map<int32_t, const ColumnType&> typeMap;

  const int16_t mariadbType;
  const int32_t javaType;
  const SQLString javaTypeName;
  const SQLString className;
  const std::size_t binSize;

  ColumnType(int32_t mariadbType, int32_t javaType, const SQLString& javaTypeName, const SQLString& className, size_t binBindTypeSize);

public:
  static const ColumnType OLDDECIMAL;
  static const ColumnType TINYINT;
  static const ColumnType SMALLINT;
  static const ColumnType INTEGER;
  static const ColumnType FLOAT;
  static const ColumnType DOUBLE;
  static const ColumnType _NULL;
  static const ColumnType TIMESTAMP;
  static const ColumnType BIGINT;
  static const ColumnType MEDIUMINT;
  static const ColumnType DATE;
  static const ColumnType TIME;
  static const ColumnType DATETIME;
  static const ColumnType YEAR;
  static const ColumnType NEWDATE;
  static const ColumnType VARCHAR;
  static const ColumnType BIT;
  static const ColumnType JSON;
  static const ColumnType DECIMAL;
  static const ColumnType ENUM;
  static const ColumnType SET;
  static const ColumnType TINYBLOB;
  static const ColumnType MEDIUMBLOB;
  static const ColumnType LONGBLOB;
  static const ColumnType BLOB;
  static const ColumnType VARSTRING;
  static const ColumnType STRING;
  static const ColumnType GEOMETRY;

  /* Will see if typeif fits what the method is used for. Probably would be better to allocate and return
     type object */
  static const std::type_info& classFromJavaType(int32_t type);
  static bool isNumeric(const ColumnType& type);
  static SQLString getColumnTypeName(const ColumnType& type, int64_t len, int64_t charLen, bool _signed, bool binary);
  static const ColumnType& fromServer(int32_t typeValue, int32_t charsetNumber);
  static const ColumnType& toServer(int32_t javaType);
  static SQLString getClassName(const ColumnType& type, int32_t len, bool _signed, bool binary, Shared::Options options);

  const SQLString& getClassName() const;
  int32_t getSqlType() const;
  const SQLString& getTypeName() const;
  int16_t getType() const;
  const SQLString & getCppTypeName() const;
  std::size_t binarySize() const { return binSize; }
};

bool operator==(const ColumnType& type1, const ColumnType& type2);

bool operator!=(const ColumnType& type1, const ColumnType& type2);

}
}
#endif
