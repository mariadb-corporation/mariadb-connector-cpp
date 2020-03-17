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


#include "ShortParameter.h"

namespace sql
{
namespace mariadb
{

  ShortParameter::ShortParameter(int16_t value)
    : value(value)
  {
  }


  void ShortParameter::writeTo(SQLString& str)
  {
    str.append(std::to_string(value));
  }


  void ShortParameter::writeTo(PacketOutputStream& pos)
  {
    pos.write(std::to_string(value).c_str());
  }

  int64_t ShortParameter::getApproximateTextProtocolLength()
  {
    return std::to_string(value).length();
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void ShortParameter::writeBinary(PacketOutputStream& pos)
  {
    pos.writeShort(value);
  }

  uint32_t ShortParameter::writeBinary(sql::bytes & buffer)
  {
    int16_t* shortBuff= (int16_t*)(buffer.arr);
    *shortBuff= value;
    return getValueBinLen();
  }

  const ColumnType& ShortParameter::getColumnType() const
  {
    return ColumnType::SMALLINT;
  }

  SQLString ShortParameter::toString()
  {
    return std::to_string(value);
  }

  bool ShortParameter::isNullData() const
  {
    return false;
  }

  bool ShortParameter::isLongData()
  {
    return false;
  }
}
}
