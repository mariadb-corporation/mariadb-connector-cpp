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


#include "MariaDbDriver.h"

#include "UrlParser.h"
#include "MariaDbConnection.h"
#include "options/DefaultOptions.h"
#include "Exception.h"
#include "Consts.h"
#include "util/ClassField.h"
#include "MariaDbDatabaseMetaData.h"

namespace sql
{
namespace mariadb
{
  MARIADB_EXPORTED Driver* get_driver_instance()
  {
    return new MariaDbDriver();
  }


  Connection* MariaDbDriver::connect(const SQLString& url, Properties& props)
  {
    UrlParser* urlParser= UrlParser::parse(url, props);

    if (urlParser == nullptr || urlParser->getHostAddresses().empty())
    {
      return nullptr;
    }
    else
    {
      return MariaDbConnection::newConnection(*urlParser, nullptr);
    }
  }


  Connection * MariaDbDriver::connect(const SQLString& host, const SQLString& user, const SQLString& pwd)
  {
    Properties props{ {"user", user}, {"password", pwd} };
    return connect(host, props);
  }

  Connection * MariaDbDriver::connect(Properties & props)
  {
    SQLString uri;
    auto cit= props.find("hostName");

    if (cit != props.end())
    {
      if (!cit->second.startsWith(mysqlTcp)) {
        uri= mysqlTcp;
      }
      uri.append(cit->second);
      props.erase(cit);
    }
    else if ((cit= props.find("pipe")) != props.end())
    {
      if (!cit->second.startsWith(mysqlPipe)) {
        uri= mysqlPipe;
      }
      uri.append(cit->second);
    }
    else if ((cit= props.find("socket")) != props.end())
    {
      if (!cit->second.startsWith(mysqlSocket)) {
        uri= mysqlSocket;
      }
      uri.append(cit->second);
      props.erase(cit);
    }

    cit= props.find("schema");

    if (cit != props.end())
    {
      uri.append('/');
      uri.append(cit->second);
    }
    return connect(uri, props);
  }

  bool MariaDbDriver::acceptsURL(const SQLString& url) {
    return UrlParser::acceptsUrl(url);
  }


  std::unique_ptr<std::vector<DriverPropertyInfo>> MariaDbDriver::getPropertyInfo(SQLString& url, Properties& info)
  {
    std::unique_ptr<std::vector<DriverPropertyInfo>> result;
    Shared::Options options;

    if (!url.empty())
    {
      UrlParser *urlParser= UrlParser::parse(url, info);
      if (urlParser == NULL)// urlParser->getOptions())
      {
        return result;
      }
      options= urlParser->getOptions();
    }
    else
    {
      options= DefaultOptions::parse(HaMode::NONE, emptyStr, info, options);
    }
    for (auto o : OptionsMap)
    {
      try
      {
        ClassField<Options> field= Options::getField(o.second.getOptionName());
        Value value= field.get(*options);
        SQLString strValue= value;
        /* TODO: should ClassField know it's name?*/
        DriverPropertyInfo propertyInfo(/*field.getName()*/o.first, strValue);
        propertyInfo.description= o.second.getDescription();
        propertyInfo.required= o.second.isRequired();
        result->push_back(propertyInfo);
      }
      catch (std::exception &) {
      }
    }
    return result;
  }


  int32_t MariaDbDriver::getMajorVersion() {
    return Version::majorVersion;
  }


  int32_t MariaDbDriver::getMinorVersion() {
    return Version::minorVersion;
  }


  bool MariaDbDriver::jdbcCompliant() {
    return true;
  }


  const SQLString& MariaDbDriver::getName()
  {
    return MariaDbDatabaseMetaData::DRIVER_NAME;
  }


  Logger* MariaDbDriver::getParentLogger() {
    throw SQLFeatureNotSupportedException("Use logging parameters for enabling logging.");
  }
}
}
