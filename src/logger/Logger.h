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


#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "StringImp.h"
#include "MariaDBException.h"

namespace sql
{
namespace mariadb
{
struct Logger
{
  Logger() {}
  virtual ~Logger() {}
  virtual bool isTraceEnabled()= 0;
  virtual void trace(const SQLString& msg)= 0;
  virtual bool isDebugEnabled()= 0;
  virtual void debug(const SQLString& msg)= 0;
  virtual void debug(const SQLString& msg, std::exception& e)= 0;
  virtual bool isInfoEnabled()= 0;
  virtual void info(const SQLString& msg)= 0;
  virtual bool isWarnEnabled()= 0;
  virtual void warn(const SQLString& msg)= 0;
  virtual bool isErrorEnabled()= 0;
  virtual void error(const SQLString& msg)= 0;
  virtual void error(const SQLString& msg, SQLException& e)= 0;
  virtual void error(const SQLString& msg, sql::MariaDBExceptionThrower& e)= 0;

private:
  Logger(const Logger &) {}
  void operator=(Logger &) {}
};

}
}
#endif