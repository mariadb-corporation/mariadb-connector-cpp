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


#ifndef _CLIENTPREPARERESULT_H_
#define _CLIENTPREPARERESULT_H_

#include <vector>

#include "Consts.h"

#include "PrepareResult.h"

namespace sql
{
namespace mariadb
{

class ClientPrepareResult : public PrepareResult
{
  enum LexState
  {
  Normal= 0,
  SqlString,
  SlashStarComment,
  Escape,
  EOLComment,
  Backtick
  };

  const SQLString& sql;
  const std::vector<SQLString> queryParts;
  bool rewriteType;
  uint32_t paramCount;
  bool isQueryMultiValuesRewritableFlag; /*true*/
  bool isQueryMultipleRewritableFlag; /*true*/

 ClientPrepareResult(
  const SQLString& sql,
  std::vector<SQLString>& queryParts,
  bool isQueryMultiValuesRewritable,
  bool isQueryMultipleRewritable,
  bool rewriteType);

public:
  static ClientPrepareResult* parameterParts(const SQLString& queryString, bool noBackslashEscapes);
  static bool canAggregateSemiColon(const SQLString& queryString,bool noBackslashEscapes);
  static ClientPrepareResult* rewritableParts(const SQLString& queryString, bool noBackslashEscapes);

  const SQLString& getSql() const;
  const std::vector<SQLString>& getQueryParts() const;
  bool isQueryMultiValuesRewritable() const;
  bool isQueryMultipleRewritable() const;
  bool isRewriteType() const;
  std::size_t getParamCount() const;
  };

//void assembleQueryText(SQLString& resultSql, const ClientPrepareResult* clientPrepareResult, const std::vector<ParameterHolder>& parameters);

}
}
#endif