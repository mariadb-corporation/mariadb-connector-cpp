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



#include "ColumnDefinition.h"

#include "com/capi/ColumnDefinitionCapi.h"

namespace sql
{

namespace mariadb
{
  /**
  * Constructor.
  *
  * @param name column name
  * @param type column type
  * @return Shared::ColumnDefinition
  */
  Shared::ColumnDefinition ColumnDefinition::create(const SQLString& name, const ColumnType& _type)
  {
    capi::MYSQL_FIELD* md= new capi::MYSQL_FIELD;

    std::memset(md, 0, sizeof(capi::MYSQL_FIELD));

    md->name= (char*)name.c_str();
    md->org_name= (char*)name.c_str();
    md->name_length= static_cast<unsigned int>(name.length());
    md->org_name_length= static_cast<unsigned int>(name.length());

    switch (_type.getSqlType()) {
    case Types::VARCHAR:
    case Types::CHAR:
      md->length= 64*3;
      break;
    case Types::SMALLINT:
      md->length= 5;
      break;
    case Types::_NULL:
      md->length= 0;
      break;
    default:
      md->length= 1;
      break;
    }

    md->type= static_cast<capi::enum_field_types>(ColumnType::toServer(_type.getSqlType()).getType());

    return Shared::ColumnDefinition(new capi::ColumnDefinitionCapi(md, true));
  }

}
}

