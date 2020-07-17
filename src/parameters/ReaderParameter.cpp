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
#include "ReaderParameter.h"

#include "util/Utils.h"

namespace sql
{
namespace mariadb
{
  /**
    * Constructor.
    *
    * @param reader reader to write
    * @param length max length to write (can be null)
    * @param noBackslashEscapes must backslash be escape
    */
  ReaderParameter::ReaderParameter(std::istream& reader, int64_t length, bool noBackslashEscapes)
    : reader(reader)
    , length(length)
    , noBackslashEscapes(noBackslashEscapes)
  {
  }

  ReaderParameter::ReaderParameter(std::istream& reader, bool noBackslashEscapes) : ReaderParameter(reader, INT64_MAX, noBackslashEscapes)
  {
  }

  //
  void ReaderParameter::writeTo(SQLString& str)
  {
    char buffer[8192];
    size_t readMax= sizeof(buffer), readTotal= static_cast<size_t>(length);
    std::streamsize readCount;

    str.append(QUOTE);

    do {

      if (readMax > readTotal)
      {
        readMax= readTotal;
      }

      readCount= reader.read(buffer, readMax).gcount();

      if (readCount > 0)
      {
        readTotal-= static_cast<size_t>(readCount);
        Utils::escapeData(buffer, static_cast<size_t>(readCount), noBackslashEscapes, str);
      }
    } while (readTotal > 0 && readCount > 0);

    str.append(QUOTE);
  }


  uint32_t ReaderParameter::writeBinary(sql::bytes &buffer)
  {
    std::streamsize readMax= buffer.size(), readTotal= static_cast<uint32_t>(length);

    if (readMax > readTotal)
    {
      readMax= readTotal;
    }

    return static_cast<uint32_t>(reader.read(buffer, readMax).gcount());
  }

  /**
    * Write reader to database in text format.
    *
    * @param pos database outputStream
    * @throws IOException if any error occur when reading reader
    */
  void ReaderParameter::writeTo(PacketOutputStream& pos)
  {
    pos.write(QUOTE);
    if (length == INT64_MAX) {
      pos.write(reader, true, noBackslashEscapes);
    }
    else {
      pos.write(reader, length, true, noBackslashEscapes);
    }
    pos.write(QUOTE);
  }

  /**
    * Return approximated data calculated length for rewriting queries.
    *
    * @return approximated data length.
    */
  int64_t ReaderParameter::getApproximateTextProtocolLength()
  {
    return -1;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void ReaderParameter::writeBinary(PacketOutputStream& pos)
  {
    if (length == INT64_MAX) {
      pos.write(reader, false, noBackslashEscapes);
    }
    else {
      pos.write(reader, length, false, noBackslashEscapes);
    }
  }

  const ColumnType& ReaderParameter::getColumnType() const
  {
    return ColumnType::STRING;
  }

  SQLString ReaderParameter::toString()
  {
    return "<std::istringstream&>";
  }

  bool ReaderParameter::isNullData() const
  {
    return false;
  }

  bool ReaderParameter::isLongData()
  {
    return true;
  }
}
}
