/************************************************************************************
   Copyright (C) 2021 MariaDB Corporation AB

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

#include "SimpleParameterMetaData.h"
#include "ExceptionFactory.h"

namespace sql
{
namespace mariadb
{
void SimpleParameterMetaData::validateParameter(uint32_t param)
{
  if (param < 1 || param > parameterCount) {
    std::ostringstream msg("Parameter metadata out of range : param was ", std::ios_base::ate);
    msg << param << " and must be in range 1 - " << parameterCount;
    ExceptionFactory::INSTANCE.create(msg.str(), "07009").Throw();
  }
}


SimpleParameterMetaData::SimpleParameterMetaData(uint32_t _parameterCount)
: parameterCount(_parameterCount)
{
}


uint32_t SimpleParameterMetaData::getParameterCount()
{
  return parameterCount;
}


int32_t SimpleParameterMetaData::isNullable(uint32_t param)
{
  validateParameter(param);
  return ParameterMetaData::parameterNullableUnknown;
}


bool SimpleParameterMetaData::isSigned(uint32_t param)
{
  validateParameter(param);
  return true;
}


int32_t SimpleParameterMetaData::getPrecision(uint32_t param)
{
  validateParameter(param);
  ExceptionFactory::INSTANCE.create("Unknown parameter metadata precision").Throw();
  return 0;
}


int32_t SimpleParameterMetaData::getScale(uint32_t param)
{
  validateParameter(param);
  ExceptionFactory::INSTANCE.create("Unknown parameter metadata scale").Throw();
  return 0;
}


int32_t SimpleParameterMetaData::getParameterType(uint32_t param)
{
  validateParameter(param);
  throw ExceptionFactory::INSTANCE.notSupported(
    "Getting parameter type metadata are not supported");
}


SQLString SimpleParameterMetaData::getParameterTypeName(uint32_t param)
{
  validateParameter(param);
  ExceptionFactory::INSTANCE.create("Unknown parameter metadata type name").Throw();
  return 0;
}


SQLString SimpleParameterMetaData::getParameterClassName(uint32_t param)
{
  validateParameter(param);
  ExceptionFactory::INSTANCE.create("Unknown parameter metadata class name").Throw();
  return 0;
}


int32_t SimpleParameterMetaData::getParameterMode(uint32_t param)
{
  return parameterModeIn;
}

} // namespace mariadb
} // namespace sql
