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

#include <cstring>
#include "StringImp.h"

namespace sql
{
  static std::unique_ptr<StringImp> nullString(StringImp::createString("", 0));
  static StringImp* nullStr= nullString.get();

  bool StringImp::isNull(const SQLString& str)
  {
    return (str.theString == nullStr);
  }


  StringImp* StringImp::createString(const SQLString& str) {
    if (!isNull(str)) {
      return new StringImp(str.theString->realStr.c_str(), str.theString->realStr.length());
    }
    return nullStr;
  }


  StringImp* StringImp::createString(const char* str, std::size_t count) {
    if (str) {
      return new StringImp(str, count);
    }
    return nullStr;
  }


  StringImp* StringImp::createString(const char* str) {
    if (str) {
      return new StringImp(str, std::strlen(str));
    }
    return nullStr;
  }


  void StringImp::deleteString(SQLString& str) {
    if (!isNull(str)) {
      delete str.theString;
    }
  }


  SQLString& StringImp::copyString(SQLString& to, const SQLString& from)
  {
    if (isNull(to)) {
      if (!isNull(from)) {
        to.theString= createString(from.theString->realStr.c_str(), from.theString->realStr.length());
      }
    }
    else {
      // If from is Null, to won't be
      to.theString->realStr.assign(from.theString->realStr);
    }
    return to;
  }


  SQLString& StringImp::copyString(SQLString& to, const char* from)
  {
    if (isNull(to)) {
      if (from) {
        to.theString= createString(from);
      }
    }
    else {
      to.theString->realStr.assign(from);
    }
    return to;
  }


  SQLString& StringImp::appendString(SQLString& to, const SQLString& from)
  {
    if (isNull(to)) {
      //if (!isNull(from)) {
      to.theString= createString(from.theString->realStr.c_str(), from.theString->realStr.length());
      //}
    }
    else {
      to.theString->realStr.append(from.theString->realStr.c_str(), from.theString->realStr.length());
    }
    return to;
  }


  SQLString& StringImp::appendString(SQLString& to, const char* from)
  {
    if (isNull(to)) {
      to.theString= createString(from, std::strlen(from));
    }
    else {
      to.theString->realStr.append(from);
    }
    return to;
  }

  // Assumes from can't be null
  SQLString& StringImp::appendString(SQLString& to, const char* from, std::size_t len)
  {
    if (isNull(to)) {
      to.theString= createString(from, len);
    }
    else {
      to.theString->realStr.append(from, len);
    }
    return to;
  }


  SQLString& StringImp::appendString(SQLString& to, char c)
  {
    if (isNull(to)) {
      to.theString= createString(&c, 1);
    }
    else {
      to.theString->realStr.append(1, c);
    }
    return to;
  }


  void StringImp::reserveSize(SQLString& str, std::size_t requiredCapacity)
  {
    if (StringImp::isNull(str)) {
      str.theString= StringImp::createString("", 0);
    }
    str.theString->realStr.reserve(requiredCapacity);
  }


  std::string& StringImp::get(SQLString& str) {
    return str.theString->realStr;
  }


  const std::string& StringImp::get(const SQLString& str) {
    return str.theString->realStr;
  }


  StringImp::StringImp(const char* str, std::size_t count) : realStr(str, count) {
  }
}

