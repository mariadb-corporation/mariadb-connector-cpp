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

#include "ExceptionFactory.h"
#include "MariaDbConnection.h"


namespace sql
{
namespace mariadb
{
  ExceptionFactory ExceptionFactory::INSTANCE(-1, nullptr);


  ExceptionFactory::ExceptionFactory(int64_t threadId, Shared::Options& options, MariaDbConnection* connection, Statement* statement)
    : threadId(threadId)
    , options(options)
    , connection(connection)
    , statement(statement)
  {
  }


  ExceptionFactory::ExceptionFactory(int64_t _threadId, Shared::Options _options)
    : threadId(_threadId)
    , options(_options)
    , connection(nullptr)
    , statement(nullptr)
  {
  }


  ExceptionFactory* ExceptionFactory::of(int64_t threadId, Shared::Options& options)
  {
    return new ExceptionFactory(threadId, options);
  }


  Unique::SQLException ExceptionFactory::createException(
      const SQLString& initialMessage, const SQLString& sqlState,
      int32_t errorCode,
      int64_t threadId,
      Shared::Options& options,
      MariaDbConnection* connection,
      Statement* statement,
      std::exception* cause)
  {
    SQLString msg(buildMsgText(initialMessage, threadId, options, cause));

    Unique::SQLException returnEx;

    if (sqlState.compare("70100") == 0) { // ER_QUERY_INTERRUPTED
      returnEx.reset(new SQLTimeoutException(msg, sqlState, errorCode));
      return returnEx;
    }

    SQLString sqlClass(sqlState.empty() ? "42" : sqlState.substr(0, 2).c_str());

    if (sqlClass.compare("0A") == 0) {
      returnEx.reset(new SQLFeatureNotSupportedException(msg, sqlState, errorCode, cause));
    }
    else if (sqlClass.compare("22") == 0 || sqlClass.compare("26") == 0 || sqlClass.compare("2F") == 0
      || sqlClass.compare("20") == 0 || sqlClass.compare("42") == 0 || sqlClass.compare("XA"))
    {
      returnEx.reset(new SQLSyntaxErrorException(msg, sqlState, errorCode, cause));
    }
    else if (sqlClass.compare("25") == 0 || sqlClass.compare("28") == 0) {
      returnEx.reset(new SQLInvalidAuthorizationSpecException(msg, sqlState, errorCode, cause));
    }
    else if (sqlClass.compare("21") == 0 || sqlClass.compare("23") == 0) {
      returnEx.reset(new SQLIntegrityConstraintViolationException(msg, sqlState, errorCode, cause));
    }
    else if (sqlClass.compare("08") == 0) {
      returnEx.reset(new SQLNonTransientConnectionException(msg, sqlState, errorCode, cause));
    }
    else if (sqlClass.compare("40") == 0) {
      returnEx.reset(new SQLTransactionRollbackException(msg, sqlState, errorCode, cause));
    }
    else {
      returnEx.reset(new SQLTransientConnectionException(msg, sqlState, errorCode, cause));
    }

    if (connection && connection->pooledConnection) {
      connection->pooledConnection->fireStatementErrorOccured(statement, *returnEx);
    }
    return returnEx;
  }

  SQLString ExceptionFactory::buildMsgText(const SQLString& initialMessage, int64_t threadId, Shared::Options& options, std::exception* cause)
  {
    std::stringstream msg("");
    SQLString deadLockException;
    SQLString threadName;

    if (threadId != -1){
      msg << "(conn=" << threadId << ") " << initialMessage.c_str();
    }else {
      msg << initialMessage.c_str();
    }

    SQLException* exception= dynamic_cast<SQLException*>(cause);
    if (exception != nullptr) {

      SQLString sql;// = exception.getSql();

      if (options && options->dumpQueriesOnException && !sql.empty()){
        if (options && options->maxQuerySizeToLog != 0
            && sql.size() + 3 > static_cast<std::size_t>(options->maxQuerySizeToLog)) {
          msg << "\nQuery is: " << sql.substr(0, options->maxQuerySizeToLog - 3).c_str() << "...";
        }else {
          msg << "\nQuery is: " << sql.c_str();
        }
      }
      //deadLockException= exception.getDeadLockInfo();
      //threadName= exception.getThreadName();
    }

    if (options && options->includeInnodbStatusInDeadlockExceptions
        && !deadLockException.empty()) {
      msg << "\ndeadlock information: " << deadLockException.c_str();
    }

    if (options && options->includeThreadDumpInDeadlockExceptions) {
      if (!threadName.empty()){
        msg << "\nthread name: " << threadName.c_str();
      }
      msg << "\ncurrent threads: ";
      /*Thread.getAllStackTraces()
        .forEach(
            (thread,traces)->{
            msg.append("\n  name:\"")
            .append(thread.getName())
            .append("\" pid:")
            .append(thread.getId())
            .append(" status:")
            .append(thread.getState());
            for (int32_t i= 0;i <traces.length;i++){
            msg.append("\n    ").append(traces[i]);
            }
            });*/
    }

    return msg.str();
  }

  std::unique_ptr<ExceptionFactory> ExceptionFactory::raiseStatementError(MariaDbConnection* connection, Statement* stmt)
  {
    return std::unique_ptr<ExceptionFactory>(new ExceptionFactory(threadId, options, connection, stmt));
  }

  Unique::SQLException ExceptionFactory::create(SQLException& cause)
  {
    return createException(
        cause.getMessage(),
        cause.getSQLState(),
        cause.getErrorCode(),
        threadId,
        options,
        connection,
        statement,
        &cause);
  }

  SQLFeatureNotSupportedException ExceptionFactory::notSupported(const SQLString& message)
  {
    return *dynamic_cast<SQLFeatureNotSupportedException*>(createException(message, "0A000", -1, threadId, options, connection, statement, nullptr).get());
  }

  Unique::SQLException ExceptionFactory::create(const SQLString& message)
  {
    return createException(message,"42000",-1,threadId,options,connection,statement, nullptr);
  }

  Unique::SQLException ExceptionFactory::create(const SQLString& message, std::exception* cause)
  {
    return createException(message, "42000", -1, threadId, options, connection, statement, cause);
  }

  Unique::SQLException ExceptionFactory::create(const SQLString& message, const SQLString& sqlState)
  {
    return createException(message,sqlState,-1,threadId,options,connection,statement,NULL);
  }

  Unique::SQLException ExceptionFactory::create(const SQLString& message, const SQLString& sqlState, std::exception* cause)
  {
    return createException(message,sqlState,-1,threadId,options,connection,statement,cause);
  }

  Unique::SQLException ExceptionFactory::create(const SQLString& message, const SQLString& sqlState,int32_t errorCode)
  {
    return createException(
        message,sqlState,errorCode,threadId,options,connection,statement,NULL);
  }

  Unique::SQLException ExceptionFactory::create(const SQLString& message, const SQLString& sqlState,int32_t errorCode, std::exception* cause)
  {
    return createException(
        message,sqlState,errorCode,threadId,options,connection,statement,cause);
  }


  int64_t ExceptionFactory::getThreadId()
  {
    return threadId;
  }

  Shared::Options& ExceptionFactory::getOptions()
  {
    return options;
  }

  SQLString ExceptionFactory::toString()
  {
    std::stringstream asStr("");
    asStr << "ExceptionFactory{" << "threadId=" << threadId  << '}';
    return asStr.str();
  }
}
}
