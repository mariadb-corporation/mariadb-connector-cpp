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


#include "ColumnType.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"
}
  const ColumnType ColumnType::OLDDECIMAL(0, Types::DECIMAL, "Types::DECIMAL", "BigDecimal", 0);
  const ColumnType ColumnType::TINYINT(1, Types::SMALLINT, "Types::SMALLINT", "int32_t", 1);
  const ColumnType ColumnType::SMALLINT(2, Types::SMALLINT, "Types::SMALLINT", "int32_t", 2);
  const ColumnType ColumnType::INTEGER(3, Types::INTEGER, "Types::INTEGER", "int32_t", 4);
  const ColumnType ColumnType::FLOAT(4, Types::REAL, "Types::REAL", "float", 4);
  const ColumnType ColumnType::DOUBLE(5, Types::DOUBLE, "Types::DOUBLE", "long double", 8);
  const ColumnType ColumnType::_NULL(6, Types::_NULL, "Types::NULL", "SQLString", 0);
  const ColumnType ColumnType::TIMESTAMP(7, Types::TIMESTAMP, "Types::TIMESTAMP", "Timestamp", sizeof(capi::MYSQL_TIME));
  const ColumnType ColumnType::BIGINT(8, Types::BIGINT, "Types::BIGINT", "int64_t", 8);
  const ColumnType ColumnType::MEDIUMINT(9, Types::INTEGER, "Types::INTEGER", "int32_t", 4);
  const ColumnType ColumnType::DATE(10, Types::DATE, "Types::DATE", "Date", sizeof(capi::MYSQL_TIME));
  const ColumnType ColumnType::TIME(11, Types::TIME, "Types::TIME", "Time", sizeof(capi::MYSQL_TIME));
  const ColumnType ColumnType::DATETIME(12, Types::TIMESTAMP, "Types::TIMESTAMP", "Timestamp", sizeof(capi::MYSQL_TIME));
  const ColumnType ColumnType::YEAR(13, Types::SMALLINT, "Types::SMALLINT", "int16_t", 2);
  const ColumnType ColumnType::NEWDATE(14, Types::DATE, "Types::DATE", "Date", sizeof(capi::MYSQL_TIME));
  const ColumnType ColumnType::VARCHAR(15, Types::VARCHAR, "Types::VARCHAR", "SQLString", 0);
  const ColumnType ColumnType::BIT(16, Types::BIT, "Types::BIT", "[B", 0);
  const ColumnType ColumnType::JSON(245, Types::VARCHAR, "Types::VARCHAR", "SQLString", 0);
  const ColumnType ColumnType::DECIMAL(246, Types::DECIMAL, "Types::DECIMAL", "BigDecimal", 0);
  const ColumnType ColumnType::ENUM(247, Types::VARCHAR, "Types::VARCHAR", "SQLString", 0);
  const ColumnType ColumnType::SET(248, Types::VARCHAR, "Types::VARCHAR", "SQLString", 0);
  const ColumnType ColumnType::TINYBLOB(249, Types::VARBINARY, "Types::VARBINARY", "[B", 0);
  const ColumnType ColumnType::MEDIUMBLOB(250, Types::VARBINARY, "Types::VARBINARY", "[B", 0);
  const ColumnType ColumnType::LONGBLOB(251, Types::LONGVARBINARY, "Types::LONGVARBINARY", "[B", 0);
  const ColumnType ColumnType::BLOB(252, Types::LONGVARBINARY, "Types::LONGVARBINARY", "[B", 0);
  const ColumnType ColumnType::VARSTRING(253, Types::VARCHAR, "Types::VARCHAR", "SQLString", 0);
  const ColumnType ColumnType::STRING(254, Types::VARCHAR, "Types::VARCHAR", "SQLString", 0);
  const ColumnType ColumnType::GEOMETRY(255, Types::VARBINARY, "Types::VARBINARY", "[B", 0);

  const std::map<int32_t, const ColumnType&> ColumnType::typeMap= {
    {ColumnType::OLDDECIMAL.mariadbType, ColumnType::OLDDECIMAL},
    {ColumnType::TINYINT.mariadbType, ColumnType::TINYINT},
    {ColumnType::SMALLINT.mariadbType, ColumnType::SMALLINT},
    {ColumnType::INTEGER.mariadbType, ColumnType::INTEGER},
    {ColumnType::FLOAT.mariadbType, ColumnType::FLOAT},
    {ColumnType::DOUBLE.mariadbType, ColumnType::DOUBLE},
    {ColumnType::_NULL.mariadbType, ColumnType::_NULL},
    {ColumnType::TIMESTAMP.mariadbType, ColumnType::TIMESTAMP},
    {ColumnType::BIGINT.mariadbType, ColumnType::BIGINT},
    {ColumnType::MEDIUMINT.mariadbType, ColumnType::MEDIUMINT},
    {ColumnType::DATE.mariadbType, ColumnType::DATE},
    {ColumnType::TIME.mariadbType, ColumnType::TIME},
    {ColumnType::DATETIME.mariadbType, ColumnType::DATETIME},
    {ColumnType::YEAR.mariadbType, ColumnType::YEAR},
    {ColumnType::NEWDATE.mariadbType, ColumnType::NEWDATE},
    {ColumnType::VARCHAR.mariadbType, ColumnType::VARCHAR},
    {ColumnType::BIT.mariadbType, ColumnType::BIT},
    {ColumnType::JSON.mariadbType, ColumnType::JSON},
    {ColumnType::DECIMAL.mariadbType, ColumnType::DECIMAL},
    {ColumnType::ENUM.mariadbType, ColumnType::ENUM},
    {ColumnType::SET.mariadbType, ColumnType::SET},
    {ColumnType::TINYBLOB.mariadbType, ColumnType::TINYBLOB},
    {ColumnType::MEDIUMBLOB.mariadbType, ColumnType::MEDIUMBLOB},
    {ColumnType::LONGBLOB.mariadbType, ColumnType::LONGBLOB},
    {ColumnType::BLOB.mariadbType, ColumnType::BLOB},
    {ColumnType::VARSTRING.mariadbType, ColumnType::VARSTRING},
    {ColumnType::STRING.mariadbType, ColumnType::STRING},
    {ColumnType::GEOMETRY.mariadbType, ColumnType::GEOMETRY}
  };

  ColumnType::ColumnType(int32_t _mariadbType, int32_t _javaType, const SQLString& _javaTypeName, const SQLString& _className, size_t binBindTypeSize) :
    mariadbType(static_cast<int16_t>(_mariadbType)),
    javaType(_javaType),
    javaTypeName(_javaTypeName),
    className(_className),
    binSize(binBindTypeSize)
  {
  }

  /**
    * Permit to know java result class according to java.sql.Types::
    *
    * @param type java.sql.Type value
    * @return Class name.
    */
  const std::type_info& ColumnType::classFromJavaType(int32_t type)
  {
    switch (type) {
    case Types::BOOLEAN:
    case Types::BIT:
      return typeid(bool);

    case Types::TINYINT:
      return typeid(char);

    case Types::SMALLINT:
      return typeid(int16_t);

    case Types::INTEGER:
      return typeid(int32_t);

    case Types::BIGINT:
      return typeid(int64_t);

    case Types::DOUBLE:
    case Types::FLOAT:
      return typeid(long double);

    case Types::REAL:
      return typeid(float);

    case Types::TIMESTAMP:
      return typeid(Timestamp);

    case Types::DATE:
      return typeid(Date);

    case Types::VARCHAR:
    case Types::NVARCHAR:
    case Types::CHAR:
    case Types::NCHAR:
    case Types::LONGVARCHAR:
    case Types::LONGNVARCHAR:
    case Types::CLOB:
    case Types::NCLOB:
      return typeid(std::string); //SQLString may be missing some stuff we might need. it can always be casted to

    case Types::DECIMAL:
    case Types::NUMERIC:
      return typeid(BigDecimal);

    case Types::VARBINARY:
    case Types::BINARY:
    case Types::LONGVARBINARY:
    case Types::BLOB:
    case Types::JAVA_OBJECT:
      return typeid(std::string);

    case Types::_NULL:
      return typeid(bool); // Will figure out what to do here

    case Types::TIME:
      return typeid(Time);

    default:
      break;
    }
    return typeid(std::string);/*NULL*/ //yeah...;
  }

  /**
    * Is type numeric.
    *
    * @param type mariadb type
    * @return true if type is numeric
    */
  bool ColumnType::isNumeric(const ColumnType& type)
  {
    switch (type.javaType) {
    case Types::DECIMAL:
    case Types::TINYINT:
    case Types::SMALLINT:
    case Types::INTEGER:
    case Types::FLOAT:
    case Types::DOUBLE:
    case Types::BIGINT:
    //case Types::MEDIUMINT:
    case Types::BIT:
    //case Types::DECIMAL:
      return type != ColumnType::YEAR /*YEAR - TODO: change to get rid off the constant */;
    default:
      return false;
    }
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
  SQLString ColumnType::getColumnTypeName(const ColumnType& type, int64_t len, int64_t charLen, bool _signed, bool binary)
  {
    if (type == SMALLINT || type == MEDIUMINT || type == INTEGER || type == BIGINT)
    {
      if (!_signed) {
        return type.getTypeName() + " UNSIGNED";
      }
      else {
        return type.getTypeName();
      }
    }
    else if (type == BLOB)
    {
      /*
      map to different blob types based on datatype length
      see https://mariadb.com/kb/en/library/data-types/
      */
      if (len > INT32_MAX) {
        return "LONGBLOB";
      }
      else if (len <= 255) {
        return "TINYBLOB";
      }
      else if (len <= 65535) {
        return "BLOB";
      }
      else if (len <= 16777215) {
        return "MEDIUMBLOB";
      }
      else {
        return "LONGBLOB";
      }
    }
    else if (type == VARSTRING || type == VARCHAR)
    {
      if (binary) {
        return "VARBINARY";
      }

      if (len > INT32_MAX) {
        return "LONGTEXT";
      }
      else if (charLen <= 65532) {
        return "VARCHAR";
      }
      else if (charLen <= 65535) {
        return "TEXT";
      }
      else if (charLen <= 16777215) {
        return "MEDIUMTEXT";
      }
      else {
        return "LONGTEXT";
      }
    }
    else if (type == STRING)
    {
      if (binary) {
        return "BINARY";
      }
      return "CHAR";
    }
    else
    {
      return type.getTypeName();
    }
  }

  /**
    * Convert server Type to server type.
    *
    * @param typeValue type value
    * @param charsetNumber charset
    * @return MariaDb type
    */
  const ColumnType& ColumnType::fromServer(int32_t typeValue, int32_t charsetNumber)
  {
    const auto cit= typeMap.find(typeValue);
    const ColumnType& columnType= cit != typeMap.end() ? cit->second : BLOB;

    if (charsetNumber != 63 && typeValue >= 249 && typeValue <= 252) {

      return ColumnType::VARCHAR;
    }

    return columnType;
  }

  /**
    * Convert javatype to ColumnType::
    *
    * @param javaType javatype value
    * @return mariaDb type value
    */
  const ColumnType& ColumnType::toServer(int32_t javaType)
  {
    for (auto& cit : typeMap) {
      if (cit.second.javaType == javaType) {
        return cit.second;
      }
    }
    return ColumnType::BLOB;
  }

  /**
    * Get class name.
    *
    * @param type type
    * @param len len
    * @param _signed _signed
    * @param binary binary
    * @param options options
    * @return class name
    */
  /* Not sure if there is any sense for us to have class names here */
  SQLString ColumnType::getClassName(const ColumnType& type, int32_t len, bool _signed, bool binary, Shared::Options options)
  {
    if (type == TINYINT)
    {
      if (len == 1 && options->tinyInt1isBit) {
        return "bool";
      }
      return "int32_t";
    }
    if (type == INTEGER)
    {
      return (_signed) ? "int32_t" :"int64_t";
    }
    if (type == BIGINT)
    {
      return (_signed) ? "int64_t" : "uint64_t";
    }
    if (type ==  YEAR)
    {
      if (options->yearIsDateType) {
        return "Date";
      }
      return "int16_t";
    }
    if (type ==  BIT)
    {
      return (len == 1) ? "bool" :"[B";
    }
    if (type ==  STRING || type ==  VARCHAR || type ==  VARSTRING)
    {
      return binary ? "[B" : "SQLString";
    }

    return type.getClassName();
  }

  const SQLString& ColumnType::getClassName() const  {
    return className;
  }

  int32_t ColumnType::getSqlType() const {
    return javaType;
  }

  const SQLString& ColumnType::getTypeName() const {
    return javaTypeName;
  }

  int16_t ColumnType::getType() const {
    return mariadbType;
  }

  const SQLString& ColumnType::getCppTypeName() const {
    return javaTypeName;
  }


  bool operator==(const ColumnType& type1, const ColumnType& type2)
  {
    return type1.getType() ==  type2.getType();
  }
  bool operator!=(const ColumnType& type1, const ColumnType& type2)
  {
    return type1.getType() !=  type2.getType();
  }
}
}