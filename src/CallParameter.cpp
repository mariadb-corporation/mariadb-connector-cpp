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


#include "CallParameter.h"
#include "ColumnType.h"

namespace sql
{
namespace mariadb
{

  CallParameter::CallParameter()
    : sqlType(Types::OTHER),
      outputSqlType(Types::OTHER),
      isInput_(true),
      isOutput_(false)
  {
  }

  bool CallParameter::isInput() const
  {
    return isInput_;
  }

  void CallParameter::setInput(bool input)
  {
    isInput_= input;
  }

  bool CallParameter::isOutput() const
  {
    return isOutput_;
  }

  void CallParameter::setOutput(bool output)
  {
    isOutput_= output;
  }

  int32_t CallParameter::getSqlType() const
  {
    return sqlType;
  }

  void CallParameter::setSqlType(int32_t sqlType)
  {
    this->sqlType= sqlType;
  }

  int32_t CallParameter::getOutputSqlType() const
  {
    return outputSqlType;
  }

  void CallParameter::setOutputSqlType(int32_t outputSqlType)
  {
    this->outputSqlType= outputSqlType;
  }

  int32_t CallParameter::getScale() const
  {
    return scale;
  }

  void CallParameter::setScale(int32_t scale)
  {
    this->scale= scale;
  }

  const SQLString& CallParameter::getTypeName() const
  {
    return typeName;
  }

  void CallParameter::setTypeName(const SQLString& typeName)
  {
    this->typeName= typeName;
  }

  bool CallParameter::isSigned() const
  {
    return isSigned_;
  }

  void CallParameter::setSigned(bool _signed)
  {
    isSigned_= _signed;
  }

  int32_t CallParameter::getCanBeNull() const
  {
    return canBeNull;
  }

  void CallParameter::setCanBeNull(int32_t canBeNull)
  {
    this->canBeNull= canBeNull;
  }

  int32_t CallParameter::getPrecision() const
  {
    return precision;
  }

  void CallParameter::setPrecision(int32_t precision)
  {
    this->precision= precision;
  }

  const SQLString& CallParameter::getClassName() const
  {
    return className;
  }

  void CallParameter::setClassName(const SQLString& className)
  {
    this->className= className;
  }

  const SQLString& CallParameter::getName() const
  {
    return name;
  }

  void CallParameter::setName(const SQLString& name)
  {
    this->name= name;
  }
}
}
