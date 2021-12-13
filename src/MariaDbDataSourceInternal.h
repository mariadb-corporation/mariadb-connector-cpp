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

#ifndef _MARIADBDATASOURCEINTERNAL_H_
#define _MARIADBDATASOURCEINTERNAL_H_

#include <mutex>
#include <memory>

#include "StringImp.h"
#include "MariaDbDataSource.hpp"
//#include "UrlParser.h"

namespace sql
{
namespace mariadb
{

class UrlParser;

class MariaDbDataSourceInternal {
  std::mutex syncronization;
public:
  SQLString hostname;
  int32_t   port= 3306;
  int32_t   connectTimeoutInMs= 0;
  SQLString database;
  SQLString url;
  SQLString user;
  SQLString password;
  SQLString properties;

  std::shared_ptr<UrlParser> urlParser;

  MariaDbDataSourceInternal(const SQLString& _hostname, int32_t _port, const SQLString& _database)
    : hostname(_hostname)
    , port(_port)
    , database(_database)
    , urlParser(nullptr)
  {}

  MariaDbDataSourceInternal(const SQLString& url)
    : url(url)
    , urlParser(nullptr)
  {}

  void reInitializeIfNeeded();
  UrlParser* getUrlParser(); 
  void initialize(); /*synchronized*/ 
};

}
}
#endif
