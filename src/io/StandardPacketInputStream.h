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


#ifndef _STANDARDPACKETINPUTSTREAM_H_
#define _STANDARDPACKETINPUTSTREAM_H_

#include <array>

#include "Consts.h"

#include "PacketInputStream.h"
#include "ColumnType.h"

#define REUSABLE_BUFFER_LENGTH 1024
#define MAX_PACKET_SIZE 0xffffff
namespace sql
{
namespace mariadb
{

class StandardPacketInputStream  : public PacketInputStream
{
  static const Shared::Logger logger ; /*LoggerFactory.getLogger(typeid(StandardPacketInputStream))*/
  std::array<char, 4> header ; /*new int8_t[4]*/
  std::array<char, REUSABLE_BUFFER_LENGTH> reusableArray ; /*new int8_t[REUSABLE_BUFFER_LENGTH]*/
  std::istream* inputStream;
  int32_t maxQuerySizeToLog;
  int32_t packetSeq;
  int32_t lastPacketLength;
  SQLString serverThreadLog ; /*""*/
  //LruTraceCache traceCache ; /*NULL*/

public:
  StandardPacketInputStream(std::istream* in, Shared::Options options);
  static sql::bytes create(const std::string& value);

  template <size_t COLUMNCOUNT>
  static sql::bytes create(std::array<SQLString, COLUMNCOUNT> rowData, std::array<ColumnType, COLUMNCOUNT> columnTypes)
  {
    int32_t totalLength= 0;
    for (auto& columnData : rowData) {
      if (columnData.empty()) { //TODO: here and below may be incorrect. Probably need somthing with 3rd state like NULL
        totalLength++;
      }
      else {
        int32_t length= columnData.length();
        if (length < 251) {
          totalLength+= length + 1;
        }
        else if (length < 65536) {
          totalLength+= length +3;
        }
        else if (length < 16777216) {
          totalLength+= length +4;
        }
        else {
          totalLength+= length +9;
        }
      }
    }

    /*TODO: it may be still better to use string here instead of array. It looks like it has enough means to to this,
            plus C memcpy can be avoided. Con - returning the value. Would need to allocate on heap, and return pointer(or smart ptr) */
    sql::bytes buf(totalLength);
    char *arr= buf;

    int32_t pos= 0;
    for (auto& columnData : rowData) {
      if (columnData.empty()) {
        buf[pos++] =(int8_t)251;
      }
      else {
        int32_t length= columnData.length();
        if (length <251) {
          buf[pos++] =(int8_t)length;
        }
        else if (length <65536) {
          buf[pos++] =(int8_t)0xfc;
          buf[pos++] =(int8_t)length;
          buf[pos++] =(int8_t)(length >> 8);
        }
        else if (length <16777216) {
          buf[pos++] =(int8_t)0xfd;
          buf[pos++] =(int8_t)length;
          buf[pos++] =(int8_t)(length >> 8);
          buf[pos++] =(int8_t)(length >> 16);
        }
        else {
          buf[pos++] =(int8_t)0xfe;
          buf[pos++] =(int8_t)length;
          buf[pos++] =(int8_t)(length >> 8);
          buf[pos++] =(int8_t)(length >> 16);
          buf[pos++] =(int8_t)(length >> 24);
          // byte[] cannot have more than 4 byte length size, so buf[pos+5] -> buf[pos+8] = 0x00;
          pos +=4;
        }
        memcpy(arr + pos, columnData.c_str(), length);
        pos+= length;
      }
    }
    return buf;
  }

  //Buffer getPacket(bool reUsable);
  std::istream* getInputStream();
  sql::bytes getPacketArray(bool reUsable);
  int32_t getLastPacketSeq();
  int32_t getCompressLastPacketSeq();
  void close();
  void setServerThreadId(int64_t serverThreadId, bool isMaster);
  //void setTraceCache(LruTraceCache traceCache);
  };
}
}
#endif