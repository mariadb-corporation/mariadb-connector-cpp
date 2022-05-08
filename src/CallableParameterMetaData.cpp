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


#include "CallableParameterMetaData.h"

#include "ColumnType.h"
#include "MariaDbConnection.h"

namespace sql
{
namespace mariadb
{
  /**
    * Retrieve Callable metaData.
    *
    * @param con connection
    * @param database database name
    * @param name procedure/function name
    * @param isFunction is it a function
    */
  CallableParameterMetaData::CallableParameterMetaData(ResultSet* _rs, bool _isFunction)
    : rs(_rs)
    , isFunction(_isFunction)
  {
    uint32_t count= 0;
    while (rs->next()) ++count;
    this->parameterCount= count;
  }


  uint32_t CallableParameterMetaData::getParameterCount()
  {
    return parameterCount;
  }

  int32_t CallableParameterMetaData::isNullable(uint32_t index)
  {
    setIndex(index);
    return ParameterMetaData::parameterNullableUnknown;
  }

  void CallableParameterMetaData::setIndex(uint32_t index)
  {
    if (index < 1 || index > parameterCount) {
      throw SQLException("invalid parameter index " + index);
    }
    rs->absolute(index);
  }
  bool CallableParameterMetaData::isSigned(uint32_t index)
  {
    setIndex(index);
    SQLString paramDetail(rs->getString("DTD_IDENTIFIER"));
    return StringImp::get(paramDetail).find(" unsigned") == std::string::npos;
  }

  int32_t CallableParameterMetaData::getPrecision(uint32_t index)
  {
    setIndex(index);
    int32_t characterMaxLength= rs->getInt("CHARACTER_MAXIMUM_LENGTH");
    int32_t numericPrecision= rs->getInt("NUMERIC_PRECISION");
    return (numericPrecision > 0) ? numericPrecision : characterMaxLength;
  }

  int32_t CallableParameterMetaData::getScale(uint32_t index)
  {
    setIndex(index);
    return rs->getInt("NUMERIC_SCALE");
  }


  SQLString CallableParameterMetaData::getParameterName(int32_t index)
  {
    setIndex(index);
    return rs->getString("PARAMETER_NAME");
  }


  int32_t CallableParameterMetaData::getParameterType(uint32_t index)
  {
    setIndex(index);
    SQLString str = rs->getString("DATA_TYPE").toUpperCase();
    if (str.compare("BIT") == 0) {
      return Types::BIT;
    }
    else if (str.compare("TINYINT") == 0) {
      return Types::TINYINT;
    }
    else if (str.compare("SMALLINT") == 0 || str.compare("YEAR") == 0) {
      return Types::SMALLINT;
    }
    else if (str.compare("MEDIUMINT") == 0 ||
            str.compare("INT") == 0 ||
            str.compare("INT24") == 0 ||
            str.compare("INTEGER") == 0) {
      return Types::INTEGER;
    }
    else if (str.compare("LONG") == 0 || str.compare("BIGINT") == 0) {
      return Types::BIGINT;
    }
    else if (str.compare("REAL") == 0 || str.compare("DOUBLE") == 0) {
      return Types::DOUBLE;
    }
    else if (str.compare("FLOAT") == 0) {
      return Types::FLOAT;
    }
    else if (str.compare("DECIMAL") == 0) {
      return Types::DECIMAL;
    }
    else if (str.compare("CHAR") == 0) {
      return Types::CHAR;
    }
    else if (str.compare("VARCHAR") == 0 || str.compare("ENUM") == 0 || str.compare("TINYTEXT") == 0 || str.compare("SET") == 0) {
      return Types::VARCHAR;
    }
    else if (str.compare("DATE") == 0) {
      return Types::DATE;
    }
    else if (str.compare("TIME") == 0) {
      return Types::TIME;
    }
    else if (str.compare("TIMESTAMP") == 0 || str.compare("DATETIME") == 0) {
      return Types::TIMESTAMP;
    }
    else if (str.compare("BINARY") == 0) {
      return Types::BINARY;
    }
    else if (str.compare("VARBINARY") == 0) {
      return Types::VARBINARY;
    }
    else if (str.compare("TINYBLOB") == 0 || str.compare("BLOB") == 0 || str.compare("MEDIUMBLOB") == 0 ||
      str.compare("LONGBLOB") == 0 || str.compare("GEOMETRY") == 0) {
      return Types::BLOB;
    }
    else if (str.compare("TEXT") == 0 || str.compare("MEDIUMTEXT") == 0 || str.compare("LONGTEXT") == 0) {
      return Types::CLOB;
    }
    return Types::OTHER;
  }


  SQLString CallableParameterMetaData::getParameterTypeName(uint32_t index)
  {
    setIndex(index);
    return rs->getString("DATA_TYPE").toUpperCase();
  }


  SQLString CallableParameterMetaData::getParameterClassName(uint32_t index)
  {
    return emptyStr;
  }

  /**
    * Get mode info.
    *
    * <ul>
    *   <li>0 : unknown
    *   <li>1 : IN
    *   <li>2 : INOUT
    *   <li>4 : OUT
    * </ul>
    *
    * @param param parameter index
    * @return mode information
    * @throws SQLException if index is wrong
    */
  int32_t CallableParameterMetaData::getParameterMode(uint32_t index)
  {
    setIndex(index);
    if (isFunction)return ParameterMetaData::parameterModeOut;
    SQLString str = rs->getString("PARAMETER_MODE");
    if (str.compare("IN") == 0) {
      return ParameterMetaData::parameterModeIn;
    }
    else if (str.compare("OUT") == 0) {
      return ParameterMetaData::parameterModeOut;
    }
    else if (str.compare("INOUT") == 0) {
      return ParameterMetaData::parameterModeInOut;
    }
    return ParameterMetaData::parameterModeUnknown;
  }

}
}
