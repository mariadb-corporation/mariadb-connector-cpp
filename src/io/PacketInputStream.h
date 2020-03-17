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


#ifndef _PacketInputStream_H_
#define _PacketInputStream_H_

namespace sql
{
namespace mariadb
{

class PacketInputStream  {
  PacketInputStream(const PacketInputStream &);
  void operator=(PacketInputStream &);

public:

  PacketInputStream() {}
  virtual ~PacketInputStream(){}

//  virtual Buffer getPacket(bool reUsable)=0;
  virtual sql::bytes getPacketArray(bool reUsable)=0;
  virtual int32_t getLastPacketSeq()=0;
  virtual int32_t getCompressLastPacketSeq()=0;
  virtual void close()=0;
  virtual void setServerThreadId(int64_t serverThreadId, bool isMaster)=0;
//  virtual void setTraceCache(LruTraceCache traceCache)=0;
};

}
}
#endif
