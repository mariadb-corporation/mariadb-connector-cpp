/************************************************************************************
   Copyright (C) 2020,2021 MariaDB Corporation AB

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


/* MariaDB Connector/C++ Generally used consts, types definitions, enums,  macros, small classes, templates */

#ifndef __CONSTS_H_
#define __CONSTS_H_

#include <map>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <sstream>

#include "StringImp.h"
#include "Version.h"
#include "util/ServerStatus.h"
#include "util/String.h"
#include "CArrayImp.h"

#include "ResultSet.hpp"
#include "CallableStatement.hpp"
#include "Statement.hpp"
#include "Connection.hpp"
#include "ResultSetMetaData.hpp"
#include "ParameterMetaData.hpp"

#include "Charset.h"
#include "MariaDbServerCapabilities.h"
#include "MariaDBException.h"

#include "options/Options.h"
#include "logger/Logger.h"

/* Helper to write ClassField map initializer list */
#define CLASS_FIELD(_CLASS, _FIELD) {#_FIELD, &_CLASS::_FIELD}
#define INSTANCEOF(OBJ, CLASSNAME) (OBJ != nullptr && dynamic_cast<CLASSNAME>(OBJ) != NULL)

namespace sql
{
namespace mariadb
{
  enum HaMode
  {
    NONE=0,
    AURORA,
    REPLICATION,
    SEQUENTIAL,
    LOADBALANCE
  };

  enum ColumnFlags {
    NOT_NULL = 1,
    PRIMARY_KEY = 2,
    UNIQUE_KEY = 4,
    MULTIPLE_KEY = 8,
    BLOB = 16,
    UNSIGNED = 32,
    DECIMAL = 64,
    BINARY_COLLATION = 128,
    ENUM = 256,
    AUTO_INCREMENT = 512,
    TIMESTAMP = 1024,
    SET = 2048
  };

  /* Also probably temporary location. Using it as it's generally included by everybody */
  /* typedefs for shared_ptr types, so the code looks nicer and cleaner */
  class Pool;
  class Protocol;
  class Listener;
  class MariaDbConnection;
  class MariaDbStatement;
  class Results;
  class CmdInformation;
  class Buffer;
  class ClientPrepareResult;
  class ClientSidePreparedStatement;
  class ServerSidePreparedStatement;
  class SelectResultSet;
  //typedef ServerSidePreparedStatement ClientSidePreparedStatement;
  typedef SelectResultSet UpdatableResultSet;
  class ServerPrepareResult;
  class MariaDbParameterMetaData;
  class MariaDbResultSetMetaData;
  class CallableParameterMetaData;
  class ColumnDefinition;
  class Credential;
  class ParameterHolder;
  class RowProtocol;
  class SelectResultSet;
  class ExceptionFactory;

  class CloneableCallableStatement : public CallableStatement
  {
    CloneableCallableStatement(const CloneableCallableStatement&)=delete;
    void operator=(CloneableCallableStatement&)=delete;
  public:
    CloneableCallableStatement()=default;
    virtual ~CloneableCallableStatement() {}
    virtual CloneableCallableStatement* clone(MariaDbConnection* connection)=0;
  };


  extern std::map<std::string, enum HaMode> StrHaModeMap;
  extern const char* HaModeStrMap[LOADBALANCE + 1];
  extern const SQLString emptyStr;
  extern const SQLString localhost;
  extern const char QUOTE;
  extern const char DBL_QUOTE;
  extern const char ZERO_BYTE;
  extern const char BACKSLASH;


  struct ParameterConstant
  {
    static const SQLString TYPE_MASTER;// = "master";
    static const SQLString TYPE_SLAVE;// = "slave";
  };


  /* Temporary here, need to find better place for it */
  template <class T> int64_t hash(T v)
  {
    return static_cast<int64_t>(std::hash<T>{}(v));
  }

#define HASHCODE(_expr) std::hash<decltype(_expr)>{}(_expr)

