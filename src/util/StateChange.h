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


#ifndef _STATECHANGE_H_
#define _STATECHANGE_H_

#include "Consts.h"

namespace sql
{
namespace mariadb
{
enum StateChange {

  SESSION_TRACK_SYSTEM_VARIABLES = 0,
  SESSION_TRACK_SCHEMA = 1,
  SESSION_TRACK_STATE_CHANGE = 2,
  SESSION_TRACK_GTIDS = 3,
  SESSION_TRACK_TRANSACTION_CHARACTERISTICS = 4,
  SESSION_TRACK_TRANSACTION_STATE = 5
};

}
}
#endif
