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


#include "NoLogger.h"

namespace sql
{
namespace mariadb
{
  bool NoLogger::isTraceEnabled()
  {
    return false;
  }

  void NoLogger::trace(const SQLString& msg){
  }

  bool NoLogger::isDebugEnabled(){
    return false;
  }

  void NoLogger::debug(const SQLString& msg){
  }

  void NoLogger::debug(const SQLString& msg, std::exception& e)
  {}

  bool NoLogger::isInfoEnabled(){
    return false;
  }

  void NoLogger::info(const SQLString& msg){
  }

  bool NoLogger::isWarnEnabled(){
    return false;
  }

  void NoLogger::warn(const SQLString& msg){
  }

  bool NoLogger::isErrorEnabled(){
    return false;
  }

  void NoLogger::error(const SQLString& msg){
  }

  void NoLogger::error(const SQLString& msg, SQLException& e) {
  }
  void NoLogger::error(const SQLString& msg, MariaDBExceptionThrower& e){
  }
}
}
