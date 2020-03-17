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


#include "OffsetTimeParameter.h"

namespace sql
{
namespace mariadb
{

  /**
   * Constructor.
   *
   * @param offsetTime time with offset
   * @param serverZoneId server session zoneId
   * @param fractionalSeconds must fractional Seconds be send to database.
   * @param options session options
   * @throws SQLException if offset cannot be converted to server offset
   */
  OffsetTimeParameter::OffsetTimeParameter( OffsetTime offsetTime,ZoneId serverZoneId,bool fractionalSeconds,Shared::Options options)
  {
ZoneId zoneId= options->useLegacyDatetimeCode ?ZoneOffset.systemDefault():serverZoneId;
if (INSTANCEOF(zoneId, ZoneOffset)){
throw SQLException(
"cannot set OffsetTime, since server time zone is set to '"
+serverZoneId/*.toString()*/
+"' (check server variables time_zone and system_time_zone)");
}
this->time= offsetTime.withOffsetSameInstant((ZoneOffset)zoneId);
this->fractionalSeconds= fractionalSeconds;
}

  /**
   * Write timestamps to outputStream.
   *
   * @param pos the stream to write to
   */
  void OffsetTimeParameter::writeTo(PacketOutputStream& pos)
  {
DateTimeFormatter formatter =
DateTimeFormatter.ofPattern(
fractionalSeconds ?"HH:mm:ss.SSSSSS":"HH:mm:ss",Locale.ENGLISH);
pos.write(QUOTE);
pos.write(formatter.format(time).getBytes());
pos.write(QUOTE);
}

  int64_t OffsetTimeParameter::getApproximateTextProtocolLength()
  {
return 15;
}

  /**
   * Write data to socket in binary format.
   *
   * @param pos socket output stream
   * @throws IOException if socket error occur
   */
  void OffsetTimeParameter::writeBinary(PacketOutputStream& pos)
  {
if (fractionalSeconds){
pos.write(12);
pos.write(0);
pos.writeInt(0);
pos.write(time.getHour());
pos.write(time.getMinute());
pos.write(time.getSecond());
pos.writeInt(time.getNano()1000);
}else {
pos.write(8);
pos.write(0);
pos.writeInt(0);
pos.write(time.getHour());
pos.write(time.getMinute());
pos.write(time.getSecond());
}
}

  const ColumnType& getColumnType()
  {
return ColumnType::TIME;
}

  SQLString OffsetTimeParameter::toString()
  {
return "'"+time/*.toString()*/+"'";
}

  bool OffsetTimeParameter::isNullData()
  {
return false;
}

  bool OffsetTimeParameter::isLongData()
  {
return false;
}
}
}
