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


#include "FloatParameter.h"

namespace sql
{
namespace mariadb
{

  FloatParameter::FloatParameter(float value)
    : value(value)
  {
  }

  void FloatParameter::writeTo(SQLString& str)
  {
    str.append(std::to_string(value));
  }

  void FloatParameter:: writeTo(PacketOutputStream& os)
  {
    os.write(std::to_string(value).c_str());
  }

  int64_t FloatParameter::getApproximateTextProtocolLength()
  {
    return std::to_string(value).length();
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void FloatParameter::writeBinary(PacketOutputStream& pos)
  {
    const int32_t *asI32= reinterpret_cast<const int32_t*>(&value);
    pos.writeInt(*asI32);
  }

  uint32_t FloatParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for int value");
    }
    float* buf= reinterpret_cast<float*>(buffer.arr);
    *buf= value;
    return getValueBinLen();
  }

  const ColumnType& FloatParameter::getColumnType() const
  {
    return ColumnType::FLOAT;
  }

  SQLString FloatParameter::toString()
  {
    return std::to_string(value);
  }

  bool FloatParameter::isNullData() const
  {
    return false;
  }

  bool FloatParameter::isLongData()
  {
    return false;
  }
}
}
