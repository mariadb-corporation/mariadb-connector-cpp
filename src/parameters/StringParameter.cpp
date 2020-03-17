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


#include "StringParameter.h"

#include "util/Utils.h"

namespace sql
{
namespace mariadb
{

  StringParameter::StringParameter(const SQLString& str, bool noBackslashEscapes)
    : stringValue(str)
    , noBackslashEscapes(noBackslashEscapes)
  {
  }

  /**
    * Send escaped String to outputStream.
    *
    * @param pos outpustream.
    */
  void StringParameter::writeTo(PacketOutputStream& pos)
  {
    pos.write(stringValue, true, noBackslashEscapes);
  }


  void StringParameter::writeTo(SQLString& str)
  {
    str.append(QUOTE);
    Utils::escapeData(stringValue.c_str(), stringValue.length(), noBackslashEscapes, str);
    str.append(QUOTE);
  }


  int64_t StringParameter::getApproximateTextProtocolLength()
  {
    return stringValue.size()*3;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void StringParameter::writeBinary(PacketOutputStream& pos)
  {
    //sql::bytes bytes= .c_str(/*StandardCharsets.UTF_8*/);
    pos.writeFieldLength(stringValue.length());
    pos.write(stringValue.c_str());
  }

  uint32_t StringParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for string value");
    }
    std::memcpy(buffer.arr, stringValue.c_str(), getValueBinLen());
    return getValueBinLen();
  }

  const ColumnType& StringParameter::getColumnType() const
  {
    return ColumnType::STRING;
  }

  SQLString StringParameter::toString()
  {
    if (stringValue.size()<1024) {
      return "'"+stringValue +"'";
    }
    else {
      return "'"+stringValue.substr(0, 1024)+"...'";
    }
  }

  bool StringParameter::isNullData() const
  {
    return false;
  }

  bool StringParameter::isLongData()
  {
    return false;
  }
}
}
