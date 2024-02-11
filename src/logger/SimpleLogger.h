/************************************************************************************
   Copyright (C) 2024 MariaDB Corporation plc

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

#ifndef _SIMPLELOGGER_H_
#define _SIMPLELOGGER_H_

#include <iostream>
#include <sstream>
#include "SQLString.hpp"
#include "Exception.hpp"
#include "MariaDBException.h"

namespace sql
{
namespace mariadb
{

class SimpleLogger
{
  static uint32_t level;
  std::string signature;

public:
  constexpr static uint32_t ERROR= 1, WARNING= 2, INFO= 3, DEBUG= 4, TRACE= 5;

  SimpleLogger(const char* loggedClassName);

  static void setLoggingLevel(uint32_t _level);
  static void setLogFilename(const std::string& name);

  bool isTraceEnabled();
  void trace(const SQLString& msg);
  template<typename T>
  static std::string varmsg(const T& msg)
  {
    std::stringstream str;
    str << msg;
    return str.str();
  }
  template<typename T, typename... Args>
  static std::string varmsg(const T& msg, Args... args)
  {
    std::stringstream str;
    str << msg << " ";
    str << varmsg(args...);
    return str.str();
  }

  template<typename T, typename... Args>
  void trace(const T& msg, Args... args)
  {
    if (level > DEBUG) {
      SQLString assembled_msg(varmsg(msg, args...));
      trace(assembled_msg);
    }
  }
  bool isDebugEnabled();
  void debug(const SQLString& msg);
  void debug(const SQLString& msg, std::exception& e);
  void debug(const SQLString& msg, const SQLString& tag, int32_t total, int64_t active, int32_t pending);
  bool isInfoEnabled();
  void info(const SQLString& msg);
  bool isWarnEnabled();
  void warn(const SQLString& msg);
  bool isErrorEnabled();
  void error(const SQLString& msg);
  void error(const SQLString& msg, SQLException& e);
  void error(const SQLString& msg, MariaDBExceptionThrower& e);

};

}
}
#endif
