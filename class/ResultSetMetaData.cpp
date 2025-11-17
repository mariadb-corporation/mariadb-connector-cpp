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

#include "ma_odbc.h"
#include "ResultSetMetaData.h"
#include "ColumnDefinition.h"
#include "Exception.h"


namespace mariadb
{
  /**
    * Constructor.
    *
    * @param fields column informations
    * @param forceAlias force table and column name alias as original data
    */
  ResultSetMetaData::ResultSetMetaData(const std::vector<ColumnDefinition>& columnsInformation, bool _forceAlias)
    : field(columnsInformation)
    , forceAlias(_forceAlias)
  {
    for (auto& ci : field)
    {
      rawField.push_back(*const_cast<MYSQL_FIELD*>(ci.getColumnRawData()));
    }
  }

  /**
    * Returns the number of columns in this <code>ResultSet</code> object.
    *
    * @return the number of columns
    */
  uint32_t ResultSetMetaData::getColumnCount()
  {
    return static_cast<uint32_t>(field.size());
  }

 #ifndef LEAVING_THIS_SO_FAR_HERE_COMMENTED
  /**
    * Indicates whether the designated column is automatically numbered.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    * @throws SQLException if a database access error occurs
    */
  bool ResultSetMetaData::isAutoIncrement(uint32_t column)  const
  {
    return (getColumnDefinition(column).getFlags() & AUTO_INCREMENT_FLAG)!=0;
  }

  /**
    * Indicates whether a column's case matters.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    * @throws SQLException if a database access error occurs
    */
  bool ResultSetMetaData::isCaseSensitive(uint32_t column) const
  {
    return (getColumnDefinition(column).getFlags() & BINARY_FLAG)!=0;
  }


  /**
    * Indicates the nullability of values in the designated column.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return the nullability status of the given column; one of <code>columnNoNulls</code>, <code>
    *     columnNullable</code> or <code>columnNullableUnknown</code>
    * @throws SQLException if a database access error occurs
    */
  int32_t ResultSetMetaData::isNullable(uint32_t column)  const
  {
    if ((getColumnDefinition(column).getFlags() & NOT_NULL_FLAG)==0) {
      return ResultSetMetaData::columnNullable;
    }
    else {
      return ResultSetMetaData::columnNoNulls;
    }
  }

  /**
    * Indicates whether values in the designated column are signed numbers.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    * @throws SQLException if a database access error occurs
    */
  bool ResultSetMetaData::isSigned(uint32_t column)  const
  {
    return getColumnDefinition(column).isSigned();
  }

  /**
    * Indicates the designated column's normal maximum width in characters.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return the normal maximum number of characters allowed as the width of the designated column
    * @throws SQLException if a database access error occurs
    */
  uint32_t ResultSetMetaData::getColumnDisplaySize(uint32_t column) const
  {
    return getColumnDefinition(column).getDisplaySize();
  }

  /**
    * Gets the designated column's suggested title for use in printouts and displays. The suggested
    * title is usually specified by the SQL <code>AS</code> clause. If a SQL <code>AS</code> is not
    * specified, the value returned from <code>getColumnLabel</code> will be the same as the value
    * returned by the <code>getColumnName</code> method.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return the suggested column title
    * @throws SQLException if a database access error occurs
    */
  SQLString ResultSetMetaData::getColumnLabel(uint32_t column) const
  {
    return getColumnDefinition(column).getName();
  }

  /**
    * Get the designated column's name.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return column name
    * @throws SQLException if a database access error occurs
    */
  SQLString ResultSetMetaData::getColumnName(uint32_t column) const
  {
    SQLString columnName(getColumnDefinition(column).getOriginalName());
    if (columnName.empty() || forceAlias) {
      return getColumnLabel(column);
    }
    return columnName;
  }

  /**
    * Get the designated column's table's schema.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return schema name or "" if not applicable
    * @throws SQLException if a database access error occurs
    */
  SQLString ResultSetMetaData::getCatalogName(uint32_t column) const
  {
    // Just to check that valid column index requested
    getColumnDefinition(column);
    return "";
  }

