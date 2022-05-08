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
#include "Exception.hpp"
#include "Consts.h"
#include "util/ClassField.h"
#include "MariaDbDatabaseMetaData.h"

namespace sql
{
namespace mariadb
{
  static MariaDbDriver theInstance;
  extern const SQLString mysqlTcp, mysqlSocket, mysqlPipe;


  Driver* get_driver_instance()
  {
    return &theInstance;
  }


  Connection* MariaDbDriver::connect(const SQLString& url, Properties& props)
  {
    Properties propsCopy(props);
    UrlParser* urlParser= UrlParser::parse(url, propsCopy);

    if (urlParser == nullptr || urlParser->getHostAddresses().empty())
    {
      return nullptr;
    }
    else
    {
      return MariaDbConnection::newConnection(*urlParser, nullptr);
    }
  }


  void normalizeLegacyUri(SQLString& url, Properties* prop= nullptr) {

    //Making TCP default with legacy uri
    if (StringImp::get(url).find("://") == std::string::npos) {
      url= "tcp://" + url;
    }

    // Looks like in the only use of the normalizeLegacyUri propeties are not passed, i.e. it's nullptr
    // TODO: Something is wrong
    if (prop != nullptr)
    {
      std::string key;
      std::size_t offset= 0;
      
      if (url.startsWith(mysqlTcp))
      {
        auto cit= prop->find("port");
        if (cit != prop->end()) {
          SQLString host(url.substr(mysqlTcp.length()));
          std::size_t colon= host.find_first_of(':');
          std::size_t schemaSlash= schemaSlash= host.find_first_of('/');
          SQLString schema(schemaSlash != std::string::npos ? url.substr(schemaSlash + 1) : emptyStr);

          if (colon != std::string::npos) {
            host= host.substr(0, colon);
          }
          url= mysqlTcp + host + ":" + cit->second + "/" + schema;
        }
      }
      else if (url.startsWith(mysqlPipe)) {
        offset = mysqlPipe.length();
        key = "pipe";
      }
      else if (url.startsWith(mysqlSocket)) {
        key = "localSocket";
        offset = mysqlSocket.length();
      }
      else {
        return;
      }

      std::string name(StringImp::get(url.substr(offset)));
      std::size_t slashPos = name.find_first_of('/');

      if (slashPos != std::string::npos) {
        name = name.substr(0, slashPos);
      }

      (*prop)[key] = name;
    }
  }


  Connection * MariaDbDriver::connect(const SQLString& host, const SQLString& user, const SQLString& pwd)
  {
    Properties props{ {"user", user}, {"password", pwd} };
    SQLString localCopy(host);

    normalizeLegacyUri(localCopy);

    return connect(localCopy, props);
  }


  Connection * MariaDbDriver::connect(const Properties &initProps)
  {
    SQLString uri;
    Properties props(initProps);
    auto cit= props.find("hostName");

    if (cit != props.end())
    {
      if (!UrlParser::acceptsUrl(cit->second)) {
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
      if (urlParser == nullptr)// urlParser->getOptions())
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


  uint32_t MariaDbDriver::getMajorVersion() {
    return Version::majorVersion;
  }


  uint32_t MariaDbDriver::getMinorVersion() {
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
