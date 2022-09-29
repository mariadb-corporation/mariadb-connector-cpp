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


#include "MariaDBException.h"

namespace sql
{
  /******************** SQLException ***************************/
  SQLException::~SQLException()
  {}

  SQLException::SQLException() : std::runtime_error(""), ErrorCode(0)
  {}

  SQLException::SQLException(const SQLException& other) :
    std::runtime_error(other),
    SqlState(other.SqlState),
    ErrorCode(other.ErrorCode),
    Cause(other.Cause)
  {}

  SQLException::SQLException(const char* msg, const char* state, int32_t error, const std::exception *e) : std::runtime_error(msg),
    SqlState(state), ErrorCode(error)
  {}

  SQLException::SQLException(const SQLString& msg): SQLException(msg.c_str(), "", 0)
  {}

  SQLException* SQLException::getNextException()
  {
    return nullptr;
  }

  char const* SQLException::what() const noexcept
  {
    return std::runtime_error::what();
  }

  void SQLException::setNextException(sql::SQLException& nextException)
  {
    /* Doing nothing so far */
  }


  SQLString SQLException::getMessage()
  {
    return this->what();
  }

  std::exception* SQLException::getCause() const
  {
    if (Cause)
    {
      return Cause.get();
    }
    else
    {
      return nullptr;
    }
  }

  /******************** SQLFeatureNotImplementedException ***************************/
  SQLFeatureNotImplementedException::~SQLFeatureNotImplementedException()
  {}

  SQLFeatureNotImplementedException::SQLFeatureNotImplementedException(const SQLFeatureNotImplementedException& other) :
    SQLException(other)
  {}

  SQLFeatureNotImplementedException::SQLFeatureNotImplementedException(const SQLString& msg)
    : SQLException(msg)
  {}

  /******************** IllegalArgumentException ***************************/
  IllegalArgumentException::~IllegalArgumentException()
  {}

  IllegalArgumentException::IllegalArgumentException(const IllegalArgumentException& other) :
    SQLException(other)
  {}

  IllegalArgumentException::IllegalArgumentException(const SQLString& msg, const char* sqlState, int32_t error) :
    SQLException(msg.c_str(), sqlState, error)
  {}

  /******************** SQLFeatureNotSupportedException ***************************/
  SQLFeatureNotSupportedException::~SQLFeatureNotSupportedException()
  {}

  SQLFeatureNotSupportedException::SQLFeatureNotSupportedException(const SQLFeatureNotSupportedException& other) :
    SQLException(other)
  {}

  SQLFeatureNotSupportedException::SQLFeatureNotSupportedException(const SQLString& msg) :
    SQLException(msg)
  {}

  SQLFeatureNotSupportedException::SQLFeatureNotSupportedException(const SQLString& msg, const char* state, int32_t error, const std::exception* e) :
    SQLException(msg.c_str(), state, error, e)
  {}

  /******************** SQLTransientConnectionException ***************************/
  SQLTransientConnectionException::~SQLTransientConnectionException()
  {}

  SQLTransientConnectionException::SQLTransientConnectionException(const SQLTransientConnectionException& other) :
    SQLException(other)
  {}

  SQLTransientConnectionException::SQLTransientConnectionException(const SQLString& msg) :
    SQLException(msg)
  {}

