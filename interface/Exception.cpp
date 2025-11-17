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


#include "Exception.h"

namespace mariadb
{
  /******************** SQLException ***************************/
  SQLException::~SQLException()
  {}

  SQLException::SQLException() : std::runtime_error(""), ErrorCode(0)
  {}

  SQLException::SQLException(const SQLException& other) :
    std::runtime_error(other),
    SqlState(other.SqlState),
    ErrorCode(other.ErrorCode)
  {}

  SQLException::SQLException(const char* msg, const char* state, int32_t error, const std::exception *e) : std::runtime_error(msg),
    SqlState(state), ErrorCode(error)
  {}

  SQLException::SQLException(const SQLString& msg, const SQLString& state, int32_t error, const std::exception* e) : SQLException(msg.c_str(), state.c_str(), error, e)
  {}

  SQLException::SQLException(const SQLString& msg): SQLException(msg.c_str(), "", 0)
  {}


  SQLString SQLException::getMessage()
  {
    return what();
  }


  char const* SQLException::what() const noexcept
  {
    return std::runtime_error::what();
  }

} // namespace mariadb
