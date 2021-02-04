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

#ifndef _MARIADBDRIVER_H_
#define _MARIADBDRIVER_H_

#include <vector>
#include <memory>

#include "Driver.hpp"
#include "options/DriverPropertyInfo.h"
#include "logger/Logger.h"

namespace sql
{
namespace mariadb
{
  class MariaDbDriver final : public sql::Driver {
    public:
      Connection* connect(const SQLString& url, Properties& props);
      Connection* connect(const SQLString& host, const SQLString& user, const SQLString& pwd);
      Connection* connect(const Properties& props);

      bool acceptsURL(const SQLString& url);
      std::unique_ptr<std::vector<DriverPropertyInfo>> getPropertyInfo(SQLString& url, Properties& info);
      uint32_t getMajorVersion();
      uint32_t getMinorVersion();
      bool jdbcCompliant();
      const SQLString& getName();
      Logger* getParentLogger();
  };
}
}
#endif