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


#include "SqlStates.h"
#include <array>

namespace sql
{
namespace mariadb
{
  SqlStates WARNING("01");
  SqlStates NO_DATA("02");
  SqlStates CONNECTION_EXCEPTION("08");
  SqlStates FEATURE_NOT_SUPPORTED("0A");
  SqlStates CARDINALITY_VIOLATION("21");
  SqlStates DATA_EXCEPTION("22");
  SqlStates CONSTRAINT_VIOLATION("23");
  SqlStates INVALID_CURSOR_STATE("24");
  SqlStates INVALID_TRANSACTION_STATE("25");
  SqlStates INVALID_AUTHORIZATION("28");
  SqlStates SQL_FUNCTION_EXCEPTION("2F");
  SqlStates TRANSACTION_ROLLBACK("40");
  SqlStates SYNTAX_ERROR_ACCESS_RULE("42");
  SqlStates INVALID_CATALOG("3D");
  SqlStates INTERRUPTED_EXCEPTION("70");
  SqlStates UNDEFINED_SQLSTATE("HY");
  SqlStates TIMEOUT_EXCEPTION("JZ");
  SqlStates DISTRIBUTED_TRANSACTION_ERROR("XA");

  static const std::array<SqlStates, 18> sqlStates=
    { WARNING, NO_DATA, CONNECTION_EXCEPTION, FEATURE_NOT_SUPPORTED, CARDINALITY_VIOLATION, DATA_EXCEPTION,
      CONSTRAINT_VIOLATION, INVALID_CURSOR_STATE, INVALID_TRANSACTION_STATE, INVALID_AUTHORIZATION, SQL_FUNCTION_EXCEPTION,
      TRANSACTION_ROLLBACK, SYNTAX_ERROR_ACCESS_RULE, INVALID_CATALOG, INTERRUPTED_EXCEPTION, UNDEFINED_SQLSTATE,
      TIMEOUT_EXCEPTION, DISTRIBUTED_TRANSACTION_ERROR};

  SqlStates::SqlStates(const char* stateGroup) : sqlStateGroup(stateGroup)
  {
  }

  /**
    * Get sqlState from group.
    *
    * @param group group
    * @return sqlState
    */
   SqlStates SqlStates::fromString(const SQLString& group){
    for (auto state : sqlStates){
      if (group.startsWith(state.sqlStateGroup)){
        return state;
      }
    }
    return UNDEFINED_SQLSTATE;
  }

  const SQLString& SqlStates::getSqlState() const
  {
    return sqlStateGroup;
  }

}
}
