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
#include <string>
#include <cstring>
#include <stdexcept>

#include "util/String.h"
#include "StringImp.h"

namespace sql
{

////////////////////// Standalone SQLString util functions /////////////////////////////

namespace mariadb
{
  /* split does not accept regexp delimiter any more */
  sql::mariadb::Tokens split(const SQLString& str, const SQLString & delimiter)
  {
    std::unique_ptr<std::vector<sql::SQLString>> result(new std::vector<sql::SQLString>());
    std::string::const_iterator it = str.cbegin();
    std::size_t offset = 0, prevOffset = 0;

    while ((offset = StringImp::get(str).find(delimiter, prevOffset)) != std::string::npos)
    {
      std::string tmp(it, it + (offset - prevOffset));
      result->emplace_back(tmp);

      prevOffset = offset + delimiter.size();

      // Moving iterator past last found delimiter
      it += tmp.size() + delimiter.size();
      if (it >= str.cend())
      {
        break;
      }
    }
    std::string tmp(it, str.cend());
    result->emplace_back(tmp);

    return result;
  }


  SQLString& replaceInternal(SQLString& str, const SQLString& _substr, const SQLString &subst)
  {
    size_t pos= 0, prev= 0;
    std::string &real= StringImp::get(str);
    const std::string &realSub= StringImp::get(subst), &substr= StringImp::get(_substr);

    while ((pos= real.find(substr, prev)) != std::string::npos)
    {
      real.replace(pos, substr.length(), realSub);
      prev+= realSub.length();
    }
    return str;
  }

  /* replace does not accept regexp */
  SQLString& replace(SQLString& str, const SQLString& substr, const SQLString& subst)
  {
    return replaceInternal(str, substr, subst);
  }


  SQLString replace(const SQLString& str, const SQLString& substr, const SQLString& subst)
  {
    SQLString result(str);

    /* This should probably work w/out Internal function, but...  feels safer :) */
    return replaceInternal(result, substr, subst);
  }

  /* This function is to repace occurrences of any character in the substr(ing) to subst(itution) string.
   * It covers the case of regexp dfining class of characters.
   */
  SQLString& replaceAny(SQLString& str, const SQLString& substr, const SQLString &subst)
  {
    size_t pos= 0, prev= 0;
    std::string &real= StringImp::get(str);
    const std::string &realSub= StringImp::get(subst);

    while ((pos= real.find_first_of(substr.c_str(), prev)) != std::string::npos)
    {
      real.replace(pos, 1, realSub);
      prev+= realSub.length();
    }
    return str;
  }


  SQLString replaceAny(const SQLString& str, const SQLString& substr, const SQLString &subst)
  {
    SQLString changeble(str);
    return replaceAny(changeble, substr, subst);
  }


  bool equalsIgnoreCase(const SQLString& str1, const SQLString& str2)
  {
    SQLString localStr1(str1), localStr2(str2);

    return (localStr1.toLowerCase().compare(localStr2.toLowerCase()) == 0);
  }


  uint64_t stoull(const SQLString& str, std::size_t* pos)
  {
    bool negative= false;
    std::string::const_iterator ci= str.begin();
    while (std::isblank(*ci) &&  ci < str.end()) ++ci;

    if (*str == '-')
    {
      negative= true;
    }

    uint64_t result= std::stoull(StringImp::get(str), pos);

    if (negative && result != 0)
    {
      throw std::out_of_range("String represents number beyond uint64_t range");
    }
    return result;
  }


  uint64_t stoull(const char* str, std::size_t len, std::size_t* pos)
  {
    len= len == static_cast<std::size_t>(-1) ? std::strlen(str) : len;
    return stoull(sql::SQLString(str, len), pos);
  }
}
}
