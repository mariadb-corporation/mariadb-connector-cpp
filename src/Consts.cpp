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

#include "Version.h"
#include "Consts.h"
#include "jdbccompat.hpp"

namespace sql
{
namespace mariadb
{

const char* Version::version= MACPP_VERSION_STR;
const SQLString ParameterConstant::TYPE_MASTER("master");
const SQLString ParameterConstant::TYPE_SLAVE("slave");
const SQLString emptyStr("");
const SQLString localhost("localhost");

const char QUOTE=     '\'';
const char DBL_QUOTE= '"';
const char ZERO_BYTE= '\0';
const char BACKSLASH= '\\';

std::map<std::string, enum HaMode> StrHaModeMap={ {"NONE", HaMode::NONE},
    {"AURORA", AURORA},
    {"REPLICATION", REPLICATION},
    {"SEQUENTIAL", SEQUENTIAL},
    {"LOADBALANCE", LOADBALANCE} };

const char* HaModeStrMap[]= {"NONE", "AURORA", "REPLICATION", "SEQUENTIAL", "LOADBALANCE"};

//int32_t arr[]= {11, 7, 29};
//sql::chars b(32);
//sql::Ints c(arr, 3);
}
}
