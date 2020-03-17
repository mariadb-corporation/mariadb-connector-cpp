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


#ifndef _PACKET_H_
#define _PACKET_H_

#include "stdint.h"

namespace sql
{
namespace mariadb
{

  class Packet {

  public:
    static const int8_t ERROR; /*(int8_t)0xff*/
    static const int8_t OK; /*(int8_t)0x00*/
    static const int8_t EOF_; /*(int8_t)0xfe*/
    static const int8_t LOCAL_INFILE; /*(int8_t)0xfb*/
    static const int8_t COM_QUIT; /*(int8_t)0x01*/
    static const int8_t COM_INIT_DB; /*(int8_t)0x02*/
    static const int8_t COM_QUERY; /*(int8_t)0x03*/
    static const int8_t COM_PING; /*(int8_t)0x0e*/
    static const int8_t COM_STMT_PREPARE; /*(int8_t)0x16*/
    static const int8_t COM_STMT_EXECUTE; /*(int8_t)0x17*/
    static const int8_t COM_STMT_FETCH; /*(int8_t)0x1c*/
    static const int8_t COM_STMT_SEND_LONG_DATA; /*(int8_t)0x18*/
    static const int8_t COM_STMT_CLOSE; /*(int8_t)0x19*/
    static const int8_t COM_RESET_CONNECTION; /*(int8_t)0x1f*/
    static const int8_t COM_STMT_BULK_EXECUTE; /*(int8_t)0xfa*/
    static const int8_t COM_MULTI; /*(int8_t)0xfe*/
    static const int8_t CURSOR_TYPE_NO_CURSOR; /*(int8_t)0x00*/
    static const int8_t CURSOR_TYPE_READ_ONLY; /*(int8_t)0x01*/
    static const int8_t CURSOR_TYPE_FOR_UPDATE; /*(int8_t)0x02*/
  };
}
}
#endif