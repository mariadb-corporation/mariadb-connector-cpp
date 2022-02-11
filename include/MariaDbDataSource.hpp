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

#ifndef _MARIADBDATASOURCE_H_
#define _MARIADBDATASOURCE_H_

//#include "buildconf.hpp"
#include "conncpp.hpp"

namespace sql
{
namespace mariadb
{
class MariaDbDataSourceInternal;

#pragma warning(push)
#pragma warning(disable:4251)

class MARIADB_EXPORTED MariaDbDataSource {
  /* Hiding class properties to accomodate there possible enhancement, expecting that the rest of properties is stable set of them*/
  std::unique_ptr<MariaDbDataSourceInternal> internal;
#pragma warning(pop)

public:
  MariaDbDataSource(const SQLString& url);
  MariaDbDataSource(const SQLString& url, const Properties& props);
  MariaDbDataSource();  
  ~MariaDbDataSource();

  const SQLString& getUser();
  void setUser(const SQLString& user);  
  void setPassword(const SQLString& password);  
  void setProperties(const Properties& properties);
  void getProperties(Properties& properties);
  void setUrl(const SQLString& url);
  SQLString getUrl();
  Connection* getConnection();  
  Connection* getConnection(const SQLString& username,const SQLString& password);  
  PrintWriter* getLogWriter();  
  void setLogWriter(const PrintWriter& out);  
  int32_t getLoginTimeout();  
  void setLoginTimeout(int32_t seconds);  
  PooledConnection& getPooledConnection();  
  PooledConnection& getPooledConnection(const SQLString& user, const SQLString& password);  
  XAConnection* getXAConnection();  
  XAConnection* getXAConnection(const SQLString& user, const SQLString& password);  
  sql::Logger* getParentLogger();
  /** Close datasource - this closes corresponding connections pool */
  void close();
};

}
}
#endif
