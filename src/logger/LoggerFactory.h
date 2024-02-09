/************************************************************************************
   Copyright (C) 2020,2024 MariaDB Corporation plc

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


#ifndef _LOGGERFACTORY_H_
#define _LOGGERFACTORY_H_

#include <unordered_map>
#include <typeindex>
#include <memory>

#include "SimpleLogger.h"

#define GET_LOGGER() LoggerFactory::getLogger(typeid(*this))
namespace sql
{
namespace mariadb
{
class LoggerFactory
{
  static std::unique_ptr<std::unordered_map<std::type_index, SimpleLogger>> logger;
  static bool hasToLog;
  static bool initLoggersIfNeeded();
public:
  static void init(uint32_t logLevel, const std::string& logFileName);
  static SimpleLogger* getLogger(const std::type_info &typeId);
};
}
}
#endif