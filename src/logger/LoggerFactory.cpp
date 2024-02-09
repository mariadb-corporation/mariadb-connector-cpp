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


#include "LoggerFactory.h"
#include "SimpleLogger.h"

namespace sql
{
namespace mariadb
{
  std::unique_ptr<std::unordered_map<std::type_index, SimpleLogger>> LoggerFactory::logger;
  bool LoggerFactory::hasToLog= false;

  void LoggerFactory::init(uint32_t logLevel, const std::string& logFileName)
  {
    if ((hasToLog != (logLevel > 0)) && logLevel > 0)
    {
      hasToLog= true;
      SimpleLogger::setLoggingLevel(logLevel);
      SimpleLogger::setLogFilename(logFileName);
    }
  }

  bool LoggerFactory::initLoggersIfNeeded()
  {
    if (!logger) {
      logger.reset(new std::unordered_map<std::type_index, SimpleLogger>());
    }
    return true;
  }

  SimpleLogger* LoggerFactory::getLogger(const std::type_info& typeId)
  {
    static bool inited= initLoggersIfNeeded();
    // Just to use inited and shut up the compiler
    if (inited)
    {
      const auto& it= logger->find(std::type_index(typeId));
      if (it == logger->end()) {
        auto inserted= logger->emplace(std::type_index(typeId), SimpleLogger(typeId.name()));
        return &inserted.first->second;
      }
      else {
        return &it->second;
      }
    }
    return nullptr;
  }
}
}
