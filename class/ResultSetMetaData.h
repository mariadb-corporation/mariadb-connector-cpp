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


#ifndef _RESULTSETMETADATA_H_
#define _RESULTSETMETADATA_H_

#include <vector>
#include <ColumnDefinition.h>

namespace mariadb
{
class ResultSetMetaData
{
  const std::vector<ColumnDefinition>& field;
  bool forceAlias;
  std::vector<MYSQL_FIELD> rawField;
public:
  enum {
    columnNoNulls= 0,
    columnNullable,
    columnNullableUnknown
  };

  ResultSetMetaData(const std::vector<ColumnDefinition>& columnsInformation , bool forceAlias= false);
  const MYSQL_FIELD* getFields() const { return rawField.data(); };
  const MYSQL_FIELD* getField(std::size_t columnIdx) const {
    return field[columnIdx].getColumnRawData();
  }
  uint32_t getColumnCount();

  bool isAutoIncrement(uint32_t column) const;
  bool isCaseSensitive(uint32_t column) const;
  bool isSearchable(uint32_t column) const;
  int32_t isNullable(uint32_t column) const;
  bool isSigned(uint32_t column) const;
  uint32_t getColumnDisplaySize(uint32_t column) const;
  SQLString getColumnLabel(uint32_t column) const;
  SQLString getColumnName(uint32_t column) const;
  SQLString getCatalogName(uint32_t column) const;
  uint32_t getPrecision(uint32_t column) const;
  uint32_t getScale(uint32_t column) const;
  SQLString getTableName(uint32_t column) const;
  SQLString getSchemaName(uint32_t column) const;
  int32_t getColumnType(uint32_t column) const;
  SQLString getColumnTypeName(uint32_t column) const;
  bool isReadOnly(uint32_t column) const;
  bool isWritable(uint32_t column) const;
  bool isDefinitelyWritable(uint32_t column) const;
  bool isZerofill(uint32_t column) const;
  SQLString getColumnCollation(uint32_t column) const;

private:
  const ColumnDefinition& getColumnDefinition(uint32_t column) const;
};

} // namespace mariadb
#endif
