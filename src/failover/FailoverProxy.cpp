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


#include "FailoverProxy.h"
#include "logger/LoggerFactory.h"

namespace sql
{
namespace mariadb
{

  const SQLString FailoverProxy::METHOD_IS_EXPLICIT_CLOSED= "isExplicitClosed";
  const SQLString FailoverProxy::METHOD_GET_OPTIONS= "getOptions";
  const SQLString FailoverProxy::METHOD_GET_URLPARSER= "getUrlParser";
  const SQLString FailoverProxy::METHOD_GET_PROXY= "getProxy";
  const SQLString FailoverProxy::METHOD_EXECUTE_QUERY= "executeQuery";
  const SQLString FailoverProxy::METHOD_SET_READ_ONLY= "setReadonly";
  const SQLString FailoverProxy::METHOD_GET_READ_ONLY= "getReadonly";
  const SQLString FailoverProxy::METHOD_IS_MASTER_CONNECTION= "isMasterConnection";
  const SQLString FailoverProxy::METHOD_VERSION_GREATER_OR_EQUAL= "versionGreaterOrEqual";
  const SQLString FailoverProxy::METHOD_SESSION_STATE_AWARE= "sessionStateAware";
  const SQLString FailoverProxy::METHOD_CLOSED_EXPLICIT= "closeExplicit";
  const SQLString FailoverProxy::METHOD_ABORT= "abort";
  const SQLString FailoverProxy::METHOD_IS_CLOSED= "isClosed";
  const SQLString FailoverProxy::METHOD_EXECUTE_PREPARED_QUERY= "executePreparedQuery";
  const SQLString FailoverProxy::METHOD_COM_MULTI_PREPARE_EXECUTES= "prepareAndExecutesComMulti";
  const SQLString FailoverProxy::METHOD_PROLOG_PROXY= "prologProxy";
  const SQLString FailoverProxy::METHOD_RESET= "reset";
  const SQLString FailoverProxy::METHOD_IS_VALID= "isValid";
  const SQLString FailoverProxy::METHOD_GET_LOCK= "getLock";
  const SQLString FailoverProxy::METHOD_GET_NO_BACKSLASH= "noBackslashEscapes";
  const SQLString FailoverProxy::METHOD_GET_SERVER_THREAD_ID= "getServerThreadId";
  const SQLString FailoverProxy::METHOD_PROLOG= "prolog";
  const SQLString FailoverProxy::METHOD_GET_CATALOG= "getCatalog";
  const SQLString FailoverProxy::METHOD_GET_TIMEOUT= "getTimeout";
  const SQLString FailoverProxy::METHOD_GET_MAJOR_VERSION= "getMajorServerVersion";
  const SQLString FailoverProxy::METHOD_IN_TRANSACTION= "inTransaction";
  const SQLString FailoverProxy::METHOD_IS_MARIADB= "isServerMariaDb";

  const Shared::Logger logger= LoggerFactory::getLogger(typeid(FailoverProxy));

  /**
   * Proxy constructor.
   *
   * @param listener failover implementation.
   * @param lock synchronisation lock
   * @throws SQLException if connection error occur
   */
  FailoverProxy::FailoverProxy(Listener* _listener, std::mutex* _lock)
    : lock(_lock)
    , listener(_listener)
  {
    listener->setProxy(this);
    listener->initializeConnection();
  }

