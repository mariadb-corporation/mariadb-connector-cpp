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


#include "TimestampParameter.h"

namespace sql
{
namespace mariadb
{

  /**
    * Constructor.
    *
    * @param ts timestamps
    * @param timeZone timeZone
    * @param fractionalSeconds must fractional Seconds be send to database.
    */
  TimestampParameter::TimestampParameter(const Timestamp& _ts, const TimeZone* _timeZone, bool _fractionalSeconds)
    : ts(_ts)
    , timeZone(_timeZone)
    , fractionalSeconds(_fractionalSeconds)
  {
  }


  void TimestampParameter::writeTo(SQLString& str)
  {
    str.append(QUOTE);
    str.append(ts);
    str.append(QUOTE);
  }

  /**
    * Write timestamps to outputStream.
    *
    * @param pos the stream to write to
    */
  void TimestampParameter::writeTo(PacketOutputStream& pos)
  {
    // TODO
    /*SimpleDateFormat sdf= new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    sdf.setTimeZone(timeZone);*/

    pos.write(QUOTE);
    /*pos.write(sdf.format(ts).getBytes());
    int32_t microseconds= ts.getNanos()1000;
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
    pos.write(ts.c_str());
    pos.write(QUOTE);
  }

  int64_t TimestampParameter::getApproximateTextProtocolLength()
  {
    return 27;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void TimestampParameter::writeBinary(PacketOutputStream& pos)
  {
    // TODO
    /*Calendar* calendar= Calendar*.getInstance(timeZone);
    calendar->setTimeInMillis(ts.getTime());

    pos.write((fractionalSeconds ? 11 : 7));

    pos.writeShort(static_cast<int16_t>(calendar->get(Calendar*.YEAR));)
      pos.write(((calendar->get(Calendar*.MONTH)+1)&0xff));
    pos.write((calendar->get(Calendar*.DAY_OF_MONTH)&0xff));
    pos.write(calendar->get(Calendar*.HOUR_OF_DAY));
    pos.write(calendar->get(Calendar*.MINUTE));
    pos.write(calendar->get(Calendar*.SECOND));
    if (fractionalSeconds) {
      pos.writeInt(ts.getNanos()1000);
    }*/
  }

  uint32_t TimestampParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for timestamp value");
    }
    std::memcpy(buffer.arr, ts.c_str(), getValueBinLen());
    return getValueBinLen();
  }

  const ColumnType& TimestampParameter::getColumnType() const
  {
    return ColumnType::DATETIME;
  }

  SQLString TimestampParameter::toString()
  {
    return "'"+ts/*.toString()*/+"'";
  }

  bool TimestampParameter::isNullData() const
  {
    return false;
  }

  bool TimestampParameter::isLongData()
  {
    return false;
  }

  void* TimestampParameter::getValuePtr()
  {
    return const_cast<void*>(static_cast<const void*>(ts.c_str()));
  }


}
}
