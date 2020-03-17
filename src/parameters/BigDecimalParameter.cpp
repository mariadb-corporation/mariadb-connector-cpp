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


#include "BigDecimalParameter.h"

namespace sql
{
namespace mariadb
{

  BigDecimalParameter::BigDecimalParameter(const BigDecimal& _bigDecimal)
    : bigDecimal(_bigDecimal)
  {
  }

  void BigDecimalParameter:: writeTo(PacketOutputStream& pos)
  {
    pos.write(bigDecimal); /*toPlainString().getBytes());*/
  }


  void BigDecimalParameter::writeTo(SQLString& str)
  {
    str.append(bigDecimal);
  }

  int64_t BigDecimalParameter::getApproximateTextProtocolLength()
  {
    return bigDecimal.length();
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void BigDecimalParameter::writeBinary(PacketOutputStream& pos)
  {
    SQLString value(bigDecimal);
    pos.writeFieldLength(value.size());
    pos.write(value);
  }

  uint32_t BigDecimalParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for string value");
    }
    std::memcpy(buffer.arr, bigDecimal.c_str(), getValueBinLen());
    return getValueBinLen();
  }

  const ColumnType& BigDecimalParameter::getColumnType() const
  {
    return ColumnType::DECIMAL;
  }

  SQLString BigDecimalParameter::toString()
  {
    return bigDecimal/*.toString()*/;
  }

  bool BigDecimalParameter::isNullData() const
  {
    return false;
  }

  bool BigDecimalParameter::isLongData()
  {
    return false;
  }

  void* BigDecimalParameter::getValuePtr()
  {
    return const_cast<void*>(static_cast<const void*>(bigDecimal.c_str()));
  }
  unsigned long BigDecimalParameter::getValueBinLen() const
  {
    return static_cast<unsigned long>(bigDecimal.length());
  }
}
}
