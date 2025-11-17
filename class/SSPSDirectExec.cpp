/************************************************************************************
   Copyright (C) 2025 MariaDB Corporation plc

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


#include <deque>

#include "SSPSDirectExec.h"
#include "Results.h"
#include "ResultSetMetaData.h"
#include "ServerPrepareResult.h"
#include "interface/Exception.h"
#include "Protocol.h"
#include "interface/ResultSet.h"


namespace mariadb
{
  SSPSDirectExec::~SSPSDirectExec()
  {
    // ServerSidePreparedStatement should care about everything
  }
  /**
    * Constructor for creating Server prepared statement.
    *
    * @param connection current connection
    * @param sql Sql String to prepare
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    * @throws SQLException exception
    */
  SSPSDirectExec::SSPSDirectExec(
    Protocol* _connection, const SQLString& _sql, unsigned long _paramCount, int32_t resultSetScrollType)
    : ServerSidePreparedStatement(_connection, new ServerPrepareResult(_sql, _paramCount, _connection), resultSetScrollType)
  {
  }

  
  ResultSetMetaData* SSPSDirectExec::getMetaData()
  {
    return metadata.release();
  }


  void SSPSDirectExec::executeBatchInternal(uint32_t queryParameterSize)
  {
    executeQueryPrologue(serverPrepareResult);

    results.reset(
      new Results(
        this,
        0,
        true,
        queryParameterSize,
        true,
        resultSetScrollType,
        emptyStr,
        nullptr));

    mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_ARRAY_SIZE, (void*)&queryParameterSize);
    if (param != nullptr) {
      mysql_stmt_bind_param(serverPrepareResult->getStatementId(), param);
    }
    auto& query= serverPrepareResult->getSql();
    int32_t rc= mariadb_stmt_execute_direct(serverPrepareResult->getStatementId(), query.c_str(), query.length());
    if ( rc == 0)
    {
      getResult();
      if (!metadata) {
        // At this point we should have no early metadata
        metadata.reset(serverPrepareResult->getEarlyMetaData());
      }
      results->commandEnd();
      return;
    }
    else
    {
      throw rc;
    }
    clearBatch();
  }


  bool SSPSDirectExec::executeInternal(int32_t fetchSize)
  {
    checkClose();
    validateParamset(serverPrepareResult->getParamCount());

    results.reset(
      new Results(
        this,
        fetchSize,
        false,
        1,
        true,
        resultSetScrollType,
        *sql,
        param));

      
    guard->directExecutePreparedQuery(serverPrepareResult, results.get());

    results->commandEnd();
    return results->getResultSet() != nullptr;
  }


  void SSPSDirectExec::close()
  {
    if (closed) {
      return;
    }

    markClosed();
    if (results) {
      if (results->getFetchSize() != 0) {
        // Probably not current results, but current streamer's results
        results->loadFully(true, guard);
      }
      results->close();
    }

    if (serverPrepareResult) {
      serverPrepareResult->decrementShareCounter();

      // deallocate from server if not cached
      if (serverPrepareResult->canBeDeallocate()) {
        delete serverPrepareResult;
        serverPrepareResult= nullptr;
      }
    }
  }
} // namespace mariadb
