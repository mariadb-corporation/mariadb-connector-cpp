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


#include "UrlParser.h"
#include "Consts.h"
#include "options/DefaultOptions.h"
#include "pool/GlobalStateInfo.h"
#include "Exception.hpp"
#include "credential/CredentialPluginLoader.h"
#include "logger/LoggerFactory.h"

namespace sql
{
namespace mariadb
{
  std::regex UrlParser::URL_PARAMETER("(\\/([^\\?]*))?(\\?(.+))*", std::regex_constants::ECMAScript);
  std::regex UrlParser::AWS_PATTERN("(.+)\\.([a-z0-9\\-]+\\.rds\\.amazonaws\\.com)", std::regex_constants::ECMAScript | std::regex_constants::icase);

  const SQLString mysqlTcp("tcp://"), mysqlSocket("unix://"), mysqlPipe("pipe://");


  bool isLegacyUriFormat(const SQLString& url)
  {
    if (url.empty() || url.startsWith(mysqlTcp))
    {
      return true;
    }
    else if (url.startsWith(mysqlPipe)) {
      return true;
    }
    else if (url.startsWith(mysqlSocket)) {
      return true;
    }

    return false;
  }


  UrlParser::UrlParser() : options(new Options())
  {}

  UrlParser::UrlParser(SQLString& database, std::vector<HostAddress>& addresses, Shared::Options options, enum HaMode haMode) :
    options(options),
    database(database),
    addresses(addresses),
    haMode(haMode)
  {
    if (haMode == HaMode::AURORA)
    {
      for (HostAddress hostAddress : addresses) {
        hostAddress.type= "";
      }
    }
    else
    {
      for (HostAddress hostAddress : addresses) {
        if (hostAddress.type.empty()) {
          hostAddress.type= ParameterConstant::TYPE_MASTER;
        }
      }
    }
    this->credentialPlugin= CredentialPluginLoader::get(StringImp::get(options->credentialType));
    DefaultOptions::postOptionProcess(options, credentialPlugin.get());
    setInitialUrl();
    loadMultiMasterValue();
  }


  bool UrlParser::acceptsUrl(const SQLString& url) {
    return (url.startsWith("jdbc:mariadb:") || isLegacyUriFormat(url));
  }


  UrlParser* UrlParser::parse(const SQLString& url) {
    Properties emptyProps;
    return parse(url, emptyProps);
  }   


  UrlParser* UrlParser::parse(const SQLString& url, Properties& prop)
  {
    if (url.startsWith("jdbc:mariadb:")
      || isLegacyUriFormat(url))
    {
      UrlParser *urlParser= new UrlParser();

      parseInternal(*urlParser, url, prop);
      return urlParser;
    }

    return nullptr;
  }


  void UrlParser::parseInternal(UrlParser& urlParser, const SQLString& url, Properties &properties)
  {
    try
    {
      urlParser.initialUrl= url;
      size_t separator= StringImp::get(url).find("//");

      if (separator == std::string::npos)
      {
        throw IllegalArgumentException("url parsing error : '//' is not present in the url " + url);
      }
      urlParser.haMode= parseHaMode(url, separator);

      if (urlParser.haMode != HaMode::NONE)
      {
        throw SQLFeatureNotImplementedException(SQLString("Support of the HA mode") + HaModeStrMap[urlParser.haMode] + "is not yet implemented");
      }

      SQLString urlSecondPart= url.substr(separator + 2);
      size_t dbIndex= urlSecondPart.find_first_of('/');
      size_t paramIndex= urlSecondPart.find_first_of('?');
      SQLString hostAddressesString;
      SQLString additionalParameters;

      if (paramIndex != std::string::npos && (dbIndex == std::string::npos || dbIndex > paramIndex))
      {
        hostAddressesString= urlSecondPart.substr(0, paramIndex);
        additionalParameters= urlSecondPart.substr(paramIndex);
      }
      else if (dbIndex != std::string::npos)
      {
        hostAddressesString= urlSecondPart.substr(0, dbIndex);
        additionalParameters= urlSecondPart.substr(dbIndex);
      }
      else
      {
        hostAddressesString= urlSecondPart;
      }

      defineUrlParserParameters(urlParser, properties, hostAddressesString, additionalParameters);
      setDefaultHostAddressType(urlParser);
      urlParser.loadMultiMasterValue();
    }
    catch (std::exception &i)
    {
      delete &urlParser;
      throw SQLException(std::string("Error parsing url: ") + i.what());
    }
  }