  /**
    * Get the designated column's specified column size. For numeric data, this is the maximum
    * precision. For character data, this is the length in characters. For datetime datatypes, this
    * is the length in characters of the String representation (assuming the maximum allowed
    * precision of the fractional seconds component). For binary data, this is the length in bytes.
    * For the ROWID datatype, this is the length in bytes. 0 is returned for data types where the
    * column size is not applicable.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return precision
    * @throws SQLException if a database access error occurs
    */
  uint32_t ResultSetMetaData::getPrecision(uint32_t column) const
  {
    return static_cast<uint32_t>(getColumnDefinition(column).getPrecision());
  }

  /**
    * Gets the designated column's number of digits to right of the decimal point. 0 is returned for
    * data types where the scale is not applicable.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return scale
    * @throws SQLException if a database access error occurs
    */
  uint32_t ResultSetMetaData::getScale(uint32_t column) const
  {
    return getColumnDefinition(column).getDecimals();
  }

  /**
    * Gets the designated column's table name.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return table name or "" if not applicable
    * @throws SQLException if a database access error occurs
    */
  SQLString ResultSetMetaData::getTableName(uint32_t column) const
  {
    if (forceAlias) {
      return getColumnDefinition(column).getTable();
    }

    return getColumnDefinition(column).getOriginalTable();
  }

  SQLString ResultSetMetaData::getSchemaName(uint32_t column) const
  {
    return getColumnDefinition(column).getDatabase();
  }

  /**
    * Retrieves the designated column's SQL type.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return SQL type from java.sql.Types
    * @throws SQLException if a database access error occurs
    * @see Types
    */
  int32_t ResultSetMetaData::getColumnType(uint32_t column) const
  {
    const ColumnDefinition& ci= getColumnDefinition(column);
    auto type= ci.getColumnType();
    auto length= ci.getLength();

    switch (type)
    {
    case MYSQL_TYPE_BIT:
      if (length == 1) {
        return MYSQL_TYPE_BIT;
      }
      return MYSQL_TYPE_BLOB;
    case MYSQL_TYPE_BLOB:
      if (length > MAX_MEDIUM_BLOB) {
        return MYSQL_TYPE_LONG_BLOB;
      }
      return MYSQL_TYPE_BLOB;
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
      if (ci.isBinary()) {
        return MYSQL_TYPE_BLOB;
      }
      if (length > static_cast<uint32_t>(INT32_MAX)) {
        return MYSQL_TYPE_LONG_BLOB;
      }
      return MYSQL_TYPE_STRING;
    case MYSQL_TYPE_STRING:
      if (ci.isBinary()) {
        return MYSQL_TYPE_TINY_BLOB;
      }
      return MYSQL_TYPE_STRING;
    default:
      return type;
    }
  }

  /**
    * Retrieves the designated column's database-specific type name.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return type name used by the database. If the column type is a user-defined type, then a
    *     fully-qualified type name is returned.
    * @throws SQLException if a database access error occurs
    */
  SQLString ResultSetMetaData::getColumnTypeName(uint32_t column) const
  {
    return getColumnDefinition(column).getColumnTypeName();
  }

  /**
    * Indicates whether the designated column is definitely not writable.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool ResultSetMetaData::isReadOnly(uint32_t column) const
  {
    return getColumnDefinition(column).isReadonly();
  }

  /**
    * Indicates whether it is possible for a write on the designated column to succeed.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool ResultSetMetaData::isWritable(uint32_t column) const
  {
    return !isReadOnly(column);
  }

  /**
    * Indicates whether a write on the designated column will definitely succeed.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool ResultSetMetaData::isDefinitelyWritable(uint32_t column) const
  {
    return !isReadOnly(column);
  }


  const ColumnDefinition& ResultSetMetaData::getColumnDefinition(uint32_t column) const
  {
    if (column >=1 && column <= field.size()) {
      return field[column - 1];
    }
    throw SQLException("No such column", "42000");
  }


  bool ResultSetMetaData::isZerofill(uint32_t column) const
  {
    return getColumnDefinition(column).isZeroFill();
  }


  SQLString ResultSetMetaData::getColumnCollation(uint32_t column) const
  {
    return getColumnDefinition(column).getCollation();
  }
#endif // ifndef LEAVING_THIS_SO_FAR_HERE_COMMENTED

} // namespace mariadb
