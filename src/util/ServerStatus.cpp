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


#include "ServerStatus.h"

namespace sql
{
namespace mariadb
{

  const int16_t ServerStatus::IN_TRANSACTION= 1;
  const int16_t ServerStatus::AUTOCOMMIT= 2;
  const int16_t ServerStatus::MORE_RESULTS_EXISTS= 8;
  const int16_t ServerStatus::QUERY_NO_GOOD_INDEX_USED= 16;
  const int16_t ServerStatus::QUERY_NO_INDEX_USED= 32;
  const int16_t ServerStatus::CURSOR_EXISTS= 64;
  const int16_t ServerStatus::LAST_ROW_SENT= 128;
  const int16_t ServerStatus::DB_DROPPED= 256;
  const int16_t ServerStatus::NO_BACKSLASH_ESCAPES= 512;
  const int16_t ServerStatus::METADATA_CHANGED= 1024;
  const int16_t ServerStatus::QUERY_WAS_SLOW= 2048;
  const int16_t ServerStatus::PS_OUT_PARAMETERS= 4096;
  const int16_t ServerStatus::SERVER_SESSION_STATE_CHANGED= 1 <<14;
}
}
