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


#include "Packet.h"

namespace sql
{
namespace mariadb
{

  const int8_t Packet::ERROR= (int8_t)0xff;
  const int8_t Packet::OK= (int8_t)0x00;
  const int8_t Packet::EOF_= (int8_t)0xfe;
  const int8_t Packet::LOCAL_INFILE= (int8_t)0xfb;
  // send command
  const int8_t Packet::COM_QUIT= (int8_t)0x01;
  const int8_t Packet::COM_INIT_DB= (int8_t)0x02;
  const int8_t Packet::COM_QUERY= (int8_t)0x03;
  const int8_t Packet::COM_PING= (int8_t)0x0e;
  const int8_t Packet::COM_STMT_PREPARE= (int8_t)0x16;
  const int8_t Packet::COM_STMT_EXECUTE= (int8_t)0x17;
  const int8_t Packet::COM_STMT_FETCH= (int8_t)0x1c;
  const int8_t Packet::COM_STMT_SEND_LONG_DATA= (int8_t)0x18;
  const int8_t Packet::COM_STMT_CLOSE= (int8_t)0x19;
  const int8_t Packet::COM_RESET_CONNECTION= (int8_t)0x1f;
  const int8_t Packet::COM_STMT_BULK_EXECUTE= (int8_t)0xfa;
  const int8_t Packet::COM_MULTI= (int8_t)0xfe;
  // prepare statement cursor flag.
  const int8_t Packet::CURSOR_TYPE_NO_CURSOR= (int8_t)0x00;
  const int8_t Packet::CURSOR_TYPE_READ_ONLY= (int8_t)0x01;
  const int8_t Packet::CURSOR_TYPE_FOR_UPDATE= (int8_t)0x02;
}
}
