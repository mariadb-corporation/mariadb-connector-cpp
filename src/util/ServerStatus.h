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


#ifndef _SERVERSTATUS_H_
#define _SERVERSTATUS_H_

#include "Consts.h"

namespace sql
{
namespace mariadb
{

enum ServerStatus
{
  IN_TRANSACTION= 1,
  AUTOCOMMIT= 2,
  MORE_RESULTS_EXISTS= 8,
  QUERY_NO_GOOD_INDEX_USED= 16,
  QUERY_NO_INDEX_USED= 32,
  CURSOR_EXISTS= 64,
  LAST_ROW_SENT= 128,
  DB_DROPPED= 256,
  NO_BACKSLASH_ESCAPES= 512,
  METADATA_CHANGED= 1024,
  QUERY_WAS_SLOW= 2048,
  PS_OUT_PARAMETERS= 4096,
  SERVER_SESSION_STATE_CHANGED_= 1 << 14
};

}
}
#endif
