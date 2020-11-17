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


#include "ColumnDefinitionCapi.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
  // where character_sets.character_set_name = collations.character_set_name order by id"
  uint8_t ColumnDefinitionCapi::maxCharlen[]={
  0,2,1,1,1,1,1,1,
  1,1,1,1,3,2,1,1,
  1,0,1,2,1,1,1,1,
  2,1,1,1,2,1,1,1,
  1,3,1,2,1,1,1,1,
  1,1,1,1,1,4,4,1,
  1,1,1,1,1,1,4,4,
  0,1,1,1,4,4,0,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,0,1,1,1,
  1,1,1,3,2,2,2,2,
  2,1,2,3,1,1,1,2,
  2,3,3,1,0,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,0,0,0,0,0,0,0,
  2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,
  2,2,2,2,0,2,0,0,
  0,0,0,0,0,0,0,2,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,0,0,0,0,
  0,0,0,0,0,0,0,0,
  3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,
  3,3,3,3,0,3,4,4,
  0,0,0,0,0,0,0,3,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,0,4,0,0,
  0,0,0,0,0,0,0,0
  };

  /**
    * Constructor for extent.
    *
    * @param other other columnInformation
    */
  ColumnDefinitionCapi::ColumnDefinitionCapi(const ColumnDefinitionCapi& other) :
    metadata(other.metadata),
    type(other.type),
    length(other.length),
    owned(other.owned)
  {
  }

  /**
    * Read column information from buffer.
    *
    * @param buffer buffer
    */
  ColumnDefinitionCapi::ColumnDefinitionCapi(capi::MYSQL_FIELD* _metadata, bool ownshipPassed) :
    metadata(_metadata),
    type(ColumnType::fromServer(metadata->type & 0xff, metadata->charsetnr)), // TODO: may be wrong
    length(std::max(_metadata->length, _metadata->max_length))
  {
    if (ownshipPassed) {
      owned.reset(_metadata);
    }
  }


  SQLString ColumnDefinitionCapi::getDatabase() const {
    return std::string(metadata->db, metadata->db_length);
  }


  SQLString ColumnDefinitionCapi::getTable() const {
    return metadata->table;
  }

  SQLString ColumnDefinitionCapi::getOriginalTable() const {
    return metadata->org_table;
  }


  bool ColumnDefinitionCapi::isReadonly() const {
    return (metadata->db == nullptr || *(metadata->db) == '\0');
  }


  SQLString ColumnDefinitionCapi::getName() const
  {
    return metadata->name;
  }

  SQLString ColumnDefinitionCapi::getOriginalName() const {
    return metadata->org_name;
  }

  int16_t ColumnDefinitionCapi::getCharsetNumber() const {
    return metadata->charsetnr;
  }

  SQLString ColumnDefinitionCapi::getCollation() const {
    MARIADB_CHARSET_INFO *cs= mariadb_get_charset_by_nr(metadata->charsetnr);

    if (cs != NULL) {
      return cs->name;
    }

    return emptyStr;
  }
  uint32_t ColumnDefinitionCapi::getLength() const {
    return length;//std::max(metadata->length, metadata->max_length);
  }

  uint32_t ColumnDefinitionCapi::getMaxLength() const
  {
    return metadata->max_length;
  }

  /**
    * Return metadata precision.
    *
    * @return precision
    */
  int64_t ColumnDefinitionCapi::getPrecision() const {
    if (type == ColumnType::OLDDECIMAL || type == ColumnType::DECIMAL)
    {
      if (isSigned()) {
        return length -((metadata->decimals >0) ? 2 : 1);
      }
      else {
        return length -((metadata->decimals >0) ? 1 : 0);
      }
    }
    return length;
  }

  /**
    * Get column size.
    *
    * @return size
    */
  uint32_t ColumnDefinitionCapi::getDisplaySize() const {
    int32_t vtype= type.getSqlType();
    if (vtype == Types::VARCHAR || vtype == Types::CHAR) {
      uint8_t maxWidth= maxCharlen[metadata->charsetnr & 0xff];
      if (maxWidth == 0) {
        maxWidth= 1;
      }

      return length / maxWidth;
    }
    return length;
  }

  uint8_t ColumnDefinitionCapi::getDecimals() const {
    return metadata->decimals;
  }

  const ColumnType& ColumnDefinitionCapi::getColumnType() const {
    return type;
  }

  int16_t ColumnDefinitionCapi::getFlags() const {
    return metadata->flags;
  }

  bool ColumnDefinitionCapi::isSigned() const {
    return ((metadata->flags & ColumnFlags::UNSIGNED) == 0);
  }

  bool ColumnDefinitionCapi::isNotNull() const {
    return ((metadata->flags & 1)>0);
  }

  bool ColumnDefinitionCapi::isPrimaryKey() const {
    return ((metadata->flags & 2)>0);
  }

  bool ColumnDefinitionCapi::isUniqueKey() const {
    return ((metadata->flags & 4)>0);
  }

  bool ColumnDefinitionCapi::isMultipleKey() const {
    return ((metadata->flags & 8)>0);
  }

  bool ColumnDefinitionCapi::isBlob() const {
    return ((metadata->flags & 16)>0);
  }

  bool ColumnDefinitionCapi::isZeroFill() const {
    return ((metadata->flags & 64)>0);
  }

  // like string), but have the binary flag
  bool ColumnDefinitionCapi::isBinary() const{
  return (getCharsetNumber() == 63);
  }

}
}
}
