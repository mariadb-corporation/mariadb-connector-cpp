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


#include "ColumnNameMap.h"

#include "ColumnDefinition.h"
#include "ExceptionFactory.h"

namespace sql
{
namespace mariadb
{

  ColumnNameMap::ColumnNameMap(std::vector<Shared::ColumnDefinition>& columnInformations)
    : columnInfo(columnInformations)
  {}

  /**
    * Get column index by name.
    *
    * @param name column name
    * @return index.
    * @throws SQLException if no column info exists, or column is unknown
    */
  int32_t ColumnNameMap::getIndex(const SQLString& name)
  {
    if (name.empty() == true) {
      throw SQLException("Column name cannot be empty");
    }
    SQLString lowerName(name);

    lowerName.toLowerCase();

    if (aliasMap.empty())
    {
      int32_t counter= 0;

      for (auto& ci : columnInfo)
      {
        SQLString columnAlias(ci->getName());
        if (!columnAlias.empty())
        {
          columnAlias.toLowerCase();

          if (aliasMap.find(columnAlias) == aliasMap.end())
          {
            aliasMap.emplace(columnAlias, counter);
          }

          SQLString keyName(ci->getTable());

          if (!keyName.empty())
          {
            keyName.toLowerCase().append('.').append(columnAlias);
            if (aliasMap.find(keyName) != aliasMap.end())
            {
              aliasMap.emplace(keyName, counter);
            }
          }
        }
        ++counter;
      }
    }

    const auto& cit= aliasMap.find(lowerName);
    if (cit != aliasMap.end()) {
      return cit->second;
    }

    if (originalMap.empty())
    {
      int32_t counter= 0;
      for (auto& ci : columnInfo)
      {
        SQLString columnRealName(ci->getOriginalName());

        if (columnRealName.empty()) {
          columnRealName= columnRealName.toLowerCase();

          if (originalMap.find(columnRealName) == originalMap.end())
          {
            originalMap.emplace(columnRealName, counter);
          }

          SQLString keyName(ci->getOriginalTable());
          if (!keyName.empty())
          {
            keyName.toLowerCase().append('.').append(columnRealName);
            if (originalMap.find(keyName) == originalMap.end())
            {
              originalMap.emplace(keyName, counter);
            }
          }
        }
        ++counter;
      }
    }

    const auto& otherCit= originalMap.find(lowerName);

    if (otherCit == originalMap.end()) {
      //throw ExceptionMapper::get("No such column: "+name, "42S22", 1054, NULL, false);
      throw IllegalArgumentException("No such column: " + name, "42S22", 1054);
    }
    return cit->second;
  }

}
}
