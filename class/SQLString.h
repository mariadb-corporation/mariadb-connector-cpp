/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

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
#include <cctype>
#include <algorithm>

namespace mariadb
{
  typedef std::string SQLString;
  // Putting these typedefs here so far
  typedef SQLString Date;
  typedef SQLString Time;
  typedef SQLString Timestamp;
  typedef SQLString BigDecimal;

  inline SQLString& ltrim(SQLString& str)
  {
    auto first= std::find_if(str.begin(), str.end(), [](unsigned char c){ return !std::isspace(c); });
    if (first > str.begin()) {
      str.erase(str.begin(), first);
    }
    return str;
  }

  inline SQLString& rtrim(SQLString& str)
  {
    auto pos= str.find_last_not_of(" ");
    if (pos + 1 < str.length()) {
      str.erase(str.begin() + (pos != std::string::npos ? pos + 1 : 0), str.end());
    }
    return str;
  }

  inline SQLString& sqlRtrim(SQLString& str)
  {
    auto pos= str.find_last_not_of("; ");
    if (pos + 1 < str.length()) {
      str.erase(str.begin() + (pos != std::string::npos ? pos + 1 : 0), str.end());
    }
    return str;
  }


  inline SQLString& trim(SQLString& str)
  {
    return ltrim(rtrim(str));
  }


  inline SQLString& toLowerCase(SQLString& theString)
  {
    std::transform(theString.begin(), theString.end(), theString.begin(),
      [](unsigned char c) { return std::tolower(c); });
    return theString;
  }

} // namespace mariadb
#endif
