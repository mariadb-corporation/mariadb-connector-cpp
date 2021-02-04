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


#include <cctype>
#include <regex>
#include <string>

#include "SQLString.hpp"

namespace sql
{

////////////////////// Standalone SQLString util functions /////////////////////////////

namespace mariadb
{
  typedef std::unique_ptr<std::vector<SQLString>> Tokens;

  sql::mariadb::Tokens split(const SQLString& str, const SQLString & delimiter);

  sql::SQLString& replace(SQLString& str, const SQLString& substr, const SQLString& subst);

  sql::SQLString replace(const SQLString& str, const SQLString& substr, const SQLString& substa);

  sql::SQLString& replaceAll(SQLString& str, const SQLString& substr, const SQLString &subst);

  sql::SQLString replaceAll(const SQLString& str, const SQLString& substr, const SQLString &subst);

  bool equalsIgnoreCase(const SQLString& str1, const SQLString& str2);

  uint64_t stoull(const SQLString& str, std::size_t* pos= nullptr);
  uint64_t stoull(const char* str, std::size_t len= -1, std::size_t* pos = nullptr);
}
}
