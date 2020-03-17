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


#include "NullParameter.h"

namespace sql
{
namespace mariadb
{

  const char* NullParameter::_NULL= "NULL";//{ 'N','U','L','L' };

  NullParameter::NullParameter() : type(ColumnType::_NULL)
  {

  }

  NullParameter::NullParameter(const ColumnType& _type)
    : type(_type)
  {
  }


  void NullParameter::writeTo(SQLString& str)
  {
    str.append(_NULL);
  }


  void NullParameter:: writeTo(PacketOutputStream& os)
  {
    os.write(_NULL);
  }

  int64_t NullParameter::getApproximateTextProtocolLength()
  {
    return 4;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    */
  void NullParameter::writeBinary(PacketOutputStream& pos)
  {

  }

  const ColumnType& NullParameter::getColumnType() const
  {
    return type;
  }

  SQLString NullParameter::toString()
  {
    return "<NULL>";
  }

  bool NullParameter::isNullData() const
  {
    return true;
  }

  bool NullParameter::isLongData()
  {
    return false;
  }
}
}
