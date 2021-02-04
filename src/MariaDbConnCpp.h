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


#ifndef __MARIADBCONNCPP_H_
#define __MARIADBCONNCPP_H_

#include <regex>
/* Might be, that we don't really need list */
#include <list>
#include <vector>
#include <map>
#include <string>
#include <istream>
#include <ostream>
#include <typeinfo>
#include <memory>
#include <mutex>
#include <functional>
#include <algorithm>

#include "conncpp.hpp"
#include "Version.h"
#include "Consts.h"
#include "Protocol.h"
#include "parameters/ParameterHolder.h"
#include "options/Options.h"
#include "options/DefaultOptions.h"
#include "options/DriverPropertyInfo.h"
#include "io/PacketOutputStream.h"
#include "Driver.hpp"
#include "UrlParser.h"
#include "MariaDbDatabaseMetaData.h"
#include "MariaDbConnection.h"
#include "cache/CallableStatementCache.h"
#include "pool/GlobalStateInfo.h"
#include "logger/LoggerFactory.h"

namespace sql
{
namespace mariadb
{
}
}
#endif
