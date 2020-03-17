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


#ifndef _EXCEPTIONMAPPER_H_
#define _EXCEPTIONMAPPER_H_

#include <array>
#include <exception>

#include "Consts.h"
#include "MariaDbConnection.h"

namespace sql
{
namespace mariadb
{

class ExceptionMapper
{
  static const std::array<int32_t, 3>LOCK_DEADLOCK_ERROR_CODE; /*new HashSet<>(Arrays.asList(1205,1213,1614))*/
public:
  static void throwException(SQLException& exception,MariaDbConnection* connection, MariaDbStatement* statement);
  static SQLException connException(const SQLString& message);
  static SQLException connException(const SQLString& message, std::exception* cause);
  static SQLException getException( SQLException& exception, MariaDbConnection* connection, MariaDbStatement* statement, bool timeout);
  static void checkConnectionException( SQLException& exception,MariaDbConnection* connection);
  static SQLException get(const SQLString& message, const SQLString& sqlState, int32_t errorCode, const std::exception *exception, bool timeout);
  static SQLException getSqlException(const SQLString& message, std::runtime_error& exception);
  static SQLException getSqlException(const SQLString& message, const SQLString& sqlState, std::runtime_error& exception);
  static SQLException getSqlException(const SQLString& message);
  static SQLFeatureNotSupportedException getFeatureNotSupportedException(const SQLString& message);
  static SQLString mapCodeToSqlState(int32_t code);
};

}
}
#endif