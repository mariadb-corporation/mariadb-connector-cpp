/************************************************************************************
   Copyright (C) 2022, 2025 MariaDB Corporation plc

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

#include "ServerSidePreparedStatement.h"
#include "Results.h"
#include "ResultSetMetaData.h"
#include "ServerPrepareResult.h"
#include "interface/Exception.h"
#include "Protocol.h"
#include "interface/ResultSet.h"


namespace mariadb
{
  ServerSidePreparedStatement::~ServerSidePreparedStatement()
  {
    if (parRowCallback) {
      delete parRowCallback;
    }
    if (serverPrepareResult) {
      if (serverPrepareResult->canBeDeallocate()) {
        delete serverPrepareResult;
      }
      else {
        serverPrepareResult->decrementShareCounter();
      }
    }
  }
  /**
    * Constructor for creating Server prepared statement.
    *
    * @param connection current connection
    * @param sql Sql String to prepare
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.SQL_CURSOR_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @throws SQLException exception
    */
  ServerSidePreparedStatement::ServerSidePreparedStatement(
    Protocol* _connection, const SQLString& _sql,
    int32_t resultSetScrollType)
    : PreparedStatement(_connection, _sql, resultSetScrollType)
  {
    prepare(_sql);
  }


  ServerSidePreparedStatement::ServerSidePreparedStatement(
    Protocol* connection, ServerPrepareResult* pr, int32_t resultSetScrollType)
    : PreparedStatement(connection, pr->getSql(), resultSetScrollType)
    , serverPrepareResult(pr)
  {
  }

  ServerSidePreparedStatement::ServerSidePreparedStatement(
    Protocol* _connection,
    int32_t resultSetScrollType)
    : PreparedStatement(_connection, resultSetScrollType)
  {
  }

  /**
    * Clone statement.
    *
    * @param connection connection
    * @return Clone statement.
    * @throws CloneNotSupportedException if any error occur.
    */
  ServerSidePreparedStatement* ServerSidePreparedStatement::clone(Protocol* connection)
  {
    ServerSidePreparedStatement* clone= new ServerSidePreparedStatement(connection, this->resultSetScrollType);
    clone->metadata.reset(new ResultSetMetaData(*metadata));
    clone->prepare(*sql);

    return clone;
  }


  void ServerSidePreparedStatement::prepare(const SQLString& _sql)
  {
    serverPrepareResult= new ServerPrepareResult(_sql, guard);
    sql= &serverPrepareResult->getSql();
    setMetaFromResult();
  }

  void ServerSidePreparedStatement::setMetaFromResult()
  {
    parameterCount= static_cast<int32_t>(serverPrepareResult->getParamCount());
    //initParamset(parameterCount);
    metadata.reset(serverPrepareResult->getEarlyMetaData());
    //parameterMetaData.reset(new MariaDbParameterMetaData(serverPrepareResult->getParameters()));
  }

  /* void ServerSidePreparedStatement::setParameter(int32_t parameterIndex, ParameterHolder* holder)
  {
    // TODO: does it really has to be map? can be, actually
    if (parameterIndex > 0 && parameterIndex < serverPrepareResult->getParamCount() + 1) {
      parameters[parameterIndex - 1].reset(holder);
    }
    else {
      SQLString error("Could not set parameter at position ");

      error.append(std::to_string(parameterIndex)).append(" (values was ").append(holder->toString()).append(")\nQuery - conn:");

      // A bit ugly - index validity is checked after parameter holder objects have been created.
      delete holder;

      error.append(std::to_string(getServerThreadId())).append(protocol->isMasterConnection() ? "(M)" : "(S)");
      error.append(" - \"");

      int32_t maxQuerySizeToLog= protocol->getOptions()->maxQuerySizeToLog;
      if (maxQuerySizeToLog > 0) {
        if (sql.size() < maxQuerySizeToLog) {
          error.append(sql);
        }
        else {
          error.append(sql.substr(0, maxQuerySizeToLog - 3) + "...");
        }
      }
      else {
        error.append(sql);
      }
      error.append(" - \"");
      logger->error(error);
      ExceptionFactory::INSTANCE.create(error).Throw();
    }
  }


  ParameterMetaData* ServerSidePreparedStatement::getParameterMetaData()
  {
    if (isClosed()) {
      throw SQLException("The query has been already closed");
    }

    return new MariaDbParameterMetaData(*parameterMetaData);
  } */

  ResultSetMetaData* ServerSidePreparedStatement::getMetaData()
  {
    return metadata.release(); // or get() ?
  }


  void ServerSidePreparedStatement::executeBatchInternal(uint32_t queryParameterSize)
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
    int32_t rc= mysql_stmt_execute(serverPrepareResult->getStatementId());
    if ( rc == 0)
    {
      getResult();
      if (!metadata) {
        setMetaFromResult();
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


  void ServerSidePreparedStatement::executeQueryPrologue(ServerPrepareResult* serverPrepareResult)
  {
    checkClose();
  }


  void ServerSidePreparedStatement::getResult()
  {
    if (mysql_stmt_field_count(serverPrepareResult->getStatementId()) == 0) {
      results->addStats(mysql_stmt_affected_rows(serverPrepareResult->getStatementId()), hasMoreResults());
    }
    else {
      serverPrepareResult->reReadColumnInfo();
      ResultSet *rs= ResultSet::create(results.get(), guard, serverPrepareResult);
      results->addResultSet(rs, hasMoreResults() || results->getFetchSize() > 0);
    }
  }


  bool ServerSidePreparedStatement::executeInternal(int32_t fetchSize)
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

      
    guard->executePreparedQuery(serverPrepareResult, results.get());

    results->commandEnd();
    return results->getResultSet() != nullptr;
  }


  void ServerSidePreparedStatement::close()
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

  const char* ServerSidePreparedStatement::getError()
  {
    return mysql_stmt_error(serverPrepareResult->getStatementId());
  }

  uint32_t ServerSidePreparedStatement::getErrno()
  {
    return mysql_stmt_errno(serverPrepareResult->getStatementId());
  }

  const char* ServerSidePreparedStatement::getSqlState()
  {
    return mysql_stmt_sqlstate(serverPrepareResult->getStatementId());
  }

  bool ServerSidePreparedStatement::bind(MYSQL_BIND* param)
  {
    this->param= param;
    return mysql_stmt_bind_param(serverPrepareResult->getStatementId(), param) != '\0';
  }

  bool ServerSidePreparedStatement::sendLongData(uint32_t paramNum, const char* data, std::size_t length)
  {
    return mysql_stmt_send_long_data(serverPrepareResult->getStatementId(), paramNum, data, static_cast<unsigned long>(length)) != '\0';
  }

//----------------------------- For param callbacks ------------------------------
 
  bool ServerSidePreparedStatement::setParamCallback(ParamCodec* callback, uint32_t param)
  {
    if (param == uint32_t(-1)) {
      if (parRowCallback) {
        delete parRowCallback;
      }
      parRowCallback= callback;
      if (callback != nullptr) {
        mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_CB_USER_DATA, (void*)this);
        return mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_CB_PARAM,
          (const void*)withRowCheckCallback);
      }
      else {
        // Let's say - NULL as row callbback resets everything. That seems to be least ambiguous behavior
        mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_CB_USER_DATA, (void*)NULL);
        return mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_CB_PARAM,
          (const void*)NULL);
      }
    }
    
    if (param >= serverPrepareResult->getParamCount()) {
      throw SQLException("Invalid parameter number");
    }

    parColCodec.insert({param, callback});
    if (parRowCallback == nullptr && parColCodec.size() == 1) {
      // Needed not to overwrite callback with row check
      mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_CB_USER_DATA, (void*)this);
      return mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_CB_PARAM, (const void*)defaultParamCallback);
    }
    return false;
  }

  const my_bool error= '\1';

  my_bool* defaultParamCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr)
  {
    // Assuming paramset skip is set in the indicator of 1st column
    if (bind->u.indicator && *bind->u.indicator == STMT_INDICATOR_IGNORE_ROW) {
      return NULL;
    }
    // We can't let the callback to throw - we have to intercept, as otherwise we are guaranteed to have
    // the protocol broken
    try
    {
      ServerSidePreparedStatement *stmt= reinterpret_cast<ServerSidePreparedStatement*>(data);

      for (uint32_t i= 0; i < stmt->getParamCount(); ++i) {
        auto current= bind + i;
        if (current->u.indicator && *current->u.indicator == STMT_INDICATOR_NULL) {
          continue;
        }
        const auto& it= stmt->parColCodec.find(i);
        /* Let's assume for now, that each column has to have a codec. But still we don't use vector and don't throw exception
         * if not all of them have. For such columns we could assume, that we have normal arrays that we need to pass, and this
         * function could do that simple job instead of callback. But indicator array would still be needed. Thus maybe callback
         * is still better. Another option could be - using default value(indicator for that)
         * But here there is another problem - the common "row" callback could take care of such columns
         * More likely that app will need to use array
         */
        if (it != stmt->parColCodec.end()) {
          if ((*it->second)(stmt->callbackData, current, i, row_nr)) {
            return (my_bool*)&error;
          }
        }
      }
    }
    catch (...)
    {
      return (my_bool*)&error;
    }
    return NULL;
  }


  my_bool* withRowCheckCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr)
  {
    // We can't let the callback to throw - we have to intercept, as otherwise we are guaranteed to have
    // the protocol broken
    try
    {
      ServerSidePreparedStatement *stmt= reinterpret_cast<ServerSidePreparedStatement*>(data);
      // Let's assume, that this callback should not be set if our callback is NULL
      if ((*stmt->parRowCallback)(stmt->callbackData, bind, -1, row_nr))
      {
        return (my_bool*)&error;
      }
      return defaultParamCallback(data, bind, row_nr);
    }
    catch (...)
    {
      return (my_bool*)&error;
    }
    return NULL;
  }


  bool ServerSidePreparedStatement::setCallbackData(void * data)
  {
    callbackData= data;
    // if C/C does not support callbacks 
    return mysql_stmt_attr_set(serverPrepareResult->getStatementId(), STMT_ATTR_CB_USER_DATA, (void*)this);
  }

  /* Checking if next result is the last one. That would mean, that the current is out params.
   * Method does not do any other checks and asusmes they are done by caller - i.e. it's callable result
   */
  bool ServerSidePreparedStatement::isOutParams()
  {
    return results->nextIsLast(guard);
  }
} // namespace mariadb
