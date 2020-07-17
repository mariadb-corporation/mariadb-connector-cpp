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


#ifndef _PACKETOUTPUTSTREAM_H_
#define _PACKETOUTPUTSTREAM_H_

#include <istream>

#include "StringImp.h"

namespace sql
{
namespace mariadb
{

class PacketOutputStream
{
  PacketOutputStream(const PacketOutputStream &);
  void operator=(PacketOutputStream &);

public:
  PacketOutputStream() {}
  virtual ~PacketOutputStream(){}
  virtual void startPacket(int32_t seqNo)=0;
  virtual void writeEmptyPacket(int32_t seqNo)=0;
  virtual void writeEmptyPacket()=0;
  virtual void write(int32_t arr)=0;

  virtual void write(const char* arr)=0;
  virtual void write(const char* arr, int32_t off,int32_t len)=0;
  virtual void write(const SQLString& str)=0;
  virtual void write(const SQLString& str, bool escape, bool noBackslashEscapes)=0;
  virtual void write(const std::istream& is,bool escape,bool noBackslashEscapes)=0;
  virtual void write(const std::istream& is,int64_t length,bool escape,bool noBackslashEscapes)= 0;
  virtual void write(const std::istringstream& reader,bool escape,bool noBackslashEscapes)=0;
  virtual void write(const std::istringstream& reader,int64_t length,bool escape,bool noBackslashEscapes)= 0;

  virtual void writeBytesEscaped(char* bytes,int32_t len,bool noBackslashEscapes)=0;
  virtual void flush()=0;
  virtual void close()=0;
  virtual bool checkRemainingSize(int32_t len)=0;
  virtual bool exceedMaxLength()=0;
  virtual std::ostream& getOutputStream()=0;

  virtual void writeShort(short value)=0;
  virtual void writeInt(int32_t value)=0;
  virtual void writeLong(int64_t value)=0;
  virtual void writeBytes(char value,int32_t len)=0;
  virtual void writeFieldLength(int64_t length)=0;

  virtual int32_t getMaxAllowedPacket()=0;
  virtual void setMaxAllowedPacket(int32_t maxAllowedPacket)=0;
#ifdef WE_HAVE_DECIDED_TO_HAVE_IT
  virtual void permitTrace(bool permitTrace)=0;
  virtual void setTraceCache(LruTraceCache traceCache)=0;
#endif
  virtual void setServerThreadId(int64_t serverThreadId, bool isMaster)=0;
  virtual void mark()=0;
  virtual bool isMarked()=0;
  virtual void flushBufferStopAtMark()=0;
  virtual bool bufferIsDataAfterMark()=0;
  virtual char* resetMark()=0;
  virtual int32_t initialPacketPos()=0;
  virtual void checkMaxAllowedLength(int32_t length)=0;
};

}
}
#endif
