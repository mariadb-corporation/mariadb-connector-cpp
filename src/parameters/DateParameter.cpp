/************************************************************************************
   Copyright (C) 2020,2026 MariaDB Corporation plc

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


#include "DateParameter.h"

namespace sql
{
namespace mariadb
{

  /**
    * Represents a date, constructed with time in millis since epoch.
    *
    * @param date the date
    * @param timeZone timezone to use
    * @param options jdbc options
    */
  DateParameter::DateParameter(const Date& _date, TimeZone* /*_timeZone*/, const Shared::Options& _options)
    : date(_date)
    //, timeZone(timeZone)
    , options(_options)
  {
  }

  /**
    * Write to server OutputStream in text protocol.
    *
    * @param os output buffer
    */
  void DateParameter::writeTo(PacketOutputStream& os)
  {
    os.write(QUOTE);
    os.write(dateByteFormat());
    os.write(QUOTE);
  }

  void DateParameter::writeTo(SQLString& str)
  {
    str.append(QUOTE);
    str.append(dateByteFormat());
    str.append(QUOTE);
  }

  // It's not used anyway(so far)
  const char * DateParameter::dateByteFormat()
  {
    /*SimpleDateFormat sdf= new SimpleDateFormat("yyyy-MM-dd");
    if (options->useLegacyDatetimeCode || options->maximizeMysqlCompatibility) {
      sdf.setTimeZone(Calendar.getInstance().getTimeZone());
    }
    else {
      sdf.setTimeZone(timeZone);
    }*/

    return static_cast<const char*>(static_cast<void*>(&sql::DateImp::get(date)));//sdf.format(date).getBytes();
  }

  int64_t DateParameter::getApproximateTextProtocolLength() const
  {
    return 16;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void DateParameter::writeBinary(PacketOutputStream& pos)
  {
    /*Calendar* calendar= Calendar*.getInstance(timeZone);
    calendar->setTimeInMillis(date.getTime());*/
    pos.write(7);
    pos.writeShort(date.getYear());//calendar->get(Calendar.YEAR));)
    pos.write(date.getMonth());// ((calendar->get(Calendar.MONTH)+1)&0xff));
    pos.write(date.getDate());// (calendar->get(Calendar.DAY_OF_MONTH)&0xff));
    pos.write(0);
    pos.write(0);
    pos.write(0);
  }

  uint32_t DateParameter::writeBinary(sql::bytes & buffer)
  {
    if (buffer.size() < getValueBinLen())
    {
      throw SQLException("Parameter buffer size is too small for date value");
    }
    std::memcpy(buffer.arr, &sql::DateImp::get(date), getValueBinLen());
    return getValueBinLen();
  }

  const ColumnType& DateParameter::getColumnType() const
  {
    return ColumnType::DATE;
  }

  SQLString DateParameter::toString()
  {
    return "'" + date.toString() + "'";
  }

  bool DateParameter::isNullData() const
  {
    return false;
  }

  bool DateParameter::isLongData()
  {
    return false;
  }
}
}
