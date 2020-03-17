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

#ifndef _SQLSTRING_H_
#define _SQLSTRING_H_
#include <string>

#ifdef _WIN32
# ifndef MARIADB_EXPORTED
#  define MARIADB_EXPORTED __declspec(dllimport)
# endif
#else
# ifndef MARIADB_EXPORTED
#  define MARIADB_EXPORTED
# endif
#endif

namespace sql
{
class MARIADB_EXPORTED SQLString final {

  std::string theString;
public:
  SQLString(const SQLString&);
  SQLString(const std::string&);
  //SQLString(const std::string);
  SQLString(const char* str);
  SQLString(const char * str, std::size_t count);
  SQLString();
  ~SQLString();

  const char * c_str() const { return theString.c_str(); }
  SQLString& operator=(const SQLString&);
  operator std::string&() { return theString; }
  operator const std::string&() const { return theString; }
  //operator const char*() const { return theString.c_str(); }
  SQLString& operator=(const char * right);
  bool operator <(const SQLString&) const;

  inline bool empty() const { return theString.empty(); }
  int compare(const SQLString& str) const;
  SQLString & append(const SQLString& addition);
  SQLString & append(const char * const addition);
  SQLString & append(char c);
  SQLString substr(size_t pos= 0, size_t count=std::string::npos) const;
  size_t find_first_of(const SQLString& str, size_t pos = 0) const;
  size_t find_first_of(const char* str, size_t pos = 0) const;
  size_t find_first_of(const char ch, size_t pos = 0) const;
  size_t find_last_of(const SQLString& str, size_t pos=std::string::npos) const;
  size_t find_last_of(const char* str, size_t pos=std::string::npos) const;
  size_t find_last_of(const char ch, size_t pos=std::string::npos) const;
  size_t size() const;
  size_t length() const;
  void reserve(size_t n= 0);
  char& at(size_t pos);
  const char& at(size_t pos) const;
  std::string::iterator begin();
  std::string::iterator end();
  std::string::const_iterator begin() const;
  std::string::const_iterator end() const;
  void clear();

  /* Few extensions to mimic some java's String functionality. Probably should be moved to standalone functions */
  int64_t hashCode() const;
  bool startsWith(const SQLString &str) const;
  bool endsWith(const SQLString &str) const;
  SQLString& toUpperCase();
  SQLString& toLowerCase();
  SQLString& ltrim();
  SQLString& rtrim();
  SQLString& trim();
};

MARIADB_EXPORTED SQLString operator+(const SQLString& str1, const SQLString & str2);
//SQLString operator+(const SQLString & str1, const char* str2);
MARIADB_EXPORTED bool operator==(const SQLString& str1, const SQLString & str2);
MARIADB_EXPORTED bool operator!=(const SQLString& str1, const SQLString & str2);

MARIADB_EXPORTED std::ostream& operator<<(std::ostream& stream, const SQLString& str);
}
#endif