  void UrlParser::defineUrlParserParameters(UrlParser &urlParser, Properties& properties,
    const SQLString& hostAddressesString, const SQLString& additionalParameters)
  {
    if (!additionalParameters.empty())
    {
      std::string temp(additionalParameters.c_str(), additionalParameters.length());
      std::smatch matcher;

      if (std::regex_search(temp, matcher, URL_PARAMETER))
      {
        urlParser.database= matcher[2].str();
        urlParser.options= DefaultOptions::parse(urlParser.haMode, matcher[4].str(), properties, urlParser.options);
      }
      else {
        urlParser.database= "";
        urlParser.options= DefaultOptions::parse(urlParser.haMode, emptyStr, properties, urlParser.options);
      }
    }
    else {
      urlParser.database= "";
      urlParser.options= DefaultOptions::parse(urlParser.haMode, emptyStr, properties, urlParser.options);
    }
    urlParser.credentialPlugin= CredentialPluginLoader::get(StringImp::get(urlParser.options->credentialType));
    DefaultOptions::postOptionProcess(urlParser.options, urlParser.credentialPlugin.get());

    LoggerFactory::init(
      urlParser.options->log
      || urlParser.options->profileSql
      || urlParser.options->slowQueryThresholdNanos  > 0);

    urlParser.addresses= HostAddress::parse(hostAddressesString, urlParser.haMode);
  }


  enum HaMode UrlParser::parseHaMode(const SQLString& url, size_t separator)
  {
    size_t firstColonPos= url.find_first_of(':');
    size_t secondColonPos= url.find_first_of(':', firstColonPos +1);
    size_t thirdColonPos= url.find_first_of(':', secondColonPos +1);

    if (thirdColonPos >separator ||thirdColonPos ==-1)
    {
      if (secondColonPos ==separator -1) {
        return HaMode::NONE;
      }
      thirdColonPos= separator;
    }
    try {
      std::string haModeString(StringImp::get(url.substr(secondColonPos + 1, thirdColonPos - secondColonPos - 1).toUpperCase()));
      if (haModeString.compare("FAILOVER") == 0) {
        haModeString= "LOADBALANCE";
      }
      // TODO Need to process if wrong HaMode value
      return StrHaModeMap[haModeString];
    }
    catch (std::runtime_error&) {
      throw IllegalArgumentException("wrong failover parameter format in connection SQLString "+url);
    }
  }


  void UrlParser::setDefaultHostAddressType(UrlParser &urlParser) {
    if (urlParser.haMode ==AURORA) {
      for (HostAddress hostAddress : urlParser.addresses) {
        hostAddress.type= "";
      }
    }
    else {
      for (HostAddress hostAddress : urlParser.addresses) {
        if (hostAddress.type.empty()) {
          hostAddress.type= ParameterConstant::TYPE_MASTER;
        }
      }
    }
  }


  void UrlParser::setInitialUrl()
  {
    SQLString sb("jdbc:mariadb:");

    if (haMode !=HaMode::NONE)
    {
      std::string asStr= HaModeStrMap[haMode];
      sb.append(asStr).toLowerCase().append(":");
    }
    sb.append("//");
    bool notFirst= false;
    for (auto hostAddress : addresses) {
      if (notFirst) {
        sb.append(",");
      }
      else
      {
        notFirst= true;
      }
      sb.append("address=(host=")
        .append(hostAddress.host)
        .append(")")
        .append("(port=")
        .append(std::to_string(hostAddress.port))
        .append(")");
      if (!hostAddress.type.empty()) {
        sb.append("(type=").append(hostAddress.type).append(")");
      }
    }
    sb.append("/");
    if (!database.empty()) {
      sb.append(database);
    }
    DefaultOptions::propertyString(options, haMode, sb);
    initialUrl= sb;
  }


