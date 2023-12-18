/************************************************************************************
   Copyright (C) 2020,2022 MariaDB Corporation AB

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


#include "DoubleParameter.h"
#include <iomanip>

namespace sql
{
namespace mariadb
{

  DoubleParameter::DoubleParameter(long double value)
    : value(value)
  {
  }


  void DoubleParameter::writeTo(SQLString& str)
  {
    //std::to_string is not precise enough. at least on windows it does just sprintf("%f")
    std::ostringstream doubleAsString("");
    doubleAsString << std::scientific << std::setprecision(30) << value;
    str.append(doubleAsString.str().c_str());
  }


  void DoubleParameter:: writeTo(PacketOutputStream& pos)
  {
    pos.write(std::to_string(value).c_str());
  }

  int64_t DoubleParameter::getApproximateTextProtocolLength()
  {
    return std::to_string(value).length();
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void DoubleParameter::writeBinary(PacketOutputStream& pos)
  {
    /* Not sure if this is right, but that is needed for protocol, for C API will be fine :) */
    const int64_t *asI64= reinterpret_cast<const int64_t*>(&value);
    pos.writeLong(*asI64);
  }

  uint32_t DoubleParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for int value");
    }
    long double* buf= reinterpret_cast<long double*>(buffer.arr);
    *buf= value;
    return getValueBinLen();
  }

  const ColumnType& DoubleParameter::getColumnType() const
  {
    return ColumnType::DOUBLE;
  }

  SQLString DoubleParameter::toString()
  {
    return std::to_string(value);
  }

  bool DoubleParameter::isNullData() const
  {
    return false;
  }

  bool DoubleParameter::isLongData()
  {
    return false;
  }
}
}
