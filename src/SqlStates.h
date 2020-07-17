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


#ifndef _SQLSTATES_H_
#define _SQLSTATES_H_

#include "Consts.h"

#include "StringImp.h"

namespace sql
{
namespace mariadb
{

class SqlStates
{
  SQLString sqlStateGroup;

 public:

  SqlStates(const char* stateGroup );
  static SqlStates fromString(const SQLString& group);
  const SQLString& getSqlState() const;
};

extern SqlStates WARNING;//("01"),
extern SqlStates NO_DATA; //"02")
extern SqlStates CONNECTION_EXCEPTION; //"08")
extern SqlStates FEATURE_NOT_SUPPORTED; //"0A")
extern SqlStates CARDINALITY_VIOLATION; //"21")
extern SqlStates DATA_EXCEPTION; //"22")
extern SqlStates CONSTRAINT_VIOLATION; //"23")
extern SqlStates INVALID_CURSOR_STATE; //"24")
extern SqlStates INVALID_TRANSACTION_STATE; //"25")
extern SqlStates INVALID_AUTHORIZATION; //"28")
extern SqlStates SQL_FUNCTION_EXCEPTION; //"2F")
extern SqlStates TRANSACTION_ROLLBACK; //"40")
extern SqlStates SYNTAX_ERROR_ACCESS_RULE; //"42")
extern SqlStates INVALID_CATALOG; //"3D")
extern SqlStates INTERRUPTED_EXCEPTION; //"70")
extern SqlStates UNDEFINED_SQLSTATE; //"HY")
extern SqlStates TIMEOUT_EXCEPTION; //"JZ")
extern SqlStates DISTRIBUTED_TRANSACTION_ERROR;//("XA");

}
}
#endif
