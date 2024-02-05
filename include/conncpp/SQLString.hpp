
/************************************************************************************
   Copyright (C) 2020, 2021 MariaDB Corporation AB

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

#ifndef _SQLSTRING_H_
#define _SQLSTRING_H_

#include <memory>
#include <string>

#include "buildconf.hpp"

namespace sql
{
class StringImp;

#pragma warning(push)
#pragma warning(disable:4251)

class SQLString final {

  friend class StringImp;

  StringImp* theString;
public:
  MARIADB_EXPORTED SQLString(const SQLString&);
  MARIADB_EXPORTED SQLString(SQLString&&); //Move constructor
  // Should not be exported
  SQLString(const std::string& str) : SQLString(str.c_str(), str.length()) {}
  MARIADB_EXPORTED SQLString(const char* str);
  MARIADB_EXPORTED SQLString(const char* str, std::size_t count);
  MARIADB_EXPORTED SQLString();
  MARIADB_EXPORTED ~SQLString();

  static constexpr std::size_t npos{static_cast<std::size_t>(-1)};
  MARIADB_EXPORTED const char * c_str() const;
  MARIADB_EXPORTED SQLString& operator=(const SQLString&);
  MARIADB_EXPORTED operator const char* () const;
  MARIADB_EXPORTED SQLString& operator=(const char * right);
  MARIADB_EXPORTED bool operator <(const SQLString&) const;

  MARIADB_EXPORTED bool empty() const;
  MARIADB_EXPORTED int compare(const SQLString& str) const;
  MARIADB_EXPORTED int compare(std::size_t pos1, std::size_t count1, const char* s, std::size_t count2) const;
  MARIADB_EXPORTED int caseCompare(const SQLString& other) const;
  MARIADB_EXPORTED SQLString & append(const SQLString& addition);
  MARIADB_EXPORTED SQLString & append(const char * const addition);
  MARIADB_EXPORTED SQLString & append(const char * const addition, std::size_t len);
  MARIADB_EXPORTED SQLString & append(char c);
  MARIADB_EXPORTED SQLString substr(std::size_t pos= 0, size_t count=npos) const;
  MARIADB_EXPORTED std::size_t find(const SQLString& str, std::size_t pos = 0) const;
  MARIADB_EXPORTED std::size_t find(const char* str, std::size_t pos, std::size_t n) const;
  MARIADB_EXPORTED std::size_t find_first_of(const SQLString& str, std::size_t pos = 0) const;
  MARIADB_EXPORTED std::size_t find_first_of(const char* str, std::size_t pos = 0) const;
  MARIADB_EXPORTED std::size_t find_first_of(const char ch, std::size_t pos = 0) const;
  MARIADB_EXPORTED std::size_t find_last_of(const SQLString& str, std::size_t pos=npos) const;
  MARIADB_EXPORTED std::size_t find_last_of(const char* str, std::size_t pos=npos) const;
  MARIADB_EXPORTED std::size_t find_last_of(const char ch, std::size_t pos=npos) const;
  MARIADB_EXPORTED std::size_t size() const;
  MARIADB_EXPORTED std::size_t length() const;
  MARIADB_EXPORTED void reserve(std::size_t n= 0);
  MARIADB_EXPORTED char& at(std::size_t pos);
  MARIADB_EXPORTED const char& at(std::size_t pos) const;
  MARIADB_EXPORTED std::string::iterator begin();
  MARIADB_EXPORTED std::string::iterator end();
  MARIADB_EXPORTED std::string::const_iterator begin() const;
  MARIADB_EXPORTED std::string::const_iterator end() const;
  MARIADB_EXPORTED std::string::const_iterator cbegin() const { return begin(); }
  MARIADB_EXPORTED std::string::const_iterator cend() const { return end(); }
  MARIADB_EXPORTED void clear();

  /* Few extensions to mimic some java's String functionality. Probably should be moved to standalone functions */
  MARIADB_EXPORTED int64_t hashCode() const;
  MARIADB_EXPORTED bool startsWith(const SQLString &str) const;
  MARIADB_EXPORTED bool endsWith(const SQLString &str) const;
  MARIADB_EXPORTED SQLString& toUpperCase();
  MARIADB_EXPORTED SQLString& toLowerCase();
  MARIADB_EXPORTED SQLString& ltrim();
  MARIADB_EXPORTED SQLString& rtrim();
  MARIADB_EXPORTED SQLString& trim();
};

MARIADB_EXPORTED SQLString operator+(const SQLString& str1, const SQLString & str2);
//SQLString operator+(const SQLString & str1, const char* str2);
MARIADB_EXPORTED bool operator==(const SQLString& str1, const SQLString & str2);
MARIADB_EXPORTED bool operator==(const SQLString& str1, const char* str2);
MARIADB_EXPORTED bool operator==(const char* str1, const SQLString& str2);
inline bool operator==(const SQLString& str1, const std::string& str2) { return str1.compare(0, str1.length(), str2.c_str(), str2.length()) == 0; }
inline bool operator==(const std::string& str1, const SQLString& str2) { return str2.compare(0, str2.length(), str1.c_str(), str1.length()) == 0; }
MARIADB_EXPORTED bool operator!=(const SQLString& str1, const SQLString & str2);
MARIADB_EXPORTED bool operator!=(const SQLString& str1, const char* str2);
MARIADB_EXPORTED bool operator!=(const char* str1, const SQLString& str2);
inline bool operator!=(const SQLString& str1, const std::string& str2) { return !(str1 == str2); }
inline bool operator!=(const std::string& str1, const SQLString& str2) { return !(str1 == str2); }

MARIADB_EXPORTED std::ostream& operator<<(std::ostream& stream, const SQLString& str);
}
#pragma warning(pop)
#endif
