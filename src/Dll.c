/************************************************************************************
   Copyright (C) 2013,2019 MariaDB Corporation AB
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
#include <Windows.h>

#include "mysql.h"

BOOL __stdcall DllMain ( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      mysql_library_init(0, NULL, NULL);
    break;
    case DLL_PROCESS_DETACH:
      mysql_library_end();
      break;
    case DLL_THREAD_ATTACH:
      mysql_thread_init();
      break;
    case DLL_THREAD_DETACH:
      mysql_thread_end();
      break;
  }
  return TRUE;
}
