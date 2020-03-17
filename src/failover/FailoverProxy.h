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


#ifndef _FAILOVERPROXY_H_
#define _FAILOVERPROXY_H_

#include <mutex>

#include "Consts.h"
#include "Protocol.h"

#include "Listener.h"

namespace sql
{
namespace mariadb
{
//  class Listener;

class FailoverProxy {

  static const SQLString METHOD_IS_EXPLICIT_CLOSED ; /*"isExplicitClosed"*/
  static const SQLString METHOD_GET_OPTIONS ; /*"getOptions"*/
  static const SQLString METHOD_GET_URLPARSER ; /*"getUrlParser"*/
  static const SQLString METHOD_GET_PROXY ; /*"getProxy"*/
  static const SQLString METHOD_EXECUTE_QUERY ; /*"executeQuery"*/
  static const SQLString METHOD_SET_READ_ONLY ; /*"setReadonly"*/
  static const SQLString METHOD_GET_READ_ONLY ; /*"getReadonly"*/
  static const SQLString METHOD_IS_MASTER_CONNECTION ; /*"isMasterConnection"*/
  static const SQLString METHOD_VERSION_GREATER_OR_EQUAL ; /*"versionGreaterOrEqual"*/
  static const SQLString METHOD_SESSION_STATE_AWARE ; /*"sessionStateAware"*/
  static const SQLString METHOD_CLOSED_EXPLICIT ; /*"closeExplicit"*/
  static const SQLString METHOD_ABORT ; /*"abort"*/
  static const SQLString METHOD_IS_CLOSED ; /*"isClosed"*/
  static const SQLString METHOD_EXECUTE_PREPARED_QUERY ; /*"executePreparedQuery"*/
  static const SQLString METHOD_COM_MULTI_PREPARE_EXECUTES ; /*"prepareAndExecutesComMulti"*/
  static const SQLString METHOD_PROLOG_PROXY ; /*"prologProxy"*/
  static const SQLString METHOD_RESET ; /*"reset"*/
  static const SQLString METHOD_IS_VALID ; /*"isValid"*/
  static const SQLString METHOD_GET_LOCK ; /*"getLock"*/
  static const SQLString METHOD_GET_NO_BACKSLASH ; /*"noBackslashEscapes"*/
  static const SQLString METHOD_GET_SERVER_THREAD_ID ; /*"getServerThreadId"*/
  static const SQLString METHOD_PROLOG ; /*"prolog"*/
  static const SQLString METHOD_GET_CATALOG ; /*"getCatalog"*/
  static const SQLString METHOD_GET_TIMEOUT ; /*"getTimeout"*/
  static const SQLString METHOD_GET_MAJOR_VERSION ; /*"getMajorServerVersion"*/
  static const SQLString METHOD_IN_TRANSACTION ; /*"inTransaction"*/
  static const SQLString METHOD_IS_MARIADB ; /*"isServerMariaDb"*/
  static const Shared::Logger logger ; /*LoggerFactory.getLogger(FailoverProxy.class)*/

  Shared::Listener listener;
  static SQLException addHostInformationToException( SQLException& exception, Shared::Protocol& protocol);

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /* In this case unlikely implementation, but translating to something usable */
  sql::Object* executeInvocation(Method method,sql::sql::Object** args,bool isSecondExecution);
  sql::Object* handleFailOver( SQLException qe, Method method, sql::sql::Object** args, Protocol& protocol, bool isClosed);
#endif

public:
  Shared::mutex lock; /* Weak? */

  FailoverProxy(Listener* listener, std::mutex* lock);

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  sql::Object* invoke(sql::Object* proxy,Method method,sql::sql::Object** args);
#endif

  bool hasToHandleFailover(SQLException& exception);
  void reconnect();
  Shared::Listener& getListener();
};

}
}
#endif
