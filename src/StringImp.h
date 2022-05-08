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

#ifndef _STRINGIMP_H_
#define _STRINGIMP_H_

#include <string>

#include "SQLString.hpp"

namespace sql
{
class StringImp
{
  std::string realStr;

public:
  static std::string& get(SQLString& str);
  static const std::string& get(const SQLString& str);

  StringImp(const char* str);
  StringImp(const char* str, std::size_t count);
  StringImp()=default; //or delete?
  ~StringImp()=default;

  std::string* operator ->() { return &realStr; }

  std::string& get() { return realStr; }
};

}
#endif
