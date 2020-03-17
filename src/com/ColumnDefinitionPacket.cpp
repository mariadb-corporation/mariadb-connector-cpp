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


#include "ColumnInformationPacket.h"

namespace sql
{
namespace mariadb
{
  // where character_sets.character_set_name = collations.character_set_name order by id"
  int32_t maxCharlen[]={
  0,2,1,1,1,1,1,1,
  1,1,1,1,3,2,1,1,
  1,0,1,2,1,1,1,1,
  2,1,1,1,2,1,1,1,
  1,3,1,2,1,1,1,1,
  1,1,1,1,1,4,4,1,
  1,1,1,1,1,1,4,4,
  0,1,1,1,4,4,0,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,0,1,1,1,
  1,1,1,3,2,2,2,2,
  2,1,2,3,1,1,1,2,
  2,3,3,1,0,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,0,0,0,0,0,0,0,
  2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,
  2,2,2,2,0,2,0,0,
  0,0,0,0,0,0,0,2,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,0,0,0,0,
  0,0,0,0,0,0,0,0,
  3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,
  3,3,3,3,0,3,4,4,
  0,0,0,0,0,0,0,3,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,0,4,0,0,
  0,0,0,0,0,0,0,0
  };

  /**
    * Constructor for extent.
    *
    * @param other other columnInformation
    */
  ColumnInformationPacket::ColumnInformationPacket(const ColumnInformationPacket& other) :
    buffer(other.buffer),
    charsetNumber(other.charsetNumber),
    length(other.length),
    type(other.type),
    decimals(other.decimals),
    flags(other.flags)
  {
  }

  /**
    * Read column information from buffer.
    *
    * @param buffer buffer
    */
  ColumnInformationPacket::ColumnInformationPacket(Buffer* _buffer) :
    buffer(buffer),
    type(ColumnType::_NULL) // TODO: may be wrong
  {
     /*
    lenenc_str     catalog
    lenenc_str     schema
    lenenc_str     table
    lenenc_str     org_table
    lenenc_str     name
    lenenc_str     org_name
    lenenc_int     length of fixed-length fields [0c]
    2              character set
    4              column length
    1              type
    2              flags
    1              decimals
    2              filler [00] [00]
    */

    // set position after length encoded value, not read most of the time
    buffer.position = buffer.limit - 12;

    charsetNumber = buffer.readShort();
    length = buffer.readInt();
    type = ColumnType::fromServer(buffer.readByte() & 0xff, charsetNumber);
    flags = buffer.readShort();
    decimals = buffer.readByte();
  }
  /**
    * Constructor.
    *
    * @param name column name
    * @param type column type
    * @return ColumnInformationPacket
    */
  Shared::ColumnInformation ColumnInformationPacket::create(const SQLString& name, const ColumnType& _type)
  {
    const char* nameBytes= name.c_str();

    sql::bytes arr(23 +2 *name.length());
    int32_t pos= 0;

    for (int32_t i= 0; i <4; i++)
    {
      arr[pos++]= 1;
      arr[pos++]= 0;
    }

    for (int32_t i= 0; i <2; i++)
    {
      arr[pos++]= static_cast<char>(name.length());
      std::memcpy(static_cast<char*>(arr) + pos, nameBytes, name.length());
      pos+= name.length();
    }

    arr[pos++]= 0xc;

    arr[pos++]= 33;
    arr[pos++]= 0;

    int32_t len;

    switch (_type.getSqlType()) {
    case Types::VARCHAR:
    case Types::CHAR:
      len= 64 *3;
      break;
    case Types::SMALLINT:
      len= 5;
      break;
    case Types::_NULL:
      len= 0;
      break;
    default:
      len= 1;
      break;
    }

    arr[pos]= len;
    pos +=4;

    arr[pos++]= static_cast<char>(ColumnType::toServer(_type.getSqlType()).getType());

    arr[pos++]= len;
    arr[pos++]= 0;

    arr[pos++]= 0;

    arr[pos++]= 0;
    arr[pos]= 0;

    return new ColumnInformationPacket(new Buffer(arr));
  }

  SQLString ColumnInformationPacket::getString(int32_t idx) {
    buffer->position= 0;
    for (int32_t i= 0; i <idx; i++) {
      buffer->skipLengthEncodedBytes();
    }
    return buffer->readStringLengthEncoded();// StandardCharsets::UTF_8);
  }

  SQLString ColumnInformationPacket::getDatabase() const {
    return getString(1);
  }

  SQLString ColumnInformationPacket::getTable() const {
    return getString(2);
  }

  SQLString ColumnInformationPacket::getOriginalTable() const {
    return getString(3);
  }

  SQLString ColumnInformationPacket::getName() const
  {
    return getString(4);
  }

  SQLString ColumnInformationPacket::getOriginalName() const {
    return getString(5);
  }

  int16_t ColumnInformationPacket::getCharsetNumber() const {
    return charsetNumber;
  }

  int64_t ColumnInformationPacket::getLength() const {
    return length;
  }

  /**
    * Return metadata precision.
    *
    * @return precision
    */
  int64_t ColumnInformationPacket::getPrecision() const {
    if (type == ColumnType::OLDDECIMAL || type == ColumnType::DECIMAL)
    {
      if (isSigned()) {
        return length -((decimals >0) ? 2 : 1);
      }
      else {
        return length -((decimals >0) ? 1 : 0);
      }
    }
    return length;
  }

  /**
    * Get column size.
    *
    * @return size
    */
  int32_t ColumnInformationPacket::getDisplaySize() const {
    int32_t vtype= type.getSqlType();
    if (vtype == Types::VARCHAR ||vtype == Types::CHAR) {
      int32_t maxWidth= maxCharlen[charsetNumber &0xff];
      if (maxWidth == 0) {
        maxWidth= 1;
      }

      return static_cast<int32_t>(length) / maxWidth;
    }
    return static_cast<int32_t>(length);
  }

  uint8_t ColumnInformationPacket::getDecimals() const {
    return decimals;
  }

  ColumnType ColumnInformationPacket::getColumnType() const {
    return type;
  }

  int16_t ColumnInformationPacket::getFlags() const {
    return flags;
  }

  bool ColumnInformationPacket::isSigned() const {
    return ((flags & ColumnFlags::UNSIGNED)==0);
  }

  bool ColumnInformationPacket::isNotNull() const {
    return ((this->flags & 1)>0);
  }

  bool ColumnInformationPacket::isPrimaryKey() const {
    return ((this->flags &2)>0);
  }

  bool ColumnInformationPacket::isUniqueKey() const {
    return ((this->flags &4)>0);
  }

  bool ColumnInformationPacket::isMultipleKey() const {
    return ((this->flags &8)>0);
  }

  bool ColumnInformationPacket::isBlob() const {
    return ((this->flags & 16)>0);
  }

  bool ColumnInformationPacket::isZeroFill() const {
    return ((this->flags &64)>0);
  }

  // like string), but have the binary flag
  bool ColumnInformationPacket::isBinary() const{
  return (getCharsetNumber()==63);
  }
}
}
