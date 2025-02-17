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


#ifndef _MADBCPPUTILS_H_
#define _MADBCPPUTILS_H_

#include <mutex>
#include <vector>

#include "UrlParser.h"
#include "Protocol.h"
#include "pool/GlobalStateInfo.h"
#include "Consts.h"
#include "parameters/ParameterHolder.h"
#include "io/SocketHandlerFunction.h"

namespace sql
{
namespace mariadb
{
class Socket;

class Utils
{
  static const char hexArray[]; /*"0123456789ABCDEF".toCharArray()*/
  static SocketHandlerFunction* socketHandler;

public:
  static Socket* standardSocket(Shared::Options& options, SQLString& host);

  static SQLString escapeString(const SQLString& value, bool noBackslashEscapes);
  static void escapeData(const char* in, size_t len, bool noBackslashEscapes, SQLString& out);
#ifdef THIS_FUNCTION_MAKES_SENSE
  static char* copyWithLength(char* orig,int32_t length);
  static char* copyRange(char* orig,int32_t from,int32_t to);
#endif
private:
  static SQLString replaceFunctionParameter(const SQLString& functionString, Protocol* protocol);
  static SQLString resolveEscapes(const SQLString& escaped, /*SQLString& buffer,*/ Protocol* protocol);
public:
  static const SQLString& nativeSql(const SQLString& sql, SQLString& modifiedSqlBuf, Protocol* protocol);
  static Shared::Protocol retrieveProxy(Shared::UrlParser& urlParser, GlobalStateInfo* globalInfo);

private:
  static Protocol* getProxyLoggingIfNeeded(const Shared::UrlParser& urlParser, Protocol* protocol);
public:
#ifdef WE_DEAL_WITH_TIMEZONES
  static TimeZone& getTimeZone(const SQLString& id);
#endif
  static Socket* createSocket(Shared::Options& options, const SQLString& host);
  static SQLString hexdump(const char* bytes, int32_t arrLen);
  static SQLString hexdump(int32_t maxQuerySizeToLog, int32_t offset, int32_t length, const char* byteArr, int32_t arrLen);
private:
  static void writeHex(const char* bytes, int32_t arrLen, int32_t offset,int32_t dataLength, SQLString& outputBuilder);
  static SQLString getHex(const char* raw, size_t arrLen);
public:
  static SQLString byteArrayToHexString(const char* bytes, size_t arrLen);
  static SQLString intToHexString(int32_t value);
  static SQLString parseSessionVariables(SQLString& sessionVariable);
  static bool isIPv4(const SQLString& ip);
  static bool isIPv6(const SQLString& ip);
  static int32_t transactionFromString(const SQLString& txIsolation);
  static bool strnicmp(std::string::const_iterator &it, const char *str, std::size_t len);
  static std::size_t findstrni(const std::string &str, const char* substr, std::size_t len);
  static bool validateFileName(const SQLString& sql, std::vector<ParameterHolder*>& parameters, const SQLString& fileName);
  static std::size_t tokenize(std::vector<sql::bytes>& tokens, const char* cstring, const char *separator);
  static std::string::const_iterator& skipCommentsAndBlanks(const std::string &sql, std::string::const_iterator& start);
  static std::size_t skipCommentsAndBlanks(const std::string &sql, std::size_t start= 0);

  enum Parse {
    Normal,
    String,
    Quote,
    Escape
  };
};

}
}
#endif
