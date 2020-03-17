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


#ifndef _STREAMPARAMETER_H_
#define _STREAMPARAMETER_H_

#include "Consts.h"

#include "ParameterHolder.h"

namespace sql
{
namespace mariadb
{
class StreamParameter  : public ParameterHolder {

  std::istream& is;
  const int64_t length;
  bool noBackslashEscapes;

public:
  StreamParameter(std::istream&is,int64_t length,bool noBackslashEscapes);
  StreamParameter(std::istream&is,bool noBackSlashEscapes);
  void writeTo(SQLString& str);
  void writeTo(PacketOutputStream& str);
  int64_t getApproximateTextProtocolLength();
  void writeBinary(PacketOutputStream& pos);
  uint32_t writeBinary(sql::bytes& buffer);
  SQLString toString();
  const ColumnType& getColumnType() const;
  bool isNullData() const;
  bool isLongData();
  void* getValuePtr() { return nullptr; }
  unsigned long getValueBinLen() const { return 0; }
  };
}
}
#endif