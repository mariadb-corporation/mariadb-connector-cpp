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


#include "StandardPacketInputStream.h"
#include "logger/LoggerFactory.h"
#include "util/Utils.h"

namespace sql
{
namespace mariadb
{
  const Shared::Logger StandardPacketInputStream::logger= LoggerFactory::getLogger(typeid(StandardPacketInputStream));
  /**
    * Constructor of standard socket MySQL packet stream reader.
    *
    * @param in stream
    * @param options connection options
    */
  StandardPacketInputStream::StandardPacketInputStream(std::istream* in, Shared::Options options)
    : maxQuerySizeToLog(options->maxQuerySizeToLog)
    , inputStream(in)
  {
    /*inputStream=
      options->useReadAheadInput
      ? new ReadAheadBufferedStream(in)
      : new BufferedInputStream(in, 16384);*/
  }

  /**
    * Constructor for single Data (using text format).
    *
    * @param value value
    * @return Buffer
    */
  sql::bytes StandardPacketInputStream::create(const std::string& value)
  {
    if (value.empty()) {
      sql::bytes res(1);
      res[0]= '\xFB';
      return res;
    }

    std::size_t length= value.length();
    if (length < 251)
    {
      sql::bytes buf(length + 1);
      char *arr= buf;
      buf[0]= (int8_t)length;
      memcpy(arr + 1, value.c_str(), length);
      return buf;
    }
    else if (length <65536) {

      sql::bytes buf(length +3);
      char *arr= buf;

      buf[0] =(int8_t)0xfc;
      buf[1] =(int8_t)length;
      buf[2] =(int8_t)(length >> 8);
      memcpy(arr + 3, value.c_str(), length);
      return buf;

    }
    else if (length <16777216) {

      sql::bytes buf(length + 4);
      char *arr= buf;

      buf[0] =(int8_t)0xfd;
      buf[1] =(int8_t)length;
      buf[2] =(int8_t)(length >>8);
      buf[3] =(int8_t)(length >>16);
      memcpy(arr + 4, value.c_str(), length);
      return buf;

    }
    else {
      sql::bytes buf(length + 9);
      char *arr= buf;

      buf[0] =(int8_t)0xfe;
      buf[1] =(int8_t)length;
      buf[2] =(int8_t)(length >>8);
      buf[3] =(int8_t)(length >>16);
      buf[4] =(int8_t)(length >>24);

      memcpy(arr + 9, value.c_str(), length);
      return buf;
    }
  }

  /*Buffer* StandardPacketInputStream::getPacket(bool reUsable)
  {
    return new Buffer(getPacketArray(reUsable), lastPacketLength);
  }*/

  /**
    * Get current input stream for creating compress input stream, to avoid losing already read bytes
    * in case of pipelining.
    *
    * @return input stream.
    */
  std::istream* StandardPacketInputStream::getInputStream()
  {
    return inputStream;
  }

