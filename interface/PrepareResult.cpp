/************************************************************************************
   Copyright (C) 2023 MariaDB Corporation AB

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


#include "PrepareResult.h"

namespace mariadb
{
  void PrepareResult::init(MYSQL_FIELD* fields, std::size_t fieldCount)
  {
    column.reserve(fieldCount);
    field.reserve(fieldCount);
    for (std::size_t i= 0; i < fieldCount; ++i) {
      column.emplace_back(&fields[i]);
      field.push_back(column.back().getColumnRawData());
    }
  }
}
