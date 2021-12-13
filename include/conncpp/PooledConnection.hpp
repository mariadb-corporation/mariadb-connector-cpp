/************************************************************************************
   Copyright (C) 2021 MariaDB Corporation AB

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


#ifndef _POOLEDCONNECTION_H_
#define _POOLEDCONNECTION_H_

//#include "conncpp/Connection.hpp"

namespace sql
{
class ConnectionEventListener;
class StatementEventListener;
class Connection;

class PooledConnection {

public:
  PooledConnection() {}
  virtual ~PooledConnection() {}
  PooledConnection(const PooledConnection&)= delete;
  PooledConnection& operator=(const PooledConnection&) = delete;

  virtual sql::Connection* getConnection()=0;
  virtual void close()=0;
  virtual void addConnectionEventListener(ConnectionEventListener* listener)=0;
  virtual void removeConnectionEventListener(ConnectionEventListener* listener)=0;
  virtual void addStatementEventListener(StatementEventListener* listener)=0;
  virtual void removeStatementEventListener(StatementEventListener* listener)=0;
  };
}

#endif
