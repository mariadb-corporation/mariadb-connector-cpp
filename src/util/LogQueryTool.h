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


#ifndef _LOGQUERYTOOL_H_
#define _LOGQUERYTOOL_H_

#include "Consts.h"

namespace sql
{
namespace mariadb
{
class PrepareResult;
class ParameterHolder;

class LogQueryTool  {

  const Shared::Options options;

public:
  LogQueryTool(const Shared::Options& options);
  SQLString subQuery(const SQLString& sql);

private:
  SQLString subQuery(SQLString& buffer);

public:
  SQLException exceptionWithQuery(const SQLString& sql, SQLException& sqlException, bool explicitClosed);
  SQLException exceptionWithQuery(SQLString& buffer, SQLException& sqlEx, bool explicitClosed);
  SQLException exceptionWithQuery(std::vector<Shared::ParameterHolder>& parameters, SQLException& sqlEx, PrepareResult* serverPrepareResult);
  SQLException exceptionWithQuery(SQLException& sqlEx, PrepareResult* prepareResult);

private:
  SQLString exWithQuery(const SQLString& message, PrepareResult* serverPrepareResult, std::vector<Shared::ParameterHolder>& parameters);
  };
}
}
#endif