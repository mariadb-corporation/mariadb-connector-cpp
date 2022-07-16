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


#include <sstream>
#include <thread>

#include "LogQueryTool.h"

#include "parameters/ParameterHolder.h"
#include "SqlStates.h"
#include "PrepareResult.h"

namespace sql
{
namespace mariadb
{
  // TODO putting here dummy to shut up the compiler
  class SocketTimeoutException : public SQLException
  {};

  LogQueryTool::LogQueryTool(const Shared::Options& options)
    : options(options)
  {
  }

  /**
    * Get query, truncated if to big.
    *
    * @param sql current query
    * @return possibly truncated query if too big
    */
  SQLString LogQueryTool::subQuery(const SQLString& sql)
  {
    if (options->maxQuerySizeToLog  >0 && sql.size() > static_cast<size_t>(options->maxQuerySizeToLog -3)) {
      return sql.substr(0, options->maxQuerySizeToLog -3)+"...";
    }
    return sql;
  }

  /**
    * Get query, truncated if to big.
    *
    * @param buffer current query buffer
    * @return possibly truncated query if too big
    */
  SQLString LogQueryTool::subQuery(SQLString& buffer)
  {
    SQLString queryString;
    if (options->maxQuerySizeToLog == 0) {
      queryString= buffer.substr(5, buffer.length() - 5);
    }
    else {
      queryString= buffer.substr(5, std::min(buffer.length() - 5, static_cast<size_t>(options->maxQuerySizeToLog - 3)));
      if (queryString.size() > static_cast<std::size_t>(options->maxQuerySizeToLog > 2 ? options->maxQuerySizeToLog - 3 : 0)) {
        queryString= queryString.substr(0, std::max(options->maxQuerySizeToLog - 3, 0))+"...";
      }
    }
    return queryString;
  }

  /**
    * Return exception with query information's.
    *
    * @param sql current sql command
    * @param sqlException current exception
    * @param explicitClosed has connection been explicitly closed
    * @return exception with query information
    */
  SQLException LogQueryTool::exceptionWithQuery(const SQLString& sql, SQLException& sqlException, bool explicitClosed)
  {
    if (explicitClosed) {
      return SQLException("Connection has explicitly been closed/aborted.\nQuery is: " + subQuery(sql),
        sqlException.getSQLState(),
        sqlException.getErrorCode(),
        sqlException.getCause());
    }

    if (options->dumpQueriesOnException || sqlException.getErrorCode()==1064)
    {
      std::stringstream str;
      str << std::this_thread::get_id();

      return SQLException(
        sqlException.getMessage()
        +"\nQuery is: "
        + subQuery(sql)
        +"\nThread: "
        + str.str(),
        sqlException.getSQLState(),
        sqlException.getErrorCode(),
        sqlException.getCause());
    }
    return sqlException;
  }

  /**
    * Return exception with query information's.
    *
    * @param buffer query buffer
    * @param sqlEx current exception
    * @param explicitClosed has connection been explicitly closed
    * @return exception with query information
    */
  SQLException LogQueryTool::exceptionWithQuery(SQLString& buffer, SQLException& sqlEx, bool explicitClosed)
  {
    if (options->dumpQueriesOnException ||sqlEx.getErrorCode()==1064 ||explicitClosed) {
      return exceptionWithQuery(subQuery(buffer), sqlEx, explicitClosed);
    }
    return sqlEx;
  }

  /**
    * Return exception with query information's.
    *
    * @param parameters query parameters
    * @param sqlEx current exception
    * @param serverPrepareResult prepare results
    * @return exception with query information
    */
  SQLException LogQueryTool::exceptionWithQuery(std::vector<Shared::ParameterHolder>& parameters, SQLException& sqlEx, PrepareResult* serverPrepareResult)
  {
    if (sqlEx.getCause() && dynamic_cast<SocketTimeoutException*>(sqlEx.getCause()) != nullptr) {
      return SQLException("Connection* timed out", CONNECTION_EXCEPTION.getSqlState(), 0, &sqlEx);
    }
    if (options->dumpQueriesOnException) {
      return SQLException(
        exWithQuery(sqlEx.getMessage(), serverPrepareResult, parameters),
        sqlEx.getSQLState(),
        sqlEx.getErrorCode(),
        sqlEx.getCause());
    }
    return sqlEx;
  }

  /**
    * Return exception with query information's.
    *
    * @param sqlEx current exception
    * @param prepareResult prepare results
    * @return exception with query information
    */
  SQLException LogQueryTool::exceptionWithQuery(SQLException& sqlEx, PrepareResult* prepareResult)
  {
    if (options->dumpQueriesOnException ||sqlEx.getErrorCode()==1064) {
      SQLString querySql(prepareResult->getSql());
      SQLString message(sqlEx.getMessage());
      if (options->maxQuerySizeToLog != 0 && querySql.size()>options->maxQuerySizeToLog -3) {
        message.append("\nQuery is: "+querySql.substr(0, options->maxQuerySizeToLog -3)+"...");
      }
      else {
        message.append("\nQuery is: "+querySql);
      }

      std::stringstream str;

      str << std::this_thread::get_id();
      message.append("\nthread id: ").append(str.str());

      return SQLException(message, sqlEx.getSQLState(), sqlEx.getErrorCode(), sqlEx.getCause());
    }
    return sqlEx;
  }

  /**
    * Return exception message with query.
    *
    * @param message current exception message
    * @param serverPrepareResult prepare result
    * @param parameters query parameters
    * @return exception message with query
    */
  SQLString LogQueryTool::exWithQuery(const SQLString& message, PrepareResult* serverPrepareResult, std::vector<Shared::ParameterHolder>& parameters)
  {
    if (options->dumpQueriesOnException) {
      SQLString sql(serverPrepareResult->getSql());
      if (serverPrepareResult->getParamCount()>0) {
        sql.append(", parameters [");
        if (parameters.size() > 0) {
          for (size_t i= 0;
            i < std::min(parameters.size(), serverPrepareResult->getParamCount());
            i++) {
            sql.append(parameters[i]->toString()).append(",");
          }
          sql= sql.substr(0, sql.length() - 1);
        }
        sql.append("]");
      }

      std::stringstream str;
      str << std::this_thread::get_id();

      if (options->maxQuerySizeToLog != 0 &&sql.size()>options->maxQuerySizeToLog -3) {
        return message
          +"\nQuery is: "
          +sql.substr(0, options->maxQuerySizeToLog -3)
          +"..."
          "\nThread: "
          +str.str();
      }
      else {
        return message
          +"\nQuery is: "
          +sql
          +"\nThread: "
          +str.str();
      }
    }
    return message;
  }
}
}
