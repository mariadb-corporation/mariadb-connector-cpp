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


#include <istream>

#include "StreamParameter.h"

#include "util/Utils.h"

namespace sql
{
namespace mariadb
{

  /**
    * Constructor.
    *
    * @param is stream to write
    * @param length max length to write (if null the whole stream will be send)
    * @param noBackslashEscapes must backslash be escape
    */
  StreamParameter::StreamParameter(std::istream&is, int64_t length, bool noBackslashEscapes)
    : is(is)
    , length(length)
    , noBackslashEscapes(noBackslashEscapes)
  {
  }

  StreamParameter::StreamParameter(std::istream&is, bool noBackSlashEscapes): StreamParameter(is, INT64_MAX, noBackSlashEscapes)
  {
  }


  void StreamParameter::writeTo(SQLString& str)
  {
    if (is.fail()) {
      str.append("NULL");
      return;
    }
    str.append(BINARY_INTRODUCER);

    char buffer[8192];
    size_t readMax= sizeof(buffer), readTotal= static_cast<size_t>(length);
    std::streamsize readCount;

    do {

      if (readMax > readTotal)
      {
        readMax= readTotal;
      }

      readCount= is.read(buffer, readMax).gcount();

      if (readCount > 0)
      {
        readTotal-= static_cast<size_t>(readCount);
        Utils::escapeData(buffer, static_cast<size_t>(readCount), noBackslashEscapes, str);
      }
    } while (readTotal > 0 && readCount > 0);

    str.append(QUOTE);
  }


  uint32_t StreamParameter::writeBinary(sql::bytes &buffer)
  {
    std::size_t readMax= buffer.size(), readTotal= static_cast<uint32_t>(length);

    if (readMax > readTotal)
    {
      readMax= readTotal;
    }

    return static_cast<uint32_t>(is.read(buffer, readMax).gcount());
  }

  /**
    * Write stream in text format.
    *
    * @param pos database outputStream
    * @throws IOException if any error occur when reader stream
    */
  void StreamParameter::writeTo(PacketOutputStream& pos)
  {
    pos.write(BINARY_INTRODUCER);
    if (length == INT64_MAX) {
      pos.write(is, true, noBackslashEscapes);
    }
    else {
      pos.write(is, length, true, noBackslashEscapes);
    }
    pos.write(QUOTE);
  }

  /**
    * Return approximated data calculated length.
    *
    * @return approximated data length.
    */
  int64_t StreamParameter::getApproximateTextProtocolLength()
  {
    return -1;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void StreamParameter::writeBinary(PacketOutputStream& pos)
  {
    if (length == INT64_MAX) {
      pos.write(is, false, noBackslashEscapes);
    }
    else {
      pos.write(is, length, false, noBackslashEscapes);
    }
  }


  SQLString StreamParameter::toString()
  {
    return "<Stream>";
  }

  const ColumnType& StreamParameter::getColumnType() const
  {
    return ColumnType::BLOB;
  }

  bool StreamParameter::isNullData() const
  {
    return false;
  }

  bool StreamParameter::isLongData()
  {
    return true;
  }
}
}
