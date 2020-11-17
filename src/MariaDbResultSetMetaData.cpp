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


#include "MariaDbResultSetMetaData.h"
#include "ColumnDefinition.h"
#include "ExceptionFactory.h"

namespace sql
{
namespace mariadb
{

  /**
    * Constructor.
    *
    * @param fieldPackets column informations
    * @param options connection options
    * @param forceAlias force table and column name alias as original data
    */
  MariaDbResultSetMetaData::MariaDbResultSetMetaData(const std::vector<Shared::ColumnDefinition>& _fieldPackets, const Shared::Options& _options, bool _forceAlias)
    : fieldPackets(_fieldPackets)
    , options(_options)
    , forceAlias(_forceAlias)
  {
  }

  /**
    * Returns the number of columns in this <code>ResultSet</code> object.
    *
    * @return the number of columns
    */
  uint32_t MariaDbResultSetMetaData::getColumnCount()
  {
    return static_cast<uint32_t>(fieldPackets.size());
  }

  /**
    * Indicates whether the designated column is automatically numbered.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    * @throws SQLException if a database access error occurs
    */
  bool MariaDbResultSetMetaData::isAutoIncrement(uint32_t column)
  {
    return (getColumnDefinition(column).getFlags() & ColumnFlags::AUTO_INCREMENT)!=0;
  }

  /**
    * Indicates whether a column's case matters.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    * @throws SQLException if a database access error occurs
    */
  bool MariaDbResultSetMetaData::isCaseSensitive(uint32_t column)
  {
    return (getColumnDefinition(column).getFlags() & ColumnFlags::BINARY_COLLATION)!=0;
  }

  /**
    * Indicates whether the designated column can be used in a where clause.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool MariaDbResultSetMetaData::isSearchable(uint32_t column)
  {
    // Just to check that valid column index requested
    getColumnDefinition(column);
    return true;
  }

  /**
    * Indicates whether the designated column is a cash value.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool MariaDbResultSetMetaData::isCurrency(uint32_t column)
  {
    // Just to check that valid column index requested
    getColumnDefinition(column);
    return false;
  }

  /**
    * Indicates the nullability of values in the designated column.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return the nullability status of the given column; one of <code>columnNoNulls</code>, <code>
    *     columnNullable</code> or <code>columnNullableUnknown</code>
    * @throws SQLException if a database access error occurs
    */
  int32_t MariaDbResultSetMetaData::isNullable(uint32_t column)
  {
    if ((getColumnDefinition(column).getFlags() & ColumnFlags::NOT_NULL)==0) {
      return sql::ResultSetMetaData::columnNullable;
    }
    else {
      return sql::ResultSetMetaData::columnNoNulls;
    }
  }

