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

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>

#include "string.h"

#include "StringImp.h"

namespace sql
{

  SQLString::SQLString(const SQLString& other) : theString(new StringImp((*other.theString)->c_str(), (*other.theString)->length()))
  {
  }

  SQLString::SQLString(SQLString&& moved) : theString(std::move(moved.theString))
  {
  }

  /*SQLString::SQLString(const std::string& other) : theString(new StringImp(other.c_str(), other.length()))
  {

  }*/

  //TODO: not sure if it's not better to throw on null pointer
  SQLString::SQLString(const char* str) : theString(new StringImp(str != nullptr ? str : ""))
  {
  }


  SQLString::SQLString(const char* str, std::size_t count) : theString(new StringImp(str, count))
  {
  }


  SQLString& SQLString::operator=(const SQLString &other)
  {
    *theString= *other.theString;
    return *this;
  }


  SQLString::SQLString() : SQLString("")
  {
  }


  SQLString::~SQLString()
  {
  }

  const char* SQLString::c_str() const
  {
    return (*theString)->c_str();
  }

  bool SQLString::empty() const
  {
    return (*theString)->empty();
  }

  SQLString& SQLString::toUpperCase()
  {
    std::transform((*theString)->begin(), (*theString)->end(), (*theString)->begin(),
      [](unsigned char c) { return std::toupper(c); });
    return *this;
  }

  SQLString& SQLString::toLowerCase()
  {
    std::transform((*theString)->begin(), (*theString)->end(), (*theString)->begin(),
      [](unsigned char c) { return std::tolower(c); });
    return *this;
  }

  SQLString & SQLString::ltrim()
  {
    (*theString)->erase((*theString)->begin(), std::find_if((*theString)->begin(), (*theString)->end(), [](int ch) {
      return !std::isspace(ch);
    }));
    return *this;
  }

  SQLString & SQLString::rtrim()
  {
    (*theString)->erase(std::find_if((*theString)->rbegin(), (*theString)->rend(), [](int ch) {
      return !std::isspace(ch);
    }).base(), (*theString)->end());
    return *this;
  }

  SQLString & SQLString::trim()
  {
    return ltrim().rtrim();
  }


  int SQLString::compare(const SQLString & str) const
  {
    return (*theString)->compare(0, (*theString)->length(), (*str.theString)->c_str(), (*str.theString)->length());
  }

  int SQLString::compare(std::size_t pos1, std::size_t count1, const char* str, std::size_t count2) const
  {
    return (*theString)->compare(pos1, count1, str, count2);
  }


  SQLString & SQLString::append(const SQLString & addition)
  {
    (*theString)->append((*addition.theString)->c_str(), (*addition.theString)->length());
    return *this;
  }

  SQLString& SQLString::append(const char* const addition)
  {
    (*theString)->append(addition);
    return *this;
  }

  SQLString & SQLString::append(const char * const addition, std::size_t len)
  {
    (*theString)->append(addition, len);
    return *this;
  }

  SQLString & SQLString::append(char c)
  {
    (*theString)->append(1, c);
    return *this;
  }


  int64_t SQLString::hashCode() const
  {
    return static_cast<int64_t>(std::hash<std::string>{}(theString->get()));
  }

  bool SQLString::startsWith(const SQLString & str) const
  {
    return ((*theString)->compare(0, str.size(), (*str.theString)->c_str(), (*str.theString)->length()) == 0);
  }

  bool SQLString::endsWith(const SQLString & str) const
  {
    std::size_t size= this->size(), otherSize= str.size();

    if (otherSize > size)
    {
      return false;
    }
    return (*theString)->compare(size - otherSize, otherSize, (*str.theString)->c_str(), (*str.theString)->length()) == 0;
  }

  SQLString SQLString::substr(std::size_t pos, std::size_t count) const
  {
    return (*theString)->substr(pos, count).c_str();
  }

  std::size_t SQLString::find_first_of(const SQLString & str, std::size_t pos) const
  {
    return (*theString)->find_first_of((*str.theString)->c_str(), pos, (*str.theString)->length());
  }

  std::size_t SQLString::find_first_of(const char * str, std::size_t pos) const
  {
    return (*theString)->find_first_of(str, pos);
  }

  std::size_t SQLString::find_first_of(const char ch, std::size_t pos) const
  {
    return (*theString)->find_first_of(ch, pos);
  }


  std::size_t SQLString::find_last_of(const SQLString& str, std::size_t pos) const
  {
    return (*theString)->find_last_of(str.theString->get(), pos);
  }

  std::size_t SQLString::find_last_of(const char* str, std::size_t pos) const
  {
    return (*theString)->find_last_of(str, pos);
  }

  std::size_t SQLString::find_last_of(const char ch, std::size_t pos) const
  {
    return (*theString)->find_last_of(ch, pos);
  }


  std::size_t SQLString::size() const
  {
    return (*theString)->size();
  }

  std::size_t SQLString::length() const
  {
    return (*theString)->length();
  }

  void SQLString::reserve(std::size_t n)
  {
    (*theString)->reserve(n);
  }

  char & SQLString::at(std::size_t pos)
  {
    return (*theString)->at(pos);
  }

  const char & SQLString::at(std::size_t pos) const
  {
    return (*theString)->at(pos);
  }


  std::string::iterator SQLString::begin()
  {
    return (*theString)->begin();
  }


  std::string::iterator SQLString::end()
  {
    return (*theString)->end();
  }


  std::string::const_iterator SQLString::begin() const
  {
    return (*theString)->begin();
  }


  std::string::const_iterator SQLString::end() const
  {
    return (*theString)->end();
  }

  void SQLString::clear()
  {
    (*theString)->clear();
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

  bool operator==(const SQLString& str1, const char* str2)
  {
    return str2 != nullptr && str1.compare(0, str1.length(), str2, strlen(str2)) == 0;
  }

  bool operator==(const char* str1, const SQLString& str2)
  {
    return str1 != nullptr && str2.compare(0, str2.length(), str1, strlen(str1)) == 0;
  }


  bool operator!=(const SQLString & str1, const SQLString & str2)
  {
    return str1.compare(str2) != 0;
  }

  bool operator!=(const SQLString& str1, const char* str2)
  {
    return str2 == nullptr || str1.compare(0, str1.length(), str2, strlen(str2)) != 0;
  }

  bool operator!=(const char* str1, const SQLString& str2)
  {
    return str1 == nullptr || str2.compare(0, str2.length(), str1, strlen(str1)) != 0;
  }


  std::ostream& operator<<(std::ostream& stream, const SQLString & str)
  {
    return stream << StringImp::get(str);//static_cast<const std::string&>(str);
  }


  SQLString::operator const char* () const
  {
    return (*theString)->c_str();
  }


  SQLString & SQLString::operator=(const char * right)
  {
    *theString= (right != nullptr ? right : "");
    return *this;
  }


  int SQLString::caseCompare(const SQLString& other) const
  {
    SQLString lcThis((*theString)->c_str(), (*theString)->length()), lsThat(other.c_str(), other.length());
    return lcThis.toLowerCase().compare(lsThat.toLowerCase());
  }
}
