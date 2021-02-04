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


#ifndef _MARIADBRESULTSETMETADATA_H_
#define _MARIADBRESULTSETMETADATA_H_

#include "Consts.h"
#include "ResultSetMetaData.hpp"

namespace sql
{
namespace mariadb
{
class ColumnDefinition;

class MariaDbResultSetMetaData : public sql::ResultSetMetaData
{
  const std::vector<Shared::ColumnDefinition> fieldPackets;
  const Shared::Options options;
  bool forceAlias;

public:
  MariaDbResultSetMetaData(const std::vector<Shared::ColumnDefinition>& fieldPackets, const Shared::Options& options,bool forceAlias);
  uint32_t getColumnCount();
  bool isAutoIncrement(uint32_t column);
  bool isCaseSensitive(uint32_t column);
  bool isSearchable(uint32_t column);
  bool isCurrency(uint32_t column);
  int32_t isNullable(uint32_t column);
  bool isSigned(uint32_t column);
  uint32_t getColumnDisplaySize(uint32_t column);
  SQLString getColumnLabel(uint32_t column);
  SQLString getColumnName(uint32_t column);
  SQLString getCatalogName(uint32_t column);
  uint32_t getPrecision(uint32_t column);
  uint32_t getScale(uint32_t column);
  SQLString getTableName(uint32_t column);
  SQLString getSchemaName(uint32_t column);
  int32_t getColumnType(uint32_t column);
  SQLString getColumnTypeName(uint32_t column);
  bool isReadOnly(uint32_t column);
  bool isWritable(uint32_t column);
  bool isDefinitelyWritable(uint32_t column);
  SQLString getColumnClassName(uint32_t column);
  bool isZerofill(uint32_t column);
  SQLString getColumnCollation(uint32_t column);

private:
  const ColumnDefinition& getColumnDefinition(uint32_t column);

// It's rather useless(at least in this form). Commenting out so far
/*public:
  bool isWrapperFor(ResultSetMetaData *iface);*/
  };
}
}
#endif