  /**
    * Indicates whether values in the designated column are signed numbers.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    * @throws SQLException if a database access error occurs
    */
  bool MariaDbResultSetMetaData::isSigned(uint32_t column)
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
  uint32_t MariaDbResultSetMetaData::getColumnDisplaySize(uint32_t column)
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
  SQLString MariaDbResultSetMetaData::getColumnLabel(uint32_t column)
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
  SQLString MariaDbResultSetMetaData::getColumnName(uint32_t column)
  {
    SQLString columnName(getColumnDefinition(column).getOriginalName());
    if (columnName.empty() || options->useOldAliasMetadataBehavior || forceAlias) {
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
  SQLString MariaDbResultSetMetaData::getCatalogName(uint32_t column)
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
  uint32_t MariaDbResultSetMetaData::getPrecision(uint32_t column)
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
  uint32_t MariaDbResultSetMetaData::getScale(uint32_t column)
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
  SQLString MariaDbResultSetMetaData::getTableName(uint32_t column)
  {
    if (forceAlias) {
      return getColumnDefinition(column).getTable();
    }

    if (options->blankTableNameMeta) {
      return "";
    }

    if (options->useOldAliasMetadataBehavior) {
      return getColumnDefinition(column).getTable();
    }
    return getColumnDefinition(column).getOriginalTable();
  }

  SQLString MariaDbResultSetMetaData::getSchemaName(uint32_t column)
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
  int32_t MariaDbResultSetMetaData::getColumnType(uint32_t column)
  {
    const ColumnDefinition& ci= getColumnDefinition(column);

    if (ci.getColumnType() == ColumnType::BIT)
    {
      if (ci.getLength()==1) {
        return Types::BIT;
      }
      return Types::VARBINARY;
    }
    else if(ci.getColumnType() == ColumnType::TINYINT) {
      if (ci.getLength()==1 &&options->tinyInt1isBit) {
        return Types::BIT;
      }
      return Types::TINYINT;
    }
    else if (ci.getColumnType() == ColumnType::YEAR) {
      if (options->yearIsDateType) {
        return Types::DATE;
      }
      return Types::SMALLINT;
    }
    else if (ci.getColumnType() == ColumnType::BLOB) {
      if (ci.getLength() > 16777215) {
        return Types::LONGVARBINARY;
      }
      return Types::VARBINARY;
    }
    else if (ci.getColumnType() == ColumnType::VARCHAR || ci.getColumnType() == ColumnType::VARSTRING) {
      if (ci.isBinary()) {
        return Types::VARBINARY;
      }
      if (ci.getLength() > static_cast<uint32_t>(INT32_MAX)) {
        return Types::LONGVARCHAR;
      }
      return Types::VARCHAR;
    }
    else if (ci.getColumnType() == ColumnType::STRING) {
      if (ci.isBinary()) {
        return Types::BINARY;
      }
      return Types::CHAR;
    }
    else {
      return ci.getColumnType().getSqlType();
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
  SQLString MariaDbResultSetMetaData::getColumnTypeName(uint32_t column)
  {
    const ColumnDefinition& ci= getColumnDefinition(column);
    return ColumnType::getColumnTypeName(
      ci.getColumnType(), ci.getLength(), ci.getDisplaySize(), ci.isSigned(), ci.isBinary());
  }

  /**
    * Indicates whether the designated column is definitely not writable.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool MariaDbResultSetMetaData::isReadOnly(uint32_t column)
  {
    return getColumnDefinition(column).isReadonly();
  }

  /**
    * Indicates whether it is possible for a write on the designated column to succeed.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool MariaDbResultSetMetaData::isWritable(uint32_t column)
  {
    return !isReadOnly(column);
  }

  /**
    * Indicates whether a write on the designated column will definitely succeed.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return <code>true</code> if so; <code>false</code> otherwise
    */
  bool MariaDbResultSetMetaData::isDefinitelyWritable(uint32_t column)
  {
    return !isReadOnly(column);
  }

  /**
    * Returns the fully-qualified name of the Java class whose instances are manufactured if the
    * method <code>ResultSet.getObject</code> is called to retrieve a value from the column. <code>
    * ResultSet.getObject</code> may return a subclass of the class returned by this method.
    *
    * @param column the first column is 1, the second is 2, ...
    * @return the fully-qualified name of the class in the Java programming language that would be
    *     used by the method <code>ResultSet.getObject</code> to retrieve the value in the specified
    *     column. This is the class name used for custom mapping.
    * @throws SQLException if a database access error occurs
    */
  SQLString MariaDbResultSetMetaData::getColumnClassName(uint32_t column)
  {
    const ColumnDefinition& ci= getColumnDefinition(column);
    const ColumnType& type= ci.getColumnType();
    return ColumnType::getClassName(type, static_cast<int32_t>(ci.getLength()), ci.isSigned(), ci.isBinary(), options);
  }

  const ColumnDefinition& MariaDbResultSetMetaData::getColumnDefinition(uint32_t column)
  {
    if (column >=1 &&column <= fieldPackets.size()) {
      return *fieldPackets[column -1];
    }
    throw InvalidArgumentException("No such column", "42000");//*ExceptionFactory::INSTANCE.create("No such column");
  }


  bool MariaDbResultSetMetaData::isZerofill(uint32_t column)
  {
    return getColumnDefinition(column).isZeroFill();
  }


  SQLString MariaDbResultSetMetaData::getColumnCollation(uint32_t column)
  {
    return getColumnDefinition(column).getCollation();
  }
  /**
    * Returns true if this either implements the interface argument or is directly or indirectly a
    * wrapper for an object that does. Returns false otherwise. If this implements the interface then
    * return true, else if this is a wrapper then return the result of recursively calling <code>
    * isWrapperFor</code> on the wrapped object. If this does not implement the interface and is not
    * a wrapper, return false. This method should be implemented as a low-cost operation compared to
    * <code>unwrap</code> so that callers can use this method to avoid expensive <code>unwrap</code>
    * calls that may fail. If this method returns true then calling <code>unwrap</code> with the same
    * argument should succeed.
    *
    * @param iface a Class defining an interface.
    * @return true if this implements the interface or directly or indirectly wraps an object that
    *     does.
    * @throws SQLException if an error occurs while determining whether this is a wrapper for an
    *     object with the given interface.
    */
  /*bool MariaDbResultSetMetaData::isWrapperFor(ResultSetMetaData *iface)
  {
    return INSTANCEOF(this, decltype(this));
  }*/
}
}
