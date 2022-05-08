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


#ifndef _URLPARSER_H_
#define _URLPARSER_H_

#include <regex>
#include <memory>
#include <vector>

#include "options/Options.h"
#include "StringImp.h"
#include "HostAddress.h"
#include "credential/CredentialPlugin.h"

namespace sql
{
namespace mariadb
{
extern const SQLString mysqlTcp, mysqlSocket, mysqlPipe;

class UrlParser
{
  static std::regex URL_PARAMETER;
  static std::regex AWS_PATTERN;

  SQLString database;
  Shared::Options options;
  std::vector<HostAddress> addresses;
  enum HaMode haMode;
  SQLString initialUrl;
  bool multiMaster;
  std::shared_ptr<CredentialPlugin> credentialPlugin;
  UrlParser();

public:
  UrlParser(SQLString& database, std::vector<HostAddress>& addresses, Shared::Options options, enum HaMode haMode);
  static bool acceptsUrl(const SQLString& url);
  static UrlParser* parse(const SQLString& url);
  static UrlParser* parse(const SQLString& url, Properties& prop);

private:
  static void parseInternal(UrlParser& urlParser, const SQLString& url, Properties& properties);
  static void defineUrlParserParameters(UrlParser &urlParser, Properties& properties,
  const SQLString& hostAddressesString, const SQLString& additionalParameters);
  static enum HaMode parseHaMode(const SQLString& url, size_t separator);
  static void setDefaultHostAddressType(UrlParser& urlParser);
  void setInitialUrl();

public:
  UrlParser& auroraPipelineQuirks();
  bool isAurora();
  void parseUrl(const SQLString& url);
  const SQLString& getUsername() const;
  void setUsername(const SQLString& username);
  SQLString& getPassword();
  void setPassword(const SQLString& password);
  const SQLString& getDatabase() const;
  void setDatabase(const SQLString& database);
  std::vector<HostAddress>& getHostAddresses();
  const Shared::Options& getOptions() const;
protected:
  void setProperties(const SQLString& urlParameters);
  public:  std::shared_ptr<CredentialPlugin> getCredentialPlugin();
  public:  const SQLString& toString() const;
  public:  const SQLString& getInitialUrl() const;
  public:  enum HaMode getHaMode() const;
  public:  bool equals(UrlParser* parser);
  public:  int64_t hashCode() const;
  private: void loadMultiMasterValue();
  public:  bool isMultiMaster();
  public:  UrlParser* clone();
};
}
}
#endif