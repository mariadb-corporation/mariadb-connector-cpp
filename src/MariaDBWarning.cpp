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


#include "MariaDBWarning.h"

namespace sql
{
namespace mariadb
{
  MariaDBWarning::~MariaDBWarning()
  {
  }

  MariaDBWarning::MariaDBWarning(const MariaDBWarning& other) : msg(other.msg), sqlState(other.sqlState), errorCode(other.errorCode), next()
  {}

  MariaDBWarning::MariaDBWarning(const SQLString& message)  : msg(message), next(), errorCode(0)
  {}

  /*MariaDBWarning::MariaDBWarning(const SQLString& message, const SQLString& state, int32_t error, const std::exception* e ) :
    msg(message), sqlState(state), errorCode(error), next()
  {}*/

  MariaDBWarning::MariaDBWarning(const char* message, const char* state, int32_t error, const std::exception* e ) :
    msg(message), sqlState(state), errorCode(error), next()
  {}

  SQLWarning* MariaDBWarning::getNextWarning() const
  {
    return next.get();
  }

  void MariaDBWarning::setNextWarning(SQLWarning* nextWarning)
  {
    next.reset(nextWarning);
  }

  const SQLString& MariaDBWarning::getSQLState() const
  {
    return sqlState;
  }

  int32_t MariaDBWarning::getErrorCode() const
  {
    return errorCode;
  }

  const SQLString& MariaDBWarning::getMessage() const
  {
    return msg;
  }

}
}
