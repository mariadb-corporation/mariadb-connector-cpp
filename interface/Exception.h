/************************************************************************************
   Copyright (C) 2023 MariaDB Corporation AB

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

#include <cstdint>
#include <stdexcept>
#include "class/SQLString.h"

namespace mariadb
{
class SQLException : public ::std::runtime_error
{
  SQLString SqlState;
  int32_t ErrorCode;

public:
  virtual ~SQLException();

  SQLException& operator=(const SQLException &)= default;
  
  SQLException();
  SQLException(const SQLException&);
  SQLException(const SQLString& msg);
  SQLException(const char* msg, const char* state, int32_t error=0, const std::exception *e= nullptr);
  SQLException(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= nullptr);
  //SQLException(const SQLString& msg, const char* state, int32_t error, const std::exception* e= nullptr);

  SQLString getSQLState() { return SqlState.c_str(); }
  const char* getSQLStateCStr() { return SqlState.c_str(); }
  int32_t getErrorCode() { return ErrorCode; }
  SQLString getMessage();
  char const* what() const noexcept override;
};

}
#endif
