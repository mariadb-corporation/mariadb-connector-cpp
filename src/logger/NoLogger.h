/************************************************************************************
   Copyright (C) 2020, 2021 MariaDB Corporation AB

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

#ifndef _NOLOGGER_H_
#define _NOLOGGER_H_

#include "Logger.h"

namespace sql
{
namespace mariadb
{
struct NoLogger  : public Logger {
  bool isTraceEnabled();
  void trace(const SQLString& msg);
  bool isDebugEnabled();
  void debug(const SQLString& msg);
  void debug(const SQLString& msg, std::exception& e);
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