  /**
    * Get next packet. If packet is more than 16M, read as many packet needed to finish packet.
    * (first that has not length = 16Mb)
    *
    * @param reUsable if can use existing reusable buffer to avoid creating array
    * @return array packet.
    * @throws IOException if socket exception occur.
    */
  sql::bytes StandardPacketInputStream::getPacketArray(bool reUsable)
  {
    // ***************************************************
    // Read 4 byte header
    // ***************************************************
    int32_t remaining= 4;
    int32_t off= 0;
    do {
      int32_t count= static_cast<int32_t>(inputStream->read(header.data(), remaining).gcount());

      // Cannot happen with std::streamsize count. Must be different condition(if any)
      /*if (count < 0) {
        throw SQLException(//EOFException
          "unexpected end of stream, read "
          + std::to_string(off)
          +" bytes from 4 (socket was closed by server)");
      }*/
      remaining-= count;
      off+= count;
    } while (remaining > 0);

    lastPacketLength= (header[0] &0xff)+((header[1] &0xff)<<8)+((header[2] &0xff)<<16);
    packetSeq= header[3];

    // prepare array
    sql::bytes rawBytes;
    if (reUsable && lastPacketLength < REUSABLE_BUFFER_LENGTH) {
      rawBytes.wrap(reusableArray.data(), reusableArray.size());
    }
    else {
      rawBytes.reserve(lastPacketLength);
    }

    // ***************************************************
    // Read content
    // ***************************************************
    remaining= lastPacketLength;
    off= 0;
    do {
      int32_t count= static_cast<int32_t>(inputStream->read(rawBytes, remaining).gcount());
      // Same
      //if (count < 0) {
      //  throw /*EOFException*/SQLException(
      //    "unexpected end of stream, read "
      //    + std::to_string(lastPacketLength -remaining)
      //    + " bytes from "
      //    + std::to_string(lastPacketLength)
      //    + " (socket was closed by server)");
      //}
      remaining-= count;
      off+= count;
    } while (remaining > 0);

    /*if (traceCache) {
      traceCache.put(
        new TraceObject(
          false,
          NOT_COMPRESSED,
          Arrays.copyOfRange(header, 0, 4),
          Arrays.copyOfRange(rawBytes, 0, off >1000 ? 1000 : off)));
    }*/

    if (logger->isTraceEnabled()) {
      logger->trace(
        "read: " + serverThreadLog +
        Utils::hexdump(maxQuerySizeToLog -4, 0, lastPacketLength, header.data(), static_cast<int32_t>(header.size())));//rawBytes));
    }

    // ***************************************************
    // In case content length is big, content will be separate in many 16Mb packets
    // **************************************************
    if (lastPacketLength == MAX_PACKET_SIZE) {
      int32_t packetLength;
      do {
        remaining= 4;
        off= 0;
        do {
          int32_t count= static_cast<int32_t>(inputStream->read(header.data(), remaining).gcount());
          // Same
          //if (count < 0) {
          //  throw /*EOFException*/SQLException("unexpected end of stream, read " + std::to_string(off) + " bytes from 4");
          //}
          remaining -=count;
          off +=count;
        } while (remaining >0);

        packetLength= (header[0] &0xff)+((header[1] &0xff)<<8)+((header[2] &0xff)<<16);
        packetSeq= header[3];

        int32_t currentBufferLength= static_cast<int32_t>(rawBytes.size());
        sql::bytes newRawBytes(currentBufferLength + packetLength);
        std::memcpy(newRawBytes, rawBytes, currentBufferLength);
        rawBytes= newRawBytes;

        remaining= packetLength;
        off= currentBufferLength;
        do {
          int32_t count= static_cast<int32_t>(inputStream->read(rawBytes, remaining).gcount());
          // Same
          /*if (count < 0) {
            throw SQLException(
              "unexpected end of stream, read "
              + std::to_string(packetLength - remaining)
              +" bytes from "
              + std::to_string(packetLength));
          }*/
          remaining-= count;
          off+= count;
        } while (remaining > 0);

#ifdef WE_HAVE_TRACE_CACHE
        if (traceCache) {
          traceCache.put(
            new TraceObject(
              false,
              NOT_COMPRESSED,
              Arrays.copyOfRange(header, 0, 4),
              Arrays.copyOfRange(rawBytes, 0, off >1000 ? 1000 : off)));
        }
#endif
        if (logger->isTraceEnabled()) {
          logger->trace(
            "read: " + serverThreadLog + Utils::hexdump(
              maxQuerySizeToLog -4, currentBufferLength, packetLength, header.data(), static_cast<int32_t>(header.size())));// rawBytes));
        }

        lastPacketLength +=packetLength;
      } while (packetLength == MAX_PACKET_SIZE);
    }

    return rawBytes;
  }

  int32_t StandardPacketInputStream::getLastPacketSeq()
  {
    return packetSeq;
  }

  int32_t StandardPacketInputStream::getCompressLastPacketSeq()
  {
    return 0;
  }

  void StandardPacketInputStream::close()
  {
    // TODO: when it is needed...
    //inputStream->close();
  }

  /**
    * Set server thread id.
    *
    * @param serverThreadId current server thread id.
    * @param isMaster is server master
    */
  void StandardPacketInputStream::setServerThreadId(int64_t serverThreadId, bool isMaster)
  {
    this->serverThreadLog=
      "conn="+ std::to_string(serverThreadId) + (isMaster ? "(M)" : "(S)");
  }

  /*void StandardPacketInputStream::setTraceCache(LruTraceCache traceCache)
  {
    this->traceCache= traceCache;
  }*/
}
}
