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

#include "Consts.h"
#include "DriverManager.hpp"
#include "Exception.hpp"

namespace sql
{
  static Driver* getDriver(const SQLString& url)
  {
    if (StringImp::get(url).find("jdbc:") != 0)
    {
      throw SQLException("Incorrect URL format - there is no jdbc: prefix");
    }

    std::size_t driverNameEnd = StringImp::get(url).find("://", 5); //5 - length of jdbc: prefix

    if (driverNameEnd == std::string::npos)
    {
      throw SQLException("Incorrect URL format - there is no :// separator");
    }

    SQLString driverName(url.substr(5, driverNameEnd - 5));

    // Here should be loop thru registered drivers
    if (StringImp::get(driverName).find("mariadb") == 0)
    {
      // And here should be call to the registered callback
      return sql::mariadb::get_driver_instance();
    }
    else
    {
      throw SQLException(driverName + " has not been registered");
    }
    //Shouldn't happen
    return nullptr;
  }


  Connection* DriverManager::getConnection(const SQLString& url)
  {
    static Properties dummy;
   
    return getConnection(url, dummy);
  }


  Connection* DriverManager::getConnection(const SQLString& url, Properties& props)
  {
    Driver *driver= getDriver(url);
    Connection* conn = driver->connect(url, props);

    if (conn == nullptr)
    {
      throw SQLException("Connection could not be established - URL is incorrect/could not be parsed");
    }
    return conn;
  }


  Connection* DriverManager::getConnection(const SQLString& url, const SQLString& user, const SQLString& pwd)
  {
    Driver *driver= getDriver(url);
    Connection* conn = driver->connect(url, user, pwd);

    if (conn == nullptr)
    {
      throw SQLException("Connection could not be established - URL is incorrect/could not be parsed");
    }
    return conn;
  }
}

