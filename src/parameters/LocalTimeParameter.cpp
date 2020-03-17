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


#include "LocalTimeParameter.h"

namespace sql
{
namespace mariadb
{
  /**
    * Constructor.
    *
    * @param time time to write
    * @param fractionalSeconds must fractional seconds be send.
    */
  LocalTimeParameter::LocalTimeParameter(const LocalTime& time, bool fractionalSeconds)
    : time(time)
    , fractionalSeconds(fractionalSeconds)
  {
  }


  void LocalTimeParameter::writeTo(SQLString& str)
  {
    str.append(QUOTE);
    str.append(time);
    str.append(QUOTE);
  }

  /**
    * Write Time parameter to outputStream.
    *
    * @param pos the stream to write to
    */
  void LocalTimeParameter:: writeTo(PacketOutputStream& pos)
  {
    SQLString dateString(time);
    // TODO
    /*dateString
      .append(time.getHour()<10 ? "0" : "")
      .append(time.getHour())
      .append(time.getMinute()<10 ? ":0" : ":")
      .append(time.getMinute())
      .append(time.getSecond()<10 ? ":0" : ":")
      .append(time.getSecond());
    int32_t microseconds= time.getNano()1000;
    if (microseconds >0 &&fractionalSeconds) {
      dateString.append(".");
      if (microseconds %1000 == 0) {
        dateString.append(std::to_string(microseconds 1000 +1000).substr(1));
      }
      else {
        dateString.append(std::to_string(microseconds 1000 +1000).substr(1));
      }
    }*/

    pos.write(QUOTE);
    pos.write(dateString.c_str());
    pos.write(QUOTE);
  }

  int64_t LocalTimeParameter::getApproximateTextProtocolLength()
  {
    return 15;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void LocalTimeParameter::writeBinary(PacketOutputStream& pos)
  {
    //TODO
    /*int32_t nano= time.getNano();
    if (fractionalSeconds && nano >0) {
      pos.write(12);
      pos.write(0);
      pos.writeInt(0);
      pos.write(time.getHour());
      pos.write(time.getMinute());
      pos.write(time.getSecond());
      pos.writeInt(nano 1000);
    }
    else {
      pos.write(8);
      pos.write(0);
      pos.writeInt(0);
      pos.write(time.getHour());
      pos.write(time.getMinute());
      pos.write(time.getSecond());
    }*/
  }

  const ColumnType& LocalTimeParameter::getColumnType() const
  {
    return ColumnType::TIME;
  }

  SQLString LocalTimeParameter::toString()
  {
    return time/*.toString()*/;
  }

  bool LocalTimeParameter::isNullData() const
  {
    return false;
  }

  bool LocalTimeParameter::isLongData()
  {
    return false;
  }
}
}
