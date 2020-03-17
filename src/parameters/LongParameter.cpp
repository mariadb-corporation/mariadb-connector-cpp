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


#include "LongParameter.h"

namespace sql
{
namespace mariadb
{

  LongParameter::LongParameter(int64_t value)
    : value(value)
  {
  }


  void LongParameter::writeTo(SQLString& str)
  {
    str.append(std::to_string(value));
  }


  void LongParameter::writeTo(PacketOutputStream& os)
  {
    os.write(std::to_string(value).c_str());
  }

  int64_t LongParameter::getApproximateTextProtocolLength()
  {
    return std::to_string(value).length();
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void LongParameter::writeBinary(PacketOutputStream& pos)
  {
    pos.writeLong(value);
  }

  uint32_t LongParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for int value");
    }
    int64_t* buf= reinterpret_cast<int64_t*>(buffer.arr);
    *buf= value;
    return getValueBinLen();
  }

  const ColumnType& LongParameter::getColumnType() const
  {
    return ColumnType::BIGINT;
  }

  SQLString LongParameter::toString()
  {
    return std::to_string(value);
  }

  bool LongParameter::isNullData() const
  {
    return false;
  }

  bool LongParameter::isLongData()
  {
    return false;
  }
}
}
