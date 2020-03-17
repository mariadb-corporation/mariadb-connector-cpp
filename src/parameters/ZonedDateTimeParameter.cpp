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


#include "ZonedDateTimeParameter.h"

namespace sql
{
namespace mariadb
{

  /**
   * Constructor.
   *
   * @param tz zone date time
   * @param serverZoneId server session zoneId
   * @param fractionalSeconds must fractional Seconds be send to database.
   * @param options session options
   */
  ZonedDateTimeParameter::ZonedDateTimeParameter( ZonedDateTime tz,ZoneId serverZoneId,bool fractionalSeconds,Shared::Options options)
  {
ZoneId zoneId= options->useLegacyDatetimeCode ?ZoneOffset.systemDefault():serverZoneId;
this->tz= tz.withZoneSameInstant(zoneId);
this->fractionalSeconds= fractionalSeconds;
}

  /**
   * Write timestamps to outputStream.
   *
   * @param pos the stream to write to
   */
  void ZonedDateTimeParameter::writeTo(PacketOutputStream& pos)
  {
DateTimeFormatter formatter =
DateTimeFormatter.ofPattern(
fractionalSeconds ?"yyyy-MM-dd HH:mm:ss.SSSSSS":"yyyy-MM-dd HH:mm:ss",
Locale.ENGLISH);
pos.write(QUOTE);
pos.write(formatter.format(tz).getBytes());
pos.write(QUOTE);
}

  int64_t ZonedDateTimeParameter::getApproximateTextProtocolLength()
  {
return 27;
}

  /**
   * Write data to socket in binary format.
   *
   * @param pos socket output stream
   * @throws IOException if socket error occur
   */
  void ZonedDateTimeParameter::writeBinary(PacketOutputStream& pos)
  {
pos.write((fractionalSeconds ?11 :7));
pos.writeShort(static_cast<int16_t>(tz.getYear());)
pos.write(((tz.getMonth().getValue())&0xff));
pos.write((tz.getDayOfMonth()&0xff));
pos.write(tz.getHour());
pos.write(tz.getMinute());
pos.write(tz.getSecond());
if (fractionalSeconds){
pos.writeInt(tz.getNano()1000);
}
}

  const ColumnType& getColumnType()
  {
return ColumnType::DATETIME;
}

  SQLString ZonedDateTimeParameter::toString()
  {
return "'"+tz/*.toString()*/+"'";
}

  bool ZonedDateTimeParameter::isNullData()
  {
return false;
}

  bool ZonedDateTimeParameter::isLongData()
  {
return false;
}
}
}
