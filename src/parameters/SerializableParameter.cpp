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


#include "SerializableParameter.h"

namespace sql
{
namespace mariadb
{

  SerializableParameter::SerializableParameter(const sql::Object &object, bool noBackslashEscapes)
    : object(object)
    , noBackSlashEscapes(noBackslashEscapes)
  {
  }

  /**
    * Write object to buffer for text protocol.
    *
    * @param pos the stream to write to
    * @throws IOException if error reading stream
    */
  void SerializableParameter::writeTo(PacketOutputStream& pos)
  {
    if (loadedStream/*.empty() == true*/) {
      writeObjectToBytes();
    }
    pos.write(BINARY_INTRODUCER);
    pos.writeBytesEscaped(loadedStream, loadedStream.length, noBackSlashEscapes);
    pos.write(QUOTE);
  }

  void SerializableParameter::writeObjectToBytes()
  {
    ByteArrayOutputStream baos;
    ObjectOutputStream oos(baos);
    oos.writeObject(object);
    loadedStream= baos.toByteArray();
    object= NULL;
  }

  /**
    * Return approximated data calculated length.
    *
    * @return approximated data length.
    * @throws IOException if error reading stream
    */
  int64_t SerializableParameter::getApproximateTextProtocolLength()
  {
    writeObjectToBytes();
    return loadedStream.length;
  }

  /**
    * Write data to socket in binary format.
    *
    * @param pos socket output stream
    * @throws IOException if socket error occur
    */
  void SerializableParameter::writeBinary(PacketOutputStream& pos)
  {
    if (loadedStream/*.empty() == true*/) {
      writeObjectToBytes();
    }
    pos.writeFieldLength(loadedStream.length);
    pos.write(loadedStream);
  }

  SQLString SerializableParameter::toString()
  {
    return "<Serializable>";
  }

  const ColumnType& getColumnType()
  {
    return ColumnType::BLOB;
  }

  bool SerializableParameter::isNullData()
  {
    return false;
  }

  bool SerializableParameter::isLongData()
  {
    return false;
  }
}
}
