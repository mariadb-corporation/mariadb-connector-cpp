/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
 *               2020, 2021 MariaDB Corporation AB
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0, as
 * published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an
 * additional permission to link the program and your derivative works
 * with the separately licensed software that they have included with
 * MySQL.
 *
 * Without limiting anything contained in the foregoing, this file,
 * which is part of MySQL Connector/C++, is also subject to the
 * Universal FOSS Exception, version 1.0, a copy of which can be found at
 * http://oss.oracle.com/licenses/universal-foss-exception.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */


/*
  Some common definitions and inclusions for C/C++ tests.
*/

#ifndef __C_CPP_TYPES_H_
#define __C_CPP_TYPES_H_

#include <cstdlib>
#if defined(_WIN32) || defined(_WIN64)
/* MySQL 5.1 might have defined it before in include/config-win.h */
#  ifdef strncasecmp
#    undef strncasecmp
#  endif
#  define strncasecmp(s1,s2,n) _strnicmp(s1,s2,n)
#else
#  include <string.h>
#endif

//#include "config.h"
#define strtoll(__a, __b, __c) std::strtoll((__a), (__b), (__c))
#define HAVE_FUNCTION_STRTOLL 1
#define strtoull(__a, __b, __c) std::strtoull((__a), (__b), (__c))

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <list>
#include <cctype>
/*#include <locale>*/

#ifndef _ABSTRACT
#define _ABSTRACT
#endif

#ifndef _PURE
#define _PURE =0
#endif

#ifndef L64
#ifdef _WIN32
#define L64(x) x##i64
#else
#define L64(x) x##LL
#endif
#endif

#ifndef UL64
#ifdef _WIN32
#define UL64(x) x##ui64
#else
#define UL64(x) x##ULL
#endif
#endif


/*----------------------------------------------------------------------------
ci_char_traits : Case-insensitive char traits.
----------------------------------------------------------------------------*/
template <typename charT> struct ci_char_traits
: public std::char_traits<charT>
// just inherit all the other functions
//  that we don't need to override
{
  static bool eq (charT c1, charT c2)
  {
    return std::tolower(c1) == std::tolower(c2);
  }

  static bool ne (charT c1, charT c2)
  {
    return std::tolower(c1) != std::tolower(c2);
  }

  static bool lt (charT c1, charT c2)
  {
    return std::tolower(c1) < std::tolower(c2);
  }

  static int compare (const charT* s1, const charT* s2, size_t n)
  {
    return strncasecmp(s1, s2, n);
  }

  static const charT* find (const charT* s, std::allocator<char>::size_type n, charT a)
  {
    while ( --n != static_cast<std::allocator<char>::size_type>(-1)
          && std::tolower(*s) != std::tolower(a))
    {
      ++s;
    }

    return (n != static_cast<std::allocator<char>::size_type>(-1) ? s : 0);
  }
};

//Some stubs for unicode use. Following usual win scheme
#ifdef UNICODE_32BIT
typedef std::basic_string
<wchar_t,
std::char_traits<wchar_t>,
std::allocator<wchar_t> >
String;

#ifndef _T
#define _T(strConst) L ## strConst
#endif

#else
typedef std::basic_string
<char,
std::char_traits<char>,
std::allocator<char> >
String;

typedef std::basic_string
<char,
ci_char_traits<char>,
std::allocator<char> >
ciString;

#ifndef _T
#define _T(strConst) strConst
#endif

#endif

typedef std::vector<String>       TestList;
typedef TestList::iterator        Iterator;
typedef TestList::const_iterator  ConstIterator;

typedef std::map<String, String>        TestProperties;
typedef TestProperties::iterator        PropsIterator;
typedef TestProperties::const_iterator  PropsConstIterator;

#endif /* __C_CPP_TYPES_H_ */
