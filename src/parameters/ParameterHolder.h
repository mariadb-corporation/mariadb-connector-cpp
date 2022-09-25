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


#ifndef _PARAMETERHOLDER_H_
#define _PARAMETERHOLDER_H_

#include "io/PacketOutputStream.h"
#include "ColumnType.h"

namespace sql
{
namespace mariadb
{

class ParameterHolder
{
protected:
  static char BINARY_INTRODUCER[];
  static char QUOTE;
  ParameterHolder()= default;
public:
  virtual ~ParameterHolder();

  virtual void writeTo(PacketOutputStream& os)=0;
  virtual void writeTo(SQLString& str)=0;
  virtual void writeBinary(PacketOutputStream& pos)=0;
  virtual uint32_t writeBinary(sql::bytes& buffer)=0;
  virtual int64_t getApproximateTextProtocolLength()=0;
  virtual SQLString toString()=0;
  virtual bool isNullData() const=0;
  virtual const ColumnType& getColumnType() const=0;
  virtual bool isLongData()=0;
  virtual void* getValuePtr()=0;
  virtual unsigned long getValueBinLen() const=0;
  virtual bool isUnsigned() const { return false; }
};
}
}
#endif
