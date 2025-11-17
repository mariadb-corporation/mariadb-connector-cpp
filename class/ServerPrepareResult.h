/************************************************************************************
   Copyright (C) 2022,2023 MariaDB Corporation plc

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


#ifndef _SERVERPREPARERESULT_H_
#define _SERVERPREPARERESULT_H_

#include <vector>
#include <memory>
#include <mutex>

#include "PrepareResult.h"
#include "SQLString.h"
#include "mysql.h"

namespace mariadb
{
class Protocol;

class ServerPrepareResult  : public PrepareResult
{
  std::mutex lock;
  ServerPrepareResult(const ServerPrepareResult&)= delete;
  const SQLString sql;
  Protocol* connection;
  MYSQL_STMT* statementId;
  /*std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata;*/
  unsigned long paramCount= 0;
  MYSQL_BIND* paramBind= nullptr;
  std::size_t  shareCounter= 1;
  bool isBeingDeallocate= false;

public:
  ~ServerPrepareResult();

  ServerPrepareResult(
    const SQLString& sql,
    Protocol* dbc);

  /* For the direct execution*/
  ServerPrepareResult(
    const SQLString& sql,
    unsigned long paramCount,
    Protocol* dbc);

  ServerPrepareResult(
    const SQLString& sql,
    MYSQL_STMT* statementId,
    Protocol* dbc);

  void reReadColumnInfo();

  //void resetParameterTypeHeader();
  size_t getParamCount() const;
  MYSQL_STMT* getStatementId();
  const MYSQL_FIELD* getFields() const;
  const std::vector<ColumnDefinition>& getColumns() const;
  //const MYSQL_BIND* getParameters() const;
  const SQLString&   getSql() const;
  ResultSetMetaData* getEarlyMetaData();
  bool    incrementShareCounter();
  void    decrementShareCounter();
  bool    canBeDeallocate();
  int32_t getShareCounter();
};

namespace Unique
{
  typedef std::unique_ptr<mariadb::ServerPrepareResult> ServerPrepareResult;
}

} // namespace mariadb

#endif
