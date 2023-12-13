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


#ifndef _MARIADBSERVERCAPABILITIES_H_
#define _MARIADBSERVERCAPABILITIES_H_

#include "Consts.h"

namespace sql
{
namespace mariadb
{

struct MariaDbServerCapabilities  {
  static const int32_t CLIENT_MYSQL = 1;
  static const int32_t FOUND_ROWS = 2; /* Found instead of affected rows */
  static const int32_t LONG_FLAG = 4; /* Get all column flags */
  static const int32_t CONNECT_WITH_DB = 8; /* One can specify db on connect */
  static const int32_t NO_SCHEMA = 16; /* Don't allow database.table.column */
  static const int32_t COMPRESS = 32; /* Can use compression protocol */
  static const int32_t ODBC = 64; /* Odbc client */
  static const int32_t LOCAL_FILES = 128; /* Can use LOAD DATA LOCAL */
  static const int32_t IGNORE_SPACE = 256; /* Ignore spaces before '(' */
  static const int32_t CLIENT_PROTOCOL_41_ = 512; /* New 4.1 protocol */
  static const int32_t CLIENT_INTERACTIVE_ = 1024;
  static const int32_t SSL = 2048; /* Switch to SSL after handshake */
  static const int32_t IGNORE_SIGPIPE = 4096; /* IGNORE sigpipes */
  static const int32_t TRANSACTIONS = 8192;
  static const int32_t RESERVED = 16384; /* Old flag for 4.1 protocol  */
  static const int32_t SECURE_CONNECTION = 32768; /* New 4.1 authentication */
  static const int32_t MULTI_STATEMENTS = 1 << 16; /* Enable/disable multi-stmt support */
  static const int32_t MULTI_RESULTS = 1 << 17; /* Enable/disable multi-results */
  static const int32_t PS_MULTI_RESULTS =
      1 << 18; /* Enable/disable multi-results for PrepareStatement */
  static const int32_t PLUGIN_AUTH = 1 << 19; /* Client supports plugin authentication */
  static const int32_t CONNECT_ATTRS = 1 << 20; /* Client send connection attributes */
  static const int32_t PLUGIN_AUTH_LENENC_CLIENT_DATA =
      1 << 21; /* authentication data length is a length auth integer */
  static const int32_t CLIENT_SESSION_TRACK = 1 << 23; /* server send session tracking info */
  static const int32_t CLIENT_DEPRECATE_EOF = 1 << 24; /* EOF packet deprecated */
  static const int32_t PROGRESS_OLD =
      1 << 29; /* Client support progress indicator (before 10.2)*/

  /* MariaDB specific capabilities */
  static const int64_t MARIADB_CLIENT_PROGRESS =
      1LL << 32; /* Client support progress indicator (since 10.2) */
  static const int64_t MARIADB_CLIENT_COM_MULTI =
      1LL << 33; /* bundle command during connection */
  static const int64_t _MARIADB_CLIENT_STMT_BULK_OPERATIONS =
    1LL << 34; /* support of array binding */

};
}
}
#endif
