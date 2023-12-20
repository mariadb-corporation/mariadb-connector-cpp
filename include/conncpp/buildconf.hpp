/************************************************************************************
   Copyright (C) 2020, 2023 MariaDB Corporation AB

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

#ifndef _BUILDCONF_H_
#define _BUILDCONF_H_

#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)
# ifdef MARIADB_STATIC_LINK
#  ifdef MARIADB_EXTERN
#   undef MARIADB_EXTERN
#  endif
#  define MARIADB_EXTERN extern
#  ifdef MARIADB_EXPORTED
#   undef MARIADB_EXPORTED
#  endif
#  define MARIADB_EXPORTED 
# else
#  ifndef MARIADB_EXPORTED
#   define MARIADB_EXPORTED __declspec(dllimport)
#   define MARIADB_EXTERN extern
#  else
#   ifndef MARIADB_EXTERN
#     define MARIADB_EXTERN  
#   endif
#  endif
# endif
#else
# ifndef MARIADB_EXPORTED
#  define MARIADB_EXPORTED 
# endif
# define MARIADB_EXTERN extern
#endif

#endif
