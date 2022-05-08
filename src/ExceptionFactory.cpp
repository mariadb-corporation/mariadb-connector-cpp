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


  void ExceptionFactory::Throw(std::unique_ptr<sql::SQLException> e)
  {
    sql::SQLSyntaxErrorException* asSyntaxError= dynamic_cast<sql::SQLSyntaxErrorException*>(e.get());

  }

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


  MariaDBExceptionThrower ExceptionFactory::createException(
      const SQLString& initialMessage, const SQLString& sqlState,
      int32_t errorCode,
      int64_t threadId,
      Shared::Options& options,
      MariaDbConnection* connection,
      Statement* statement,
      std::exception* cause,
      bool throwRightAway)
  {
    SQLString msg(buildMsgText(initialMessage, threadId, options, cause));

    MariaDBExceptionThrower returnEx;

    if (sqlState.compare("70100") == 0) { // ER_QUERY_INTERRUPTED
      SQLTimeoutException ex(msg, sqlState, errorCode);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }

    SQLString sqlClass(sqlState.empty() ? "42" : sqlState.substr(0, 2).c_str());

    if (sqlClass.compare("0A") == 0) {
      SQLFeatureNotSupportedException ex(msg, sqlState, errorCode, cause);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }
    else if (sqlClass.compare("22") == 0 || sqlClass.compare("26") == 0 || sqlClass.compare("2F") == 0
      || sqlClass.compare("20") == 0 || sqlClass.compare("42") == 0 || sqlClass.compare("XA"))
    {
      SQLSyntaxErrorException ex(msg, sqlState, errorCode, cause);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }
    else if (sqlClass.compare("25") == 0 || sqlClass.compare("28") == 0) {
      SQLInvalidAuthorizationSpecException ex(msg, sqlState, errorCode, cause);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }
    else if (sqlClass.compare("21") == 0 || sqlClass.compare("23") == 0) {
      SQLIntegrityConstraintViolationException ex(msg, sqlState, errorCode, cause);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }
    else if (sqlClass.compare("08") == 0) {
      SQLNonTransientConnectionException ex(msg, sqlState, errorCode, cause);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }
    else if (sqlClass.compare("40") == 0) {
      SQLTransactionRollbackException ex(msg, sqlState, errorCode, cause);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }
    else {
      SQLTransientConnectionException ex(msg, sqlState, errorCode, cause);
      if (throwRightAway) {
        throw ex;
      }
      else {
        returnEx.take(ex);
      }
    }

    if (connection && connection->pooledConnection) {
      connection->pooledConnection->fireStatementErrorOccured(statement, returnEx);
    }
    return returnEx;
  }

  SQLString ExceptionFactory::buildMsgText(const SQLString& initialMessage, int64_t threadId, Shared::Options& options, std::exception* cause)
  {
    std::ostringstream msg("");
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

  MariaDBExceptionThrower ExceptionFactory::create(SQLException& cause, bool throwRightAway)
  {
    return createException(
        cause.getMessage(),
        cause.getSQLState(),
        cause.getErrorCode(),
        threadId,
        options,
        connection,
        statement,
        &cause,
        throwRightAway);
  }

  SQLFeatureNotSupportedException ExceptionFactory::notSupported(const SQLString& message)
  {
    //TODO this particular case leaving a bit ugly. But on othe hand it doesn't need any 
    return *createException(message, "0A000", -1, threadId, options, connection, statement, nullptr).get<SQLFeatureNotSupportedException>();
  }

  MariaDBExceptionThrower ExceptionFactory::create(const SQLString& message, bool throwRightAway)
  {
    return createException(message,"42000",-1,threadId,options,connection,statement, nullptr, throwRightAway);
  }

  MariaDBExceptionThrower ExceptionFactory::create(const SQLString& message, std::exception* cause, bool throwRightAway)
  {
    return createException(message, "42000", -1, threadId, options, connection, statement, cause, throwRightAway);
  }

  MariaDBExceptionThrower ExceptionFactory::create(const SQLString& message, const SQLString& sqlState, bool throwRightAway)
  {
    return createException(message,sqlState,-1,threadId,options,connection,statement, nullptr, throwRightAway);
  }

  MariaDBExceptionThrower ExceptionFactory::create(const SQLString& message, const SQLString& sqlState, std::exception* cause, bool throwRightAway)
  {
    return createException(message,sqlState,-1,threadId,options,connection,statement,cause, throwRightAway);
  }

  MariaDBExceptionThrower ExceptionFactory::create(const SQLString& message, const SQLString& sqlState,int32_t errorCode, bool throwRightAway)
  {
    return createException(
        message,sqlState,errorCode,threadId,options,connection,statement, nullptr, throwRightAway);
  }

  MariaDBExceptionThrower ExceptionFactory::create(const SQLString& message, const SQLString& sqlState,int32_t errorCode,
    std::exception* cause, bool throwRightAway)
  {
    return createException(
        message,sqlState,errorCode,threadId,options,connection,statement,cause, throwRightAway);
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
    std::ostringstream asStr("");
    asStr << "ExceptionFactory{" << "threadId=" << threadId  << '}';
    return asStr.str();
  }
}
}
