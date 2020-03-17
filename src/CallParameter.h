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


#ifndef _CALLPARAMETER_H_
#define _CALLPARAMETER_H_

#include "Consts.h"

namespace sql
{
namespace mariadb
{

class CallParameter  {

  bool isInput_;
  bool isOutput_;
  int32_t sqlType;
  int32_t outputSqlType;
  int32_t scale;
  SQLString typeName;
  bool isSigned_;
  int32_t canBeNull;
  int32_t precision;
  SQLString className;
  SQLString name;

public:
  CallParameter();
  bool isInput() const;
  void setInput(bool input);
  bool isOutput() const;
  void setOutput(bool output);
  int32_t getSqlType() const;
  void setSqlType(int32_t sqlType);
  int32_t getOutputSqlType() const;
  void setOutputSqlType(int32_t outputSqlType);
  int32_t getScale() const;
  void setScale(int32_t scale);
  const SQLString& getTypeName() const;
  void setTypeName(const SQLString& typeName);
  bool isSigned() const;
  void setSigned(bool _signed);
  int32_t getCanBeNull() const;
  void setCanBeNull(int32_t canBeNull);
  int32_t getPrecision() const;
  void setPrecision(int32_t precision);
  const SQLString& getClassName() const;
  void setClassName(const SQLString& className);
  const SQLString& getName() const;
  void setName(const SQLString& name);
  };
}
}
#endif