  /**
   * Add Host information ("on HostAddress...") to exception.
   *
   * <p>example : java.sql.SQLException: (conn=603) Cannot execute statement in a READ ONLY
   * transaction.<br>
   * Query is: INSERT INTO TableX VALUES (21)<br>
   * on HostAddress{host='mydb.example.com', port=3306},master=true
   *
   * @param exception current exception
   * @param protocol protocol to have hostname
   */
  SQLException FailoverProxy::addHostInformationToException( SQLException& exception, Shared::Protocol& protocol)
  {
    if (protocol)
    {
      return SQLException(
        exception.getMessage().append("\non ").append(protocol->getHostAddress().toString()).append(",master=").append(protocol->isMasterConnection()),
        exception.getSQLState(),
        exception.getErrorCode());
          //exception->getCause());
    }
    return exception;
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  sql::Object* FailoverProxy::invoke(sql::Object* proxy, Method method, sql::sql::Object** args) {
    SQLString methodName= method.getName();
    switch (methodName){
      case METHOD_GET_LOCK:
        return this->lock;
      case METHOD_GET_NO_BACKSLASH:
        return listener.noBackslashEscapes();
      case METHOD_IS_MARIADB:
        return listener.isServerMariaDb();
      case METHOD_GET_CATALOG:
        return listener.getCatalog();
      case METHOD_GET_TIMEOUT:
        return listener.getTimeout();
      case METHOD_VERSION_GREATER_OR_EQUAL:
        return listener.versionGreaterOrEqual((int32_t)args[0],(int32_t)args[1],(int32_t)args[2]);
      case METHOD_SESSION_STATE_AWARE:
        return listener.sessionStateAware();
      case METHOD_IS_EXPLICIT_CLOSED:
        return listener.isExplicitClosed();
      case METHOD_GET_OPTIONS:
        return listener.getUrlParser().getOptions();
      case METHOD_GET_MAJOR_VERSION:
        return listener.getMajorServerVersion();
      case METHOD_GET_SERVER_THREAD_ID:
        return listener.getServerThreadId();
      case METHOD_GET_URLPARSER:
        return listener.getUrlParser();
      case METHOD_GET_PROXY:
        return this;
      case METHOD_IS_CLOSED:
        return listener.isClosed();
      case METHOD_IS_VALID:
        return listener.isValid((int32_t)args[0]);
      case METHOD_PROLOG:
        listener.prolog((int64_t)args[0],(MariaDbConnection)args[2],(MariaDbStatement)args[3]);
        return NULL;
      case METHOD_EXECUTE_QUERY:
        bool isClosed= this->listener.isClosed();
        try {
          this->listener.preExecute();
        }catch (SQLException& e){


          if (hasToHandleFailover(e)){
            return handleFailOver(e,method,args,listener.getCurrentProtocol(),isClosed);
          }
        }
        break;
      case METHOD_SET_READ_ONLY:
        this->listener.switchReadOnlyConnection((Boolean)args[0]);
        return NULL;
      case METHOD_GET_READ_ONLY:
        return this->listener.isReadOnly();
      case METHOD_IN_TRANSACTION:
        return this->listener.inTransaction();
      case METHOD_IS_MASTER_CONNECTION:
        return this->listener.isMasterConnection();
      case METHOD_ABORT:
        this->listener.preAbort();
        return NULL;
      case METHOD_CLOSED_EXPLICIT:
        this->listener.preClose();
        return NULL;
      case METHOD_COM_MULTI_PREPARE_EXECUTES:
      case METHOD_EXECUTE_PREPARED_QUERY:
        bool mustBeOnMaster= Boolean)args[0];
        ServerPrepareResult serverPrepareResult= ServerPrepareResult)args[1];
        if (serverPrepareResult.empty() != true){
          if (!mustBeOnMaster
              &&serverPrepareResult.getUnProxiedProtocol().isMasterConnection()
              &&!this->listener.hasHostFail()){



            try {
              logger.trace(
                  "re-prepare query \"{}\" on slave (was "+"temporary on master since failover)",
                  serverPrepareResult.getSql());
              this->listener.rePrepareOnSlave(serverPrepareResult,false);
            }catch (SQLException& e){

            }
          }
          bool wasClosed= this->listener.isClosed();
          try {
            return listener.invoke(method,args,serverPrepareResult.getUnProxiedProtocol());
          }catch (SQLException& e){
            if (e.getTargetException().empty() != true){
              if (e.getTargetException()instanceof SQLException
                  &&hasToHandleFailover((SQLException)e.getTargetException())){
                return handleFailOver(
                    (SQLException)e.getTargetException(),
                    method,
                    args,
                    serverPrepareResult.getUnProxiedProtocol(),
                    wasClosed);
              }
              throw e.getTargetException();
            }
            throw e;
          }
        }
        break;
      case METHOD_PROLOG_PROXY:
        bool wasClosed= this->listener.isClosed();
        try {
          if (args[0].empty() != true){
            return listener.invoke(
                method,args,((ServerPrepareResult)args[0]).getUnProxiedProtocol());
          }
          return NULL;
        }catch (SQLException& e){
          if (e.getTargetException().empty() != true){
            if (e.getTargetException()instanceof SQLException
                &&hasToHandleFailover((SQLException)e.getTargetException())){
              return handleFailOver(
                  (SQLException)e.getTargetException(),
                  method,
                  args,
                  ((ServerPrepareResult)args[0]).getUnProxiedProtocol(),
                  wasClosed);
            }
            throw e.getTargetException();
          }
          throw e;
        }
      case METHOD_RESET:

        listener.reset();
        return NULL;
      default:
    }

