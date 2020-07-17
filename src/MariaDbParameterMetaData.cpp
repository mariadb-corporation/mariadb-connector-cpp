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


#include "MariaDbParameterMetaData.h"

#include "ExceptionFactory.h"

namespace sql
{
namespace mariadb
{

  MariaDbParameterMetaData::MariaDbParameterMetaData(const std::vector<Shared::ColumnDefinition>& _parametersInformation)
    : parametersInformation(_parametersInformation)
  {
  }

  void MariaDbParameterMetaData::checkAvailable()
  {
    if (this->parametersInformation.empty()) {
      throw SQLException("Parameter metadata not available for these statement", "S1C00");
    }
  }

  uint32_t MariaDbParameterMetaData::getParameterCount()
  {
    return static_cast<uint32_t>(parametersInformation.size());
  }

  const ColumnDefinition& MariaDbParameterMetaData::getParameterInformation(uint32_t param)
  {
    checkAvailable();
    if (param >= 1 && param <= parametersInformation.size()) {
      return *parametersInformation[param - 1];
    }
    throw SQLException(
      ("Parameter metadata out of range : param was "
      + std::to_string(param)
      +" and must be 1 <= param <="
      + std::to_string(parametersInformation.size())).c_str(),
      "07009");
  }

  int32_t MariaDbParameterMetaData::isNullable(uint32_t param)
  {
    if (getParameterInformation(param).isNotNull()) {
      return ParameterMetaData::parameterNoNulls;
    }
    else {
      return ParameterMetaData::parameterNullable;
    }
  }

  bool MariaDbParameterMetaData::isSigned(uint32_t param)
  {
    return getParameterInformation(param).isSigned();
  }

  int32_t MariaDbParameterMetaData::getPrecision(uint32_t param)
  {
    int64_t length= getParameterInformation(param).getLength();
    return (length > INT32_MAX ? INT32_MAX : static_cast<int32_t>(length));
  }

  int32_t MariaDbParameterMetaData::getScale(uint32_t param)
  {
    if (ColumnType::isNumeric(getParameterInformation(param).getColumnType())) {
      return getParameterInformation(param).getDecimals();
    }
    return 0;
  }

  int32_t MariaDbParameterMetaData::getParameterType(uint32_t param)
  {
    throw ExceptionFactory::INSTANCE.notSupported(
      "Getting parameter type metadata are not supported");
  }

  SQLString MariaDbParameterMetaData::getParameterTypeName(uint32_t param)
  {
    return getParameterInformation(param).getColumnType().getTypeName();
  }

  SQLString MariaDbParameterMetaData::getParameterClassName(uint32_t param)
  {
    return getParameterInformation(param).getColumnType().getClassName();
  }

  int32_t MariaDbParameterMetaData::getParameterMode(uint32_t param)
  {
    return parameterModeIn;
  }

  bool MariaDbParameterMetaData::isWrapperFor(ParameterMetaData* iface)
  {
    return INSTANCEOF(iface, decltype(this));
  }
}
}
