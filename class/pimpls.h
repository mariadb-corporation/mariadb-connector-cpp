/************************************************************************************
   Copyright (C) 2023 MariaDB Corporation plc

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

#ifndef __MADB_CPP_PIMPLS_H__
#define __MADB_CPP_PIMPLS_H__

#include "mysql.h"

namespace mariadb
{
  class Protocol;
  class Results;
  class CmdInformation;
  class ClientPrepareResult;
  class ClientSidePreparedStatement;
  class ServerSidePreparedStatement;
  class ResultSet;
  class ResultSetMetaData;
  class ServerPrepareResult;
  class ColumnDefinition;
  class ResultSet;
  class PreparedStatement;
  class ParamCodec;
  class ResultCodec;

  namespace Shared
  {
    typedef std::shared_ptr<mariadb::Results> Results;
  }

  namespace Unique
  {
    typedef std::unique_ptr<mariadb::Results> Results;
    typedef std::unique_ptr<mariadb::ResultSet> ResultSet;
    typedef std::unique_ptr<mariadb::ResultSetMetaData> ResultSetMetaData;
    typedef std::unique_ptr<::MYSQL, decltype(&mysql_close)> MYSQL;
    typedef std::unique_ptr<::MYSQL_RES, decltype(&mysql_free_result)> MYSQL_RES;
  }

  namespace Weak
  {
    typedef std::weak_ptr<Results> Results;
  }
} //---- namespace mariadb


#endif