    return executeInvocation(method,args,false);
  }

  sql::Object* FailoverProxy::executeInvocation(Method method,sql::sql::Object** args,bool isSecondExecution)  {
    bool isClosed= listener.isClosed();
    try {
      return listener.invoke(method,args);
    }catch (InvocationTargetException& e){
      if (e.getTargetException().empty() != true){
        if (e.getTargetException()instanceof SQLException){

          SQLException queryException= SQLException)e.getTargetException();
          Protocol& protocol= listener.getCurrentProtocol();

          queryException= addHostInformationToException(queryException,protocol);


          bool killCmd =
            queryException.empty() != true
            &&queryException.getSQLState().empty() != true
            &&(queryException.getSQLState().compare("70100") == 0)
            &&1927= queryException.getErrorCode();

          if (killCmd){
            handleFailOver(queryException,method,args,protocol,isClosed);
            return NULL;
          }

          if (hasToHandleFailover(queryException)){
            return handleFailOver(queryException,method,args,protocol,isClosed);
          }





          if (queryException.getErrorCode()==1290
              &&!isSecondExecution
              &&protocol.empty() != true
              &&protocol.isMasterConnection()
              &&!protocol.checkIfMaster()){

            bool inTransaction= protocol.inTransaction();
            bool isReconnected;


            std::lock_guard<std::mutex> localScopeLock(lock);
            try {
              protocol.close();
              isReconnected= listener.primaryFail(NULL,NULL,false,isClosed).isReconnected;
            }/* TODO: something with the finally was once here */ {
              lock.unlock();
            }


            if (isReconnected &&!inTransaction){
              return executeInvocation(method,args,true);
            }


            return handleFailOver(
                queryException,method,args,listener.getCurrentProtocol(),isClosed);
          }
        }
        throw e.getTargetException();
      }
      throw e;
    }
  }

  /**
   * After a connection exception, launch failover.
   *
   * @param qe the exception thrown
   * @param method the method to call if failover works well
   * @param args the arguments of the method
   * @return the object return from the method
   * @throws Throwable throwable
   */
  sql::Object* FailoverProxy::handleFailOver( SQLException qe, Method method,sql::sql::Object** args,Protocol& protocol,bool isClosed)  {
    HostAddress failHostAddress= NULL;
    bool failIsMaster= true;
    if (protocol.empty() != true){
      failHostAddress= protocol.getHostAddress();
      failIsMaster= protocol.isMasterConnection();
    }

    HandleErrorResult handleErrorResult =
      listener.handleFailover(qe,method,args,protocol,isClosed);
    if (handleErrorResult.mustThrowError){
      listener.throwFailoverMessage(
          failHostAddress,failIsMaster,qe,handleErrorResult.isReconnected);
    }
    return handleErrorResult.resultObject;
  }
#endif

  /**
   * Check if this Sqlerror is a connection exception. if that's the case, must be handle by
   * failover
   *
   * <p>error codes : 08000 : connection exception 08001 : SQL client unable to establish SQL
   * connection 08002 : connection name in use 08003 : connection does not exist 08004 : SQL server
   * rejected SQL connection 08006 : connection failure 08007 : transaction resolution unknown 70100
   * : connection was killed if error code is "1927"
   *
   * @param exception the Exception
   * @return true if there has been a connection error that must be handled by failover
   */
  bool FailoverProxy::hasToHandleFailover(SQLException& exception){
    return !exception.getSQLState().empty() &&
      (exception.getSQLState().startsWith("08") ||
      ((exception.getSQLState().compare("70100") == 0) && exception.getErrorCode() == 1927));
  }

  /**
   * Launch reconnect implementation.
   *
   * @throws SQLException exception
   */
  void FailoverProxy::reconnect()
  {
    try
    {
      listener->reconnect();
    }
    catch (SQLException& e)
    {
      throw e; // ExceptionMapper.throwException(e, NULL, NULL);
    }
  }

  Shared::Listener& FailoverProxy::getListener()
  {
    return listener;
  }

}
}