  UrlParser& UrlParser::auroraPipelineQuirks()
  {
    bool disablePipeline= isAurora();

    if (disablePipeline)
    {
      /* We do need 3rd state for useBatchMultiSend - "not set" */
      if (!options->useBatchMultiSend)
      {
        options->useBatchMultiSend= !disablePipeline;
      }
      if (!options->usePipelineAuth) /* Same */
      {
        options->usePipelineAuth= !disablePipeline;
      }
    }
    return *this;
  }


  bool UrlParser::isAurora()
  {
    if (haMode ==HaMode::AURORA) {
      return true;
    }
    for (auto hostAddress : addresses)
    {
      if (std::regex_search(StringImp::get(hostAddress.toString()), AWS_PATTERN)) {
        return true;
      }
    }
    return false;
  }

  void UrlParser::parseUrl(const SQLString& url)
  {
    if (acceptsUrl(url)) {
      Properties dummy;
      parseInternal(*this, url, dummy);
    }
  }

  const SQLString& UrlParser::getUsername() const
  {
    return options->user;
  }

  void UrlParser::setUsername(const SQLString& username)
  {
    options->user= username;
  }

  SQLString& UrlParser::getPassword() {
    return options->password;
  }

  void UrlParser::setPassword(const SQLString& password) {
    options->password= password;
  }

  const SQLString& UrlParser::getDatabase() const {
    return database;
  }

  void UrlParser::setDatabase(const SQLString& database) {
    this->database= database;
  }

  std::vector<HostAddress>& UrlParser::getHostAddresses() {
    return this->addresses;
  }


  const Shared::Options& UrlParser::getOptions() const {
    return options;
  }


  void UrlParser::setProperties(const SQLString& urlParameters) {
    DefaultOptions::parse(this->haMode, urlParameters, this->options); setInitialUrl();
  }


  std::shared_ptr<CredentialPlugin> UrlParser::getCredentialPlugin() {
    return credentialPlugin;
  }

  const SQLString& UrlParser::toString() const {
    return initialUrl;
  }

  const SQLString& UrlParser::getInitialUrl() const {
    return initialUrl;
  }

  HaMode UrlParser::getHaMode() const {
    return haMode;
  }


  bool UrlParser::equals(UrlParser* parser)
  {
    if (this == parser) {
      return true;
    }
    /* TODO: empty/!empty not quite equivalent to =/!= NULL */
    return (!initialUrl.empty()
      ? initialUrl.compare(parser->getInitialUrl()) == 0
      : parser->getInitialUrl().empty())
      && (!getUsername().empty()
         ? getUsername().compare(parser->getUsername()) == 0
         : parser->getUsername().empty())
      && (!getPassword().empty()
         ? getPassword().compare(parser->getPassword()) == 0
         : parser->getPassword().empty());
  }


  int64_t UrlParser::hashCode() const
  {
    int64_t result= !options->password.empty() ? options->password.hashCode() : 0;
    result= 31 *result + (!options->user.empty() ? options->user.hashCode() : 0);
    result= 31 *result + initialUrl.hashCode();

    return result;
  }


  void UrlParser::loadMultiMasterValue()
  {
    if (haMode ==HaMode::SEQUENTIAL
      ||haMode ==REPLICATION
      ||haMode ==LOADBALANCE) {
      bool firstMaster= false;
      for (HostAddress host : addresses) {
        if (host.type.compare(ParameterConstant::TYPE_MASTER) == 0) {
          if (firstMaster) {
            multiMaster= true;
            return;
          }
          else {
            firstMaster= true;
          }
        }
      }
    }
    multiMaster= false;
  }


  bool UrlParser::isMultiMaster() {
    return multiMaster;
  }


  UrlParser* UrlParser::clone()
  {
    UrlParser *tmpUrlParser= new UrlParser(*this);
    tmpUrlParser->options.reset(options->clone());
    tmpUrlParser->addresses.assign(this->addresses.begin(), this->addresses.end());

    return tmpUrlParser;
  }
}
}
