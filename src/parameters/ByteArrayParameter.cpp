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


#include "ByteArrayParameter.h"

#include "util/Utils.h"

namespace sql
{
  namespace mariadb
{

  ByteArrayParameter::ByteArrayParameter(const sql::bytes& bytes, bool noBackslashEscapes)
    : bytes(bytes)
    , noBackslashEscapes(noBackslashEscapes)
  {
  }


  void ByteArrayParameter::writeTo(SQLString& str)
  {
    str.append(BINARY_INTRODUCER);
    Utils::escapeData(bytes.arr, static_cast<size_t>(bytes.length), noBackslashEscapes, str);
    str.append(QUOTE);
  }

  /**
    * Write data to socket in text format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void ByteArrayParameter::writeTo(PacketOutputStream& pos)
  {
    pos.write(BINARY_INTRODUCER);
    pos.writeBytesEscaped(bytes.arr, static_cast<int32_t>(bytes.length), noBackslashEscapes);
    pos.write(QUOTE);
  }

  int64_t ByteArrayParameter::getApproximateTextProtocolLength()
  {
    return bytes.length *2;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void ByteArrayParameter::writeBinary(PacketOutputStream& pos)
  {
    pos.writeFieldLength(bytes.length);
    pos.write(bytes.arr);
  }

    uint32_t ByteArrayParameter::writeBinary(sql::bytes & buf)
    {
      /*if (buf.size() < getValueBinLen())
      {
        throw SQLException("Parameter buffer size is too small for ByteArray value");
      }*/

      buf.wrap(bytes.arr, bytes.size());
      return getValueBinLen();
    }

  const ColumnType& ByteArrayParameter::getColumnType() const
  {
    return ColumnType::VARSTRING;
  }

  SQLString ByteArrayParameter::toString()
  {
    if (bytes.length >1024) {
      return "<bytearray:"+std::string(bytes.arr, 1024)+"...>";
    }
    else {
      return "<bytearray:"+ std::string(bytes.arr, static_cast<std::string::size_type>(bytes.length))+">";
    }
  }

  bool ByteArrayParameter::isNullData() const
  {
    return false;
  }

  bool ByteArrayParameter::isLongData()
  {
    return false;
  }
}
}
