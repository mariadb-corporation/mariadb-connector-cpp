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


#ifndef _EXCEPTIONFACTORY_H_
#define _EXCEPTIONFACTORY_H_

#include "Consts.h"
#include "MariaDBException.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"
void throwStmtError(MYSQL_STMT* stmt);
}

class ExceptionFactory final  {

public:
  static ExceptionFactory INSTANCE; /*new ExceptionFactory(-1,NULL)*/
  static void Throw(std::unique_ptr<sql::SQLException> e);
private:
  int64_t threadId;
  Shared::Options options;
  MariaDbConnection* connection;
  Statement* statement;

public:
  ExceptionFactory(int64_t threadId, Shared::Options& options, MariaDbConnection* connection, Statement* statement);

private:
  ExceptionFactory(int64_t threadId, Shared::Options options);

public:
  static ExceptionFactory* of(int64_t threadId, Shared::Options& options);

private:
  static MariaDBExceptionThrower createException(
    const SQLString& initialMessage, const SQLString& sqlState,
    int32_t errorCode,
    int64_t threadId,
    Shared::Options& options,
    MariaDbConnection* connection,
    Statement* statement,
    std::exception* cause,
    bool throwRightAway= true);

  static SQLString buildMsgText(const SQLString& initialMessage, int64_t threadId, Shared::Options& options, std::exception* cause);

public:
  std::unique_ptr<ExceptionFactory> raiseStatementError(MariaDbConnection* connection, Statement* stmt);
  SQLFeatureNotSupportedException notSupported(const SQLString& message);

  MariaDBExceptionThrower create(SQLException& cause, bool throwRightAway = true);
  MariaDBExceptionThrower create(const SQLString& message, bool throwRightAway = true);
  MariaDBExceptionThrower create(const SQLString& message, std::exception* cause, bool throwRightAway = true);
  MariaDBExceptionThrower create(const SQLString& message, const SQLString& sqlState, bool throwRightAway = true);
  MariaDBExceptionThrower create(const SQLString& message, const SQLString& sqlState, std::exception* cause, bool throwRightAway = true);
  MariaDBExceptionThrower create(const SQLString& message, const SQLString& sqlState, int32_t errorCode, bool throwRightAway = true);
  MariaDBExceptionThrower create(const SQLString& message, const SQLString& sqlState, int32_t errorCode, std::exception* cause, bool throwRightAway = true);

  int64_t getThreadId();
  Shared::Options& getOptions();
  SQLString toString();
  };
}
}
#endif