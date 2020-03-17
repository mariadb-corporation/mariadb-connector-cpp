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


#include "BooleanParameter.h"

namespace sql
{
namespace mariadb
{

  BooleanParameter::BooleanParameter(bool value)
    : value(value)
  {
  }

  void BooleanParameter:: writeTo(PacketOutputStream& os)
  {
    os.write(value ? '1' : '0');
  }

  //TODO probably they could be one method writing to ostream. Or create implementation of PacketOutputStream writing to string(stream)
  void BooleanParameter::writeTo(SQLString& str)
  {
    str.append(value ? '1' : '0');
  }

  int64_t BooleanParameter::getApproximateTextProtocolLength()
  {
    return 1;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void BooleanParameter::writeBinary(PacketOutputStream& pos)
  {
    pos.write(value ? 1 : 0);
  }

  uint32_t BooleanParameter::writeBinary(sql::bytes & buffer)
  {
    buffer[0]= value ? '\1' : '\0';

    return 1;
  }

  const ColumnType& BooleanParameter::getColumnType() const
  {
    return ColumnType::TINYINT;
  }

  SQLString BooleanParameter::toString()
  {
    return std::to_string(value);
  }

  bool BooleanParameter::isNullData() const
  {
    return false;
  }

  bool BooleanParameter::isLongData()
  {
    return false;
  }


  void * BooleanParameter::getValuePtr()
  {
    return static_cast<void *>(&value);
  }
}
}
