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


#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <memory>
#include <stdexcept>
#include "SQLString.h"

namespace sql
{
class /*MARIADB_EXPORTED*/ SQLException : public std::runtime_error
{
  SQLString SqlState;
  int32_t ErrorCode;
  std::shared_ptr<std::exception> Cause;
public:
  SQLException();

  SQLException& operator=(const SQLException &)=default;
  SQLException(const SQLException&)=default;
  MARIADB_EXPORTED virtual ~SQLException();
  MARIADB_EXPORTED SQLException(const SQLString& msg);
  //SQLException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception *e= NULL);
  SQLException(const char* msg, const char* state, int32_t error=0, const std::exception *e= NULL);
  MARIADB_EXPORTED SQLException* getNextException();
  void setNextException(sql::SQLException& nextException);
  MARIADB_EXPORTED SQLString getSQLState() { return SqlState.c_str(); }
  MARIADB_EXPORTED const char* getSQLStateCStr() { return SqlState.c_str(); }
  MARIADB_EXPORTED int32_t getErrorCode();
  MARIADB_EXPORTED SQLString getMessage();
  MARIADB_EXPORTED std::exception* getCause() const;
  MARIADB_EXPORTED char const* what() const noexcept override { return std::runtime_error::what(); }
};


class SQLFeatureNotImplementedException: public SQLException
{
  void operator=(const SQLFeatureNotImplementedException&) {}
  SQLFeatureNotImplementedException() {}

public:

//  SQLFeatureNotImplementedException(const SQLFeatureNotImplementedException&) {}
  SQLFeatureNotImplementedException(const SQLString& msg) : SQLException(msg) {}
  virtual ~SQLFeatureNotImplementedException(){}
};

typedef SQLFeatureNotImplementedException MethodNotImplementedException;

class IllegalArgumentException : public SQLException
{
  //IllegalArgumentException(const IllegalArgumentException&);
  void operator=(const IllegalArgumentException&);
  IllegalArgumentException() {}

public:

  virtual ~IllegalArgumentException() {}
  IllegalArgumentException(const SQLString& msg, const char* sqlState= nullptr, int32_t error= 0) : SQLException(msg.c_str(), sqlState, error) {}
};

typedef IllegalArgumentException InvalidArgumentException;

class SQLFeatureNotSupportedException : public SQLException
{
  void operator=(const SQLFeatureNotSupportedException&) {}
  SQLFeatureNotSupportedException() {}

public:

  SQLFeatureNotSupportedException(const SQLString& msg) : SQLException(msg) {}
  SQLFeatureNotSupportedException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= NULL) :
    SQLException(msg, state, error, e)
  {}
  virtual ~SQLFeatureNotSupportedException() {}
};


class SQLTransientConnectionException : public SQLException
{
  void operator=(const SQLTransientConnectionException&)=delete;
  SQLTransientConnectionException()=delete;

public:

  SQLTransientConnectionException(const SQLString& msg) : SQLException(msg) {}
  virtual ~SQLTransientConnectionException() {}
  SQLTransientConnectionException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= NULL) :
    SQLException(msg, state, error, e)
  {}
};


class SQLNonTransientConnectionException : public SQLException
{
  void operator=(const SQLNonTransientConnectionException&) {}
  SQLNonTransientConnectionException() {}

public:

  SQLNonTransientConnectionException(const SQLString& msg) : SQLException(msg) {}
  virtual ~SQLNonTransientConnectionException() {}
  SQLNonTransientConnectionException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= NULL) :
    SQLException(msg, state, error, e)
  {}
};


class SQLTransientException : public SQLException
{
  void operator=(const SQLTransientException&) {}
  SQLTransientException() {}

public:

  SQLTransientException(const SQLString& msg) : SQLException(msg) {}
  virtual ~SQLTransientException() {}
  SQLTransientException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= NULL) :
    SQLException(msg, state, error, e)
  {}
};

class SQLSyntaxErrorException : public SQLException
{
  void operator=(const SQLSyntaxErrorException&) {}
  SQLSyntaxErrorException() {}

public:

  SQLSyntaxErrorException(const SQLString& msg) : SQLException(msg) {}
  virtual ~SQLSyntaxErrorException() {}
  SQLSyntaxErrorException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= NULL) :
    SQLException(msg, state, error, e)
  {}
};

class SQLTimeoutException : public SQLException
{
  void operator=(const SQLTimeoutException&) {}
  SQLTimeoutException() {}

public:

  SQLTimeoutException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= NULL) :
    SQLException(msg, state, error, e)
  {}
  virtual ~SQLTimeoutException() {}
};


class BatchUpdateException : public SQLException
{
  void operator=(const BatchUpdateException&) {}
  BatchUpdateException() {}

public:

  BatchUpdateException(const SQLString& msg, const SQLString& state, int32_t error= 0, int64_t* updCts= NULL, const std::exception *e= NULL) :
    SQLException(msg, state, error, e)
  {}
  virtual ~BatchUpdateException() {}
};


class SQLDataException : public SQLException
{
  void operator=(const SQLDataException&) {}
  SQLDataException() {}

public:

  SQLDataException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception *e= NULL) :
    SQLException(msg, state, error, e)
  {}
  virtual ~SQLDataException() {}
};


class SQLIntegrityConstraintViolationException : public SQLException
{
  void operator=(const SQLIntegrityConstraintViolationException&) {}
  SQLIntegrityConstraintViolationException() {}

public:

  SQLIntegrityConstraintViolationException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception *e= NULL) :
    SQLException(msg, state, error, e)
  {}
  virtual ~SQLIntegrityConstraintViolationException() {}
};


class SQLInvalidAuthorizationSpecException : public SQLException
{
  void operator=(const SQLInvalidAuthorizationSpecException&) {}
  SQLInvalidAuthorizationSpecException() {}

public:

  SQLInvalidAuthorizationSpecException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception *e= NULL) :
    SQLException(msg, state, error, e)
  {}
  virtual ~SQLInvalidAuthorizationSpecException() {}
};


class SQLTransactionRollbackException : public SQLException
{
  void operator=(const SQLTransactionRollbackException&) {}
  SQLTransactionRollbackException() {}

public:

  SQLTransactionRollbackException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception *e= NULL) :
    SQLException(msg, state, error, e)
  {}
  virtual ~SQLTransactionRollbackException() {}
};


class ParseException : public SQLException
{
  void operator=(const ParseException&) {}
  ParseException() : position(0) {}

public:
  std::size_t position;
  ParseException(const SQLString& str, std::size_t pos= 0) :
    SQLException(str), position(pos)
  {}
  virtual ~ParseException() {}
};

class MaxAllowedPacketException : public std::runtime_error {

  bool mustReconnect;

public:
  MaxAllowedPacketException(const char* message, bool _mustReconnect)
    : std::runtime_error(message)
    , mustReconnect(_mustReconnect)
  {
  }

  bool isMustReconnect() {
    return mustReconnect;
  }
};

}
#endif