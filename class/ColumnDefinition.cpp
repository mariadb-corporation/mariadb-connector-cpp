/************************************************************************************
   Copyright (C) 2022, 2025 MariaDB Corporation plc

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


namespace mariadb
{
  long getTypeBinLength(enum_field_types type);
  extern const SQLString emptyStr;

  std::map<enum_field_types, SQLString> typeName{{MYSQL_TYPE_LONG, "INT"}, {MYSQL_TYPE_LONGLONG, "BIGINT"}, {MYSQL_TYPE_SHORT, "SMALLINT"},
    {MYSQL_TYPE_INT24, "MEDIUMINT"}, {MYSQL_TYPE_BLOB, "BLOB"}, {MYSQL_TYPE_TINY_BLOB, "TINYBLOB"}, {MYSQL_TYPE_MEDIUM_BLOB, "MEDIUMBLOB"},
    {MYSQL_TYPE_LONG_BLOB, "LONGBLOB"}, {MYSQL_TYPE_DATE, "DATE"}, {MYSQL_TYPE_TIME, "TIME"}, {MYSQL_TYPE_DATETIME, "DATETIME"},
    {MYSQL_TYPE_YEAR, "YEAR"}, {MYSQL_TYPE_NEWDATE, "DATE"}, {MYSQL_TYPE_TIMESTAMP, "TIMESTAMP"}, {MYSQL_TYPE_NEWDECIMAL, "DECIMAL"},
    {MYSQL_TYPE_JSON, "JSON"}, {MYSQL_TYPE_GEOMETRY, "GEOMETRY"}, {MYSQL_TYPE_ENUM, "ENUM"}, {MYSQL_TYPE_SET, "SET"}};

  bool isIntegerType(enum enum_field_types type)
  {
    switch (type) {
    case MARIADB_INTEGER_TYPES:
      return true;
    // to avoid warnings
    default:
      return false;
    }
    return false;
  }

  /**
    * Get columnTypeName.
    *
    * @param type type
    * @param len len
    * @param _signed _signed
    * @param binary binary
    * @return type
    */
  SQLString columnTypeName(enum enum_field_types type, int64_t len, int64_t charLen, bool _signed, bool binary)
  {
    if (isIntegerType(type))
    {
      if (!_signed) {
        return typeName[type] + " UNSIGNED";
      }
      else {
        return typeName[type];
      }
    }
    else if (type == MYSQL_TYPE_BLOB)
    {
      /*
      map to different blob types based on datatype length
      see https://mariadb.com/kb/en/library/data-types/
      */
      if (len <= MAX_TINY_BLOB) {
        return binary ? "TINYBLOB" : "TINYTEXT";
      }
      else if (len <= MAX_BLOB_LEN) {
        return binary ? "BLOB" : "TEXT";
      }
      else if (len <= MAX_MEDIUM_BLOB) {
        return binary ? "MEDIUMBLOB" : "MEDIUMTEXT";
      }
      else {
        return binary ? "LONGBLOB" : "LONGTEXT";
      }
    }
    else if (type == MYSQL_TYPE_VAR_STRING || type == MYSQL_TYPE_VARCHAR)
    {
      if (binary) {
        return "VARBINARY";
      }

      if (len > INT32_MAX) {
        return "LONGTEXT";
      }
      else if (charLen <= MAX_TINY_BLOB) {
        return "VARCHAR";
      }
      else if (charLen <= MAX_BLOB_LEN) {
        return "TEXT";
      }
      else if (charLen <= MAX_MEDIUM_BLOB) {
        return "MEDIUMTEXT";
      }
      else {
        return "LONGTEXT";
      }
    }
    else if (type == MYSQL_TYPE_STRING)
    {
      if (binary) {
        return "BINARY";
      }
      return "CHAR";
    }
    else
    {
      return typeName[type];
    }
  }

  SQLString ColumnDefinition::getColumnTypeName() const
  {
    return std::move(columnTypeName(getColumnType(), getLength(), getDisplaySize(), isSigned(), isBinary()));
  }

  // where character_sets.character_set_name= collations.character_set_name order by id"
  uint8_t ColumnDefinition::maxCharlen[]={
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

  ColumnDefinition ColumnDefinition::create(const SQLString& name, const MYSQL_FIELD* _type)
  {
    //TODO: this feels wrong
    /*md->name= const_cast<char*>(result.name.c_str());
    md->org_name= md->name;
    md->name_length= static_cast<unsigned int>(name.length());
    md->org_name_length= static_cast<unsigned int>(name.length());

    if (md->length == 0) {
      switch (_type->type) {
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_STRING:
        md->length= 64 * 3;
        break;
      case MYSQL_TYPE_SHORT:
        md->length= 5;
        break;
      case MYSQL_TYPE_NULL:
        md->length= 0;
        break;
      default:
        md->length= 1;
        break;
      }
    }*/
    return ColumnDefinition(name, _type);
  }


  void ColumnDefinition::fieldDeafaultBind(const ColumnDefinition & cd, MYSQL_BIND & bind, int8_t ** buffer)
  {
    bind.buffer_type= cd.getColumnType();
    if (bind.buffer_type == MYSQL_TYPE_VARCHAR) {
      bind.buffer_type= MYSQL_TYPE_STRING;
    }

    long binLength= getTypeBinLength(cd.getColumnType());
    bind.buffer_length= static_cast<unsigned long>(binLength > 0 ?
      binLength :
      cd.getMaxLength() > 0 ? cd.getMaxLength() : cd.getLength());

    bind.buffer= nullptr;
    if (buffer) {
      bind.buffer= new uint8_t[bind.buffer_length];
      if (bind.buffer) {
        *buffer= static_cast<int8_t*>(bind.buffer);
      }
    }

    bind.length=  &bind.length_value;
    bind.is_null= &bind.is_null_value;
    bind.error=   &bind.error_value;
    //bind.flags|=        MADB_BIND_DUMMY;
  }

  /* Refreshing pointers in FIELD structure to local names */
  void ColumnDefinition::refreshPointers()
  {
    metadata->name=             const_cast<char*>(name.data());
    metadata->name_length=      static_cast<unsigned int>(name.length());
    metadata->org_name=         const_cast<char*>(org_name.data());//metadata->name;
    metadata->org_name_length=  static_cast<unsigned int>(org_name.length());
    metadata->table=            const_cast<char*>(table.data());
    metadata->table_length=     static_cast<unsigned int>(table.length());
    metadata->org_table=        const_cast<char*>(org_table.data());
    metadata->org_table_length= static_cast<unsigned int>(org_table.length());
    metadata->db=               const_cast<char*>(db.data());
    metadata->db_length=        static_cast<unsigned int>(db.length());
  }


  ColumnDefinition::~ColumnDefinition()
  {
    if (metadata) {
      delete metadata;
    }
  }

  /**
    * Constructor for extent.
    *
    * @param other other columnInformation
    */
  ColumnDefinition::ColumnDefinition(const ColumnDefinition& other) :
    metadata(new MYSQL_FIELD(*other.metadata)),
    name(other.name),
    org_name(other.org_name),
    table(other.table),
    org_table(other.org_table),
    db(other.db),
    length(other.length)
  {
     refreshPointers();
  }


  ColumnDefinition::ColumnDefinition(ColumnDefinition&& other) noexcept :
    metadata(other.metadata),
    name(std::move(other.name)),
    org_name(std::move(other.org_name)),
    table(std::move(other.table)),
    org_table(std::move(other.org_table)),
    db(std::move(other.db)),
    length(other.length)
  {
    refreshPointers();
    other.metadata= nullptr;
  }

  ColumnDefinition::ColumnDefinition(const MYSQL_FIELD* field, bool ownshipPassed) :
    metadata(ownshipPassed ? const_cast<MYSQL_FIELD*>(field) : new MYSQL_FIELD(*field) ),
    name(field->name ? field->name : ""),
    org_name(field->org_name ? field->org_name : ""),
    table(field->table ? field->table : ""),
    org_table(field->org_table ? field->org_table : ""),
    db(field->db ? field->db : ""),
    length(std::max(field->length, field->max_length))
  {
    //std::memcpy(metadata, field, sizeof(MYSQL_FIELD));
    refreshPointers();
    if (metadata->length == 0) {
      switch (metadata->type) {
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_STRING:
        metadata->length= 64 * 3;
        break;
      case MYSQL_TYPE_SHORT:
        metadata->length= 5;
        break;
      case MYSQL_TYPE_NULL:
        metadata->length= 0;
        break;
      default:
        metadata->length= 1;
        break;
      }
    }
  }

  ColumnDefinition& ColumnDefinition::operator=(const ColumnDefinition& other)
  {
    org_name= other.org_name;
    table= other.table;
    org_table= other.org_table;
    db= other.db;
    length= other.length;
    metadata= new MYSQL_FIELD(*other.metadata);
    refreshPointers();
    return *this;
  }

  /**
    * Read column information from buffer.
    *
    * @param buffer buffer
    */
  ColumnDefinition::ColumnDefinition(const SQLString _name, const MYSQL_FIELD* _metadata, bool ownshipPassed) :
     ColumnDefinition(_metadata, ownshipPassed)
  {
    name= _name;
    metadata->name= const_cast<char*>(name.c_str());
    metadata->name_length= static_cast<unsigned int>(name.length());
    metadata->org_name= metadata->name;
    metadata->org_name_length= static_cast<unsigned int>(name.length());
    length= std::max(_metadata->length, _metadata->max_length);
  }


  SQLString ColumnDefinition::getDatabase() const {
    return db;
  }


  SQLString ColumnDefinition::getTable() const {
    return table;
  }

  SQLString ColumnDefinition::getOriginalTable() const {
    return org_table;
  }


  bool ColumnDefinition::isReadonly() const {
    return (metadata->db == nullptr || *(metadata->db) == '\0');
  }


  SQLString ColumnDefinition::getName() const
  {
    return name;
  }

  SQLString ColumnDefinition::getOriginalName() const {
    return org_name;
  }

  int16_t ColumnDefinition::getCharsetNumber() const {
    return metadata->charsetnr;
  }

  SQLString ColumnDefinition::getCollation() const {
    MARIADB_CHARSET_INFO *cs= mariadb_get_charset_by_nr(metadata->charsetnr);

    if (cs != nullptr) {
      return cs->name;
    }

    return emptyStr;
  }
  uint32_t ColumnDefinition::getLength() const {
    return length;//std::max(metadata->length, metadata->max_length);
  }

  uint32_t ColumnDefinition::getMaxLength() const
  {
    return metadata->max_length;
  }

  /**
    * Return metadata precision.
    *
    * @return precision
    */
  int64_t ColumnDefinition::getPrecision() const {
    if (metadata->type == MYSQL_TYPE_DECIMAL || metadata->type == MYSQL_TYPE_NEWDECIMAL)
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
  uint32_t ColumnDefinition::getDisplaySize() const {
    
    if (metadata->type == MYSQL_TYPE_VARCHAR || metadata->type == MYSQL_TYPE_STRING ||
      metadata->type == MYSQL_TYPE_VAR_STRING ) {
      uint8_t maxWidth= maxCharlen[metadata->charsetnr & 0xff];
      if (maxWidth == 0) {
        maxWidth= 1;
      }

      return length / maxWidth;
    }
    return length;
  }

  uint8_t ColumnDefinition::getDecimals() const {
    return metadata->decimals;
  }


  int16_t ColumnDefinition::getFlags() const {
    return metadata->flags;
  }

  bool ColumnDefinition::isSigned() const {
    return ((metadata->flags & UNSIGNED_FLAG) == 0);
  }

  bool ColumnDefinition::isNotNull() const {
    return ((metadata->flags & 1) > 0);
  }

  bool ColumnDefinition::isPrimaryKey() const {
    return ((metadata->flags & 2) > 0);
  }

  bool ColumnDefinition::isUniqueKey() const {
    return ((metadata->flags & 4) > 0);
  }

  bool ColumnDefinition::isMultipleKey() const {
    return ((metadata->flags & 8) > 0);
  }

  bool ColumnDefinition::isBlob() const {
    return ((metadata->flags & 16) > 0);
  }

  bool ColumnDefinition::isZeroFill() const {
    return ((metadata->flags & 64) > 0);
  }

  // like string), but have the binary flag
  bool ColumnDefinition::isBinary() const {
    return (getCharsetNumber() == 63);
  }

} // namespace mariadb
