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

#ifndef _SIMPLEPARAMETERMETADATA_H_
#define _SIMPLEPARAMETERMETADATA_H_

#include "Consts.h"

namespace sql
{
namespace mariadb
{

class SimpleParameterMetaData  : public ParameterMetaData
{
  uint32_t parameterCount;
  void validateParameter(uint32_t param);

public:
  SimpleParameterMetaData(uint32_t parameterCount);
  uint32_t getParameterCount();
  int32_t isNullable(uint32_t param);
  bool isSigned(uint32_t param);
  int32_t getPrecision(uint32_t param);
  int32_t getScale(uint32_t param);
  int32_t getParameterType(uint32_t param);
  SQLString getParameterTypeName(uint32_t param);
  SQLString getParameterClassName(uint32_t param);
  int32_t getParameterMode(uint32_t param);
  };
}
}
#endif
