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

  StringImp(const char* str, std::size_t count);

public:
  static std::string& get(SQLString& str);
  static const std::string& get(const SQLString& str);

  
  StringImp()=delete; //or delete?
  ~StringImp()=default;

  std::string* operator ->() { return &realStr; }

  std::string& get() { return realStr; }

  static StringImp* createString(const SQLString& str);
  static StringImp* createString(const char* str, std::size_t count);
  static StringImp* createString(const char* str);
  static void deleteString(SQLString& str);
  static SQLString& copyString(SQLString& to, const SQLString& from);
  static SQLString& copyString(SQLString& to, const char* from);
  static SQLString& appendString(SQLString& to, const SQLString& from);
  static SQLString& appendString(SQLString& to, const char* from);
  static SQLString& appendString(SQLString& to, const char* from, std::size_t len);
  static SQLString& appendString(SQLString& to, char c);
  static void reserveSize(SQLString& str, std::size_t requiredCapacity);
  static bool isNull(const SQLString& str);
};

}
#endif
