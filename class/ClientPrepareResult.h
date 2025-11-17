/************************************************************************************
   Copyright (C) 2022, 2024 MariaDB Corporation plc

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
#include <memory>

#include "PrepareResult.h"

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
  const std::vector<std::pair<std::size_t, std::size_t>> queryParts;
  bool rewriteType= false;
  uint32_t paramCount= 0;
  bool isQueryMultiValuesRewritableFlag= true;
  bool isQueryMultipleRewritableFlag= true;
  bool noBackslashEscapes= false;


 ClientPrepareResult(
   const SQLString& sql,
   std::vector<std::pair<std::size_t, std::size_t>>& queryParts,
   bool isQueryMultiValuesRewritable,
   bool isQueryMultipleRewritable,
   bool rewriteType,
   bool noBackslashEscapes);

public:
  static bool canAggregateSemiColon(const SQLString& queryString,bool noBackslashEscapes);
  static ClientPrepareResult* rewritableParts(const SQLString& queryString, bool noBackslashEscapes);

  const SQLString& getSql() const;
  const std::vector<std::pair<std::size_t,std::size_t>>& getQueryParts() const;
  bool isQueryMultiValuesRewritable() const;
  bool isQueryMultipleRewritable() const;
  bool isRewriteType() const;
  std::size_t getParamCount() const;
  ResultSetMetaData* getEarlyMetaData() { return nullptr; }
  SQLString& assembleQuery(SQLString& sql, MYSQL_BIND* parameters, std::map<uint32_t, std::string> &longData) const;
  std::size_t assembleBatchQuery(SQLString& sql, MYSQL_BIND* parameters, uint32_t arraySize, std::size_t curIndex) const;
  };

namespace Unique
{
  typedef std::unique_ptr<mariadb::ClientPrepareResult> ClientPrepareResult;
}
}
#endif
