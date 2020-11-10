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


#include "ByteParameter.h"

namespace sql
{
namespace mariadb
{

  const std::string ByteParameter::hexArray("0123456789ABCDEF");

  ByteParameter::ByteParameter(char value)
    : value(value & 0xFF)
  {
  }

  /**
    * Write Byte value to stream using TEXT protocol.
    *
    * @param os the stream to write to
    * @throws IOException if any socket error occur
    */
  void ByteParameter::writeTo(PacketOutputStream& os)
  {
    os.write("0x");
    os.write(hexArray[value >> 4]);
    os.write(hexArray[value &0x0F]);
  }

  void ByteParameter::writeTo(SQLString& os)
  {
    os.append("0x");
    os.append(hexArray[value >> 4]);
    os.append(hexArray[value &0x0F]);
  }

  int64_t ByteParameter::getApproximateTextProtocolLength()
  {
    return 4;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void ByteParameter::writeBinary(PacketOutputStream& pos)
  {
    pos.write(value);
  }

  uint32_t ByteParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for int value");
    }
    *buffer= value;
    return getValueBinLen();
  }

  const ColumnType& ByteParameter::getColumnType() const
  {
    return ColumnType::TINYINT;
  }

  SQLString ByteParameter::toString()
  {
    return SQLString("0x").append(hexArray[value & 0xF0]).append(hexArray[value & 0x0F]);
  }

  bool ByteParameter::isNullData() const
  {
    return false;
  }

  bool ByteParameter::isLongData()
  {
    return false;
  }
}
}
