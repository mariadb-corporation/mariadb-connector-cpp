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


#include "DefaultParameter.h"

namespace sql
{
namespace mariadb
{
  const char * DefaultParameter::defaultBytes= "DEFAULT";
  static const int64_t defaultBytesLength= 7; /* or 8? */


  void DefaultParameter::writeTo(SQLString& str)
  {
    str.append(defaultBytes);
  }
  /**
    * Send escaped String to outputStream.
    *
    * @param pos outpustream.
    */
  void DefaultParameter:: writeTo(PacketOutputStream& pos)
  {
    pos.write(defaultBytes);
  }

  int64_t DefaultParameter::getApproximateTextProtocolLength()
  {
    return 7;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void DefaultParameter::writeBinary(PacketOutputStream& pos)
  {
    pos.writeFieldLength(defaultBytesLength);
    pos.write(defaultBytes);
  }

  const ColumnType& DefaultParameter::getColumnType() const
  {
    return ColumnType::VARCHAR;
  }

  SQLString DefaultParameter::toString()
  {
    return "DEFAULT";
  }

  bool DefaultParameter::isNullData() const
  {
    return false;
  }

  bool DefaultParameter::isLongData()
  {
    return false;
  }
}
}
