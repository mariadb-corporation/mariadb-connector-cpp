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


#include "TimeParameter.h"

namespace sql
{
namespace mariadb
{

  /**
    * Constructor.
    *
    * @param time time to write
    * @param timeZone timezone
    * @param fractionalSeconds must fractional seconds be send.
    */
  TimeParameter::TimeParameter(const Time& time, const TimeZone* timeZone, bool fractionalSeconds)
    : time(time)
    , timeZone(timeZone)
    , fractionalSeconds(fractionalSeconds)
  {
  }


  void TimeParameter::writeTo(SQLString& str)
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
  void TimeParameter::writeTo(PacketOutputStream& pos)
  {
    // TODO
    /*SimpleDateFormat sdf= new SimpleDateFormat("HH:mm:ss");
    sdf.setTimeZone(timeZone);*/
    SQLString dateString(time);// = sdf.format(time);

    pos.write(QUOTE);
    /*pos.write(dateString.getBytes());
    int32_t microseconds= static_cast<int32_t>((time.getTime())%1000)*1000;
    if (microseconds >0 &&fractionalSeconds) {
      pos.write('.');
      int32_t factor= 100000;
      while (microseconds >0) {
        int32_t dig= microseconds factor;
        pos.write('0'+dig);
        microseconds -=dig *factor;
        factor= 10;
      }
    }*/
    pos.write(dateString.c_str());
    pos.write(QUOTE);
  }

  int64_t TimeParameter::getApproximateTextProtocolLength()
  {
    return 15;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void TimeParameter::writeBinary(PacketOutputStream& pos)
  {
    // TODO
    /*Calendar* calendar= Calendar*.getInstance(timeZone);
    calendar->setTime(time);
    calendar->set(Calendar*.DAY_OF_MONTH, 1);
    if (fractionalSeconds) {
      pos.write(12);
      pos.write(0);
      pos.writeInt(0);
      pos.write(calendar->get(Calendar*.HOUR_OF_DAY));
      pos.write(calendar->get(Calendar*.MINUTE));
      pos.write(calendar->get(Calendar*.SECOND));
      pos.writeInt(calendar->get(Calendar*.MILLISECOND)*1000);
    }
    else {
      pos.write(8);
      pos.write(0);
      pos.writeInt(0);
      pos.write(calendar->get(Calendar*.HOUR_OF_DAY));
      pos.write(calendar->get(Calendar*.MINUTE));
      pos.write(calendar->get(Calendar*.SECOND));
    }*/
  }

  uint32_t TimeParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for time value");
    }
    std::memcpy(buffer.arr, time.c_str(), getValueBinLen());
    return getValueBinLen();
  }

  const ColumnType& TimeParameter::getColumnType() const
  {
    return ColumnType::TIME;
  }

  SQLString TimeParameter::toString()
  {
    return time/*.toString()*/;
  }

  bool TimeParameter::isNullData() const
  {
    return false;
  }

  bool TimeParameter::isLongData()
  {
    return false;
  }
}
}
