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

#
#include <algorithm>
#include <cctype>
#include <functional>
#include <regex>

#include "Consts.h"

#include "SQLString.h"


namespace sql
{
  SQLString::SQLString(const SQLString &other) : theString(other.theString)
  {
  }


  SQLString::SQLString(const std::string& other) : theString(other)
  {

  }

  /*SQLString::SQLString(const std::string other) : theString(other)
  {
  }*/

  //TODO: not sure if it's not better to throw on null pointer
  SQLString::SQLString(const char* str) : theString(str != nullptr ? str : "")
  {
  }


  SQLString::SQLString(const char* str, std::size_t count) : theString(str, count)
  {
  }


  SQLString& SQLString::operator=(const SQLString &other)
  {
    theString= other.theString;
    return *this;
  }


  SQLString::SQLString()
  {
  }


  SQLString::~SQLString()
  {
  }


  SQLString& SQLString::toUpperCase()
  {
    std::transform(theString.begin(), theString.end(), theString.begin(),
      [](unsigned char c) { return std::toupper(c); });
    return *this;
  }

  SQLString& SQLString::toLowerCase()
  {
    std::transform(theString.begin(), theString.end(), theString.begin(),
      [](unsigned char c) { return std::tolower(c); });
    return *this;
  }

  SQLString & SQLString::ltrim()
  {
    theString.erase(theString.begin(), std::find_if(theString.begin(), theString.end(), [](int ch) {
      return !std::isspace(ch);
    }));
    return *this;
  }

  SQLString & SQLString::rtrim()
  {
    theString.erase(std::find_if(theString.rbegin(), theString.rend(), [](int ch) {
      return !std::isspace(ch);
    }).base(), theString.end());
    return *this;
  }

  SQLString & SQLString::trim()
  {
    return ltrim().rtrim();
  }


  int SQLString::compare(const SQLString & str) const
  {
    return theString.compare(str.theString);
  }

  SQLString & SQLString::append(const SQLString & addition)
  {
    theString.append(static_cast<const std::string&>(addition));
    return *this;
  }

  SQLString & SQLString::append(const char * const addition)
  {
    theString.append(addition);
    return *this;
  }

  SQLString & SQLString::append(char c)
  {
    theString.append(1, c);
    return *this;
  }

  int64_t SQLString::hashCode() const
  {
    return static_cast<int64_t>(std::hash<std::string>{}(theString));
  }

  bool SQLString::startsWith(const SQLString & str) const
  {
    return (theString.compare(0, str.size(), str) == 0);
  }

  bool SQLString::endsWith(const SQLString & str) const
  {
    size_t size= this->size(), otherSize= str.size();

    if (otherSize > size)
    {
      return false;
    }
    return theString.compare(size - otherSize, otherSize, str) == 0;
  }

  SQLString SQLString::substr(size_t pos, size_t count) const
  {
    return theString.substr(pos, count).c_str();
  }

  size_t SQLString::find_first_of(const SQLString & str, size_t pos) const
  {
    return theString.find_first_of(str.theString, pos);
  }

  size_t SQLString::find_first_of(const char * str, size_t pos) const
  {
    return theString.find_first_of(str, pos);
  }

  size_t SQLString::find_first_of(const char ch, size_t pos) const
  {
    return theString.find_first_of(ch, pos);
  }


  size_t SQLString::find_last_of(const SQLString& str, size_t pos) const
  {
    return theString.find_last_of(str.theString, pos);
  }
  size_t SQLString::find_last_of(const char* str, size_t pos) const
  {
    return theString.find_last_of(str, pos);
  }
  size_t SQLString::find_last_of(const char ch, size_t pos) const
  {
    return theString.find_last_of(ch, pos);
  }


  size_t SQLString::size() const
  {
    return theString.size();
  }

  size_t SQLString::length() const
  {
    return theString.length();
  }

  void SQLString::reserve(size_t n)
  {
    theString.reserve(n);
  }

  char & SQLString::at(size_t pos)
  {
    return theString.at(pos);
  }

  const char & SQLString::at(size_t pos) const
  {
    return theString.at(pos);
  }


  std::string::iterator SQLString::begin()
  {
    return theString.begin();
  }


  std::string::iterator SQLString::end()
  {
    return theString.end();
  }


  std::string::const_iterator SQLString::begin() const
  {
    return theString.begin();
  }


  std::string::const_iterator SQLString::end() const
  {
    return theString.end();
  }

  void SQLString::clear()
  {
    theString.clear();
  }


  bool SQLString::operator<(const SQLString &other) const
  {
    return compare(other) < 0;
  }

  /* SQLString utilities not belonging to the class */
   SQLString operator+(const SQLString & str1, const SQLString & str2)
  {
    SQLString result(str1);
    return result.append(str2);
  }

  bool operator==(const SQLString & str1, const SQLString & str2)
  {
    return str1.compare(str2) == 0;
  }

  bool operator!=(const SQLString & str1, const SQLString & str2)
  {
    return str1.compare(str2) != 0;
  }

  std::ostream& operator<<(std::ostream& stream, const SQLString & str)
  {
    return stream << static_cast<const std::string&>(str);
  }

  /*SQLString operator+(, const char* str2)
  {
    std::string result(str1);
    return result.append(str2);
  }*/

  SQLString & SQLString::operator=(const char * right)
  {
    theString= (right != nullptr ? right : "");
    return *this;
  }


  int SQLString::caseCompare(const SQLString& other) const
  {
    SQLString lcThis(theString), lsThat(other);
    return lcThis.toLowerCase().compare(lsThat.toLowerCase());
  }
////////////////////// Standalone util functions /////////////////////////////

namespace mariadb
{
  sql::mariadb::Tokens split(const SQLString& str, const SQLString & delimiter)
  {
    std::unique_ptr<std::vector<SQLString>> result(new std::vector<SQLString>());
    const std::string& theString= static_cast<const std::string&>(str);
    std::string::const_iterator it= theString.cbegin();
    size_t offset= 0, prevOffset= 0;

    while ((offset= theString.find_first_of(delimiter, prevOffset)) != std::string::npos)
    {
      std::string tmp(it, it + (offset - prevOffset));
      result->emplace_back(tmp);
      prevOffset= ++offset;

      if (it < theString.cend() - offset)
      {
        it += offset;
      }
      else
      {
        break;
      }
    }
    
    std::string tmp(it, theString.cend());
    result->push_back(tmp);

    return result;
  }


  SQLString& replaceInternal(SQLString& str, const SQLString& substr, const SQLString &subst)
  {
    /* as a matter of fact, (regex symbols in)substr has to be escaped */
    std::regex_replace(static_cast<std::string&>(str), std::regex(static_cast<const std::string&>(substr)), static_cast<const std::string&>(subst));
    return str;
  }


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


  SQLString& replaceAll(SQLString& str, const SQLString& substr, const SQLString &subst)
  {
    return replace(str, substr, subst);
  }


  SQLString replaceAll(const SQLString& str, const SQLString& substr, const SQLString &subst)
  {
    return replace(str, substr, subst);
  }

  bool equalsIgnoreCase(const SQLString& str1, const SQLString& str2)
  {
    SQLString localStr1(str1), localStr2(str2);

    return (localStr1.toLowerCase().compare(localStr2.toLowerCase()) == 0);
  }

}
}