  SQLTransientConnectionException::SQLTransientConnectionException(const SQLString& msg, const SQLString& state, int32_t error, const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** SQLNonTransientConnectionException ***************************/
  SQLNonTransientConnectionException::~SQLNonTransientConnectionException()
  {}

  SQLNonTransientConnectionException::SQLNonTransientConnectionException(const SQLNonTransientConnectionException& other) :
    SQLException(other)
  {}

  SQLNonTransientConnectionException::SQLNonTransientConnectionException(const SQLString& msg)
    : SQLException(msg)
  {}

  SQLNonTransientConnectionException::SQLNonTransientConnectionException(const SQLString& msg, const SQLString& state, int32_t error, const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** SQLTransientException ***************************/
  SQLTransientException::~SQLTransientException()
  {}

  SQLTransientException::SQLTransientException(const SQLTransientException& other) :
    SQLException(other)
  {}

  SQLTransientException::SQLTransientException(const SQLString& msg)
    : SQLException(msg)
  {}

  SQLTransientException::SQLTransientException(const SQLString& msg, const SQLString& state, int32_t error, const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** SQLSyntaxErrorException ***************************/
  SQLSyntaxErrorException::~SQLSyntaxErrorException()
  {}

  SQLSyntaxErrorException::SQLSyntaxErrorException(const SQLString& msg) :
    SQLException(msg)
  {}

  SQLSyntaxErrorException::SQLSyntaxErrorException(const SQLString& msg, const SQLString& state, int32_t error, const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  SQLSyntaxErrorException::SQLSyntaxErrorException(const SQLSyntaxErrorException& other) : SQLException(other) {}

  /******************** SQLTimeoutException ***************************/
  SQLTimeoutException::~SQLTimeoutException()
  {}

  SQLTimeoutException::SQLTimeoutException(const SQLTimeoutException& other) :
    SQLException(other)
  {}

  SQLTimeoutException::SQLTimeoutException(const SQLString& msg, const SQLString& state, int32_t error, const std::exception* e) :
    SQLException(msg, state, error, e)
  {}
  
  /******************** BatchUpdateException ***************************/
  BatchUpdateException::~BatchUpdateException()
  {}

  BatchUpdateException::BatchUpdateException(const BatchUpdateException& other) :
    SQLException(other)
  {}

  BatchUpdateException::BatchUpdateException(const SQLString& msg, const SQLString& state, int32_t error, int64_t* updCts, const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** SQLDataException ***************************/
  SQLDataException::~SQLDataException()
  {}

  SQLDataException::SQLDataException(const SQLDataException& other) :
    SQLException(other)
  {}

  SQLDataException::SQLDataException(const SQLString& msg, const SQLString& state, int32_t error, const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** SQLIntegrityConstraintViolationException ***************************/
  SQLIntegrityConstraintViolationException::~SQLIntegrityConstraintViolationException()
  {}

  SQLIntegrityConstraintViolationException::SQLIntegrityConstraintViolationException(const SQLIntegrityConstraintViolationException& other) :
    SQLException(other)
  {}

  SQLIntegrityConstraintViolationException::SQLIntegrityConstraintViolationException(const SQLString& msg, const SQLString& state, int32_t error,
                                                                                     const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** SQLInvalidAuthorizationSpecException ***************************/
  SQLInvalidAuthorizationSpecException::~SQLInvalidAuthorizationSpecException()
  {}

  SQLInvalidAuthorizationSpecException::SQLInvalidAuthorizationSpecException(const SQLInvalidAuthorizationSpecException& other) :
    SQLException(other)
  {}

  SQLInvalidAuthorizationSpecException::SQLInvalidAuthorizationSpecException(const SQLString& msg, const SQLString& state, int32_t error,
                                                                             const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** SQLTransactionRollbackException ***************************/
  SQLTransactionRollbackException::~SQLTransactionRollbackException()
  {}

  SQLTransactionRollbackException::SQLTransactionRollbackException(const SQLTransactionRollbackException& other) :
    SQLException(other)
  {}

  SQLTransactionRollbackException::SQLTransactionRollbackException(const SQLString& msg, const SQLString& state, int32_t error,
                                                                   const std::exception* e) :
    SQLException(msg, state, error, e)
  {}

  /******************** ParseException ***************************/
  ParseException::~ParseException()
  {}

  ParseException::ParseException(const ParseException& other) :
    SQLException(other), position(other.position)
  {}

  ParseException::ParseException(const SQLString& str, std::size_t pos) :
    SQLException(str), position(pos)
  {}

  /********************* MaxAllowedPacketException **************************/
  // Not sure this class really needs to be open to users
  MaxAllowedPacketException::~MaxAllowedPacketException()
  {}

  MaxAllowedPacketException::MaxAllowedPacketException(const MaxAllowedPacketException& other) :
    std::runtime_error(other), mustReconnect(other.mustReconnect)
  {}

  MaxAllowedPacketException::MaxAllowedPacketException(const char* message, bool _mustReconnect)
    : std::runtime_error(message)
    , mustReconnect(_mustReconnect)
  {
  }

  /////////////////////////////
  MariaDBExceptionThrower::MariaDBExceptionThrower(MariaDBExceptionThrower&& moved) noexcept : exceptionThrower(std::move(moved.exceptionThrower))
  {}

  void MariaDBExceptionThrower::assign(MariaDBExceptionThrower other) {
    exceptionThrower.reset(other.release());
  }
}