  template <class KT, class VT> int64_t hashMap(std::map<KT, VT>& v)
  {
    int64_t result= 0;
    for (auto it : v)
    {
      /* I don't think it's guaranteed, that pairs returned in same order, and we need the same value for same map content */
      result+= hash<KT>(it.first) ^ (hash<KT>(it.second) << 1);
    }

    return result;
  }

  namespace Shared
  {
    typedef std::shared_ptr<std::mutex> mutex;

    typedef std::shared_ptr<sql::mariadb::Options> Options;
    typedef std::shared_ptr<sql::mariadb::Logger> Logger;
    typedef std::shared_ptr<sql::mariadb::Pool> Pool;
    typedef std::shared_ptr<sql::mariadb::Protocol> Protocol;
    typedef std::shared_ptr<sql::mariadb::Listener> Listener;
    typedef std::shared_ptr<sql::mariadb::CmdInformation> CmdInformation;
    typedef std::shared_ptr<sql::mariadb::Results> Results;
    typedef std::shared_ptr<sql::mariadb::Buffer> Buffer;
    typedef std::shared_ptr<sql::mariadb::ClientPrepareResult> ClientPrepareResult;
    typedef std::shared_ptr<sql::mariadb::ServerPrepareResult> ServerPrepareResult;
    typedef std::shared_ptr<sql::mariadb::MariaDbResultSetMetaData> MariaDbResultSetMetaData;
    typedef std::shared_ptr<sql::mariadb::MariaDbParameterMetaData> MariaDbParameterMetaData;
    typedef std::shared_ptr<sql::mariadb::CallableParameterMetaData> CallableParameterMetaData;
    typedef std::shared_ptr<sql::mariadb::ColumnDefinition> ColumnDefinition;
    typedef std::shared_ptr<sql::mariadb::ParameterHolder> ParameterHolder;
    typedef std::shared_ptr<sql::mariadb::SelectResultSet> SelectResultSet;
    typedef std::shared_ptr<sql::mariadb::ExceptionFactory> ExceptionFactory;
    /* Shared sql classes - there is probably no sense to keep them in sql::Shared */
    typedef std::shared_ptr<sql::CallableStatement> CallableStatement;
    typedef std::shared_ptr<sql::ResultSetMetaData> ResultSetMetaData;
    typedef std::shared_ptr<sql::Connection> Connection;
    typedef std::shared_ptr<sql::ParameterMetaData> ParameterMetaData;
  }

  namespace Unique
  {
    /* Unique sql classes */
    /* atm I doubt we need these three */
    typedef std::unique_ptr<sql::Statement> Statement;
    typedef std::unique_ptr<sql::ResultSet> ResultSet;
    typedef std::unique_ptr<sql::SQLException> SQLException;
    typedef std::unique_ptr<sql::mariadb::MariaDbStatement> MariaDbStatement;
    typedef std::unique_ptr<sql::mariadb::CallableParameterMetaData> CallableParameterMetaData;
    typedef std::unique_ptr<sql::mariadb::Credential> Credential;
    typedef std::unique_ptr<sql::mariadb::Results> Results;
    typedef std::unique_ptr<sql::mariadb::RowProtocol> RowProtocol;
    typedef std::unique_ptr<sql::mariadb::SelectResultSet> SelectResultSet;

    typedef std::unique_ptr<sql::mariadb::ServerSidePreparedStatement> ServerSidePreparedStatement;
    typedef std::unique_ptr<sql::mariadb::ClientSidePreparedStatement> ClientSidePreparedStatement;
    typedef std::unique_ptr<sql::mariadb::ServerPrepareResult> ServerPrepareResult;
  }

  namespace Weak
  {
    typedef std::weak_ptr<MariaDbConnection> MariaDbConnection;
    typedef std::weak_ptr<Results> Results;
  }
} //---- namespace mariadb

  //namespace Shared
  //{
  //}
  //namespace Unique
  //{
  //}
} //---- namespave sql

//#include "UrlParser.h"

#endif
