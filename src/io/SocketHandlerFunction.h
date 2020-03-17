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


#ifndef _SOCKETHANDLERFUNCTION_H_
#define _SOCKETHANDLERFUNCTION_H_

namespace sql
{
namespace  mariadb
{

class Socket;

class SocketHandlerFunction
{
  SocketHandlerFunction(const SocketHandlerFunction &);
  void operator=(SocketHandlerFunction &);

public:
  SocketHandlerFunction() {}
  virtual ~SocketHandlerFunction(){}

  virtual Socket* apply(Shared::Options &options, const SQLString& host)=0;
};

}
}

#endif
