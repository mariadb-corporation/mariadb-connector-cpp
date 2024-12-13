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


#include <cctype>
#include <array>

#include "Utils.h"

#include "LogQueryTool.h"
#include "logger/ProtocolLoggingProxy.h"
#include "protocol/MasterProtocol.h"


namespace sql
{
namespace mariadb
{
  Socket* createSocket()
  {
    return nullptr;
  }

  const char Utils::hexArray[]= "0123456789ABCDEF";
  SocketHandlerFunction* Utils::socketHandler;
  /**
    * Use standard socket implementation.
    *
    * @param options url options
    * @param host host to connect
    * @return socket
    * @throws IOException in case of error establishing socket.
    */
  Socket* Utils::standardSocket(Shared::Options& /*options*/, SQLString& /*host*/)
  {
    //SocketFactory* socketFactory;
    // This(socketFactory) is unlikely something we can do in C++. And unlikely we need and will support socketFactory

    //socketFactory= SocketFactory.getDefault();
    return /*socketFactory.*/sql::mariadb::createSocket();
  }

  /**
    * Escape String.
    *
    * @param value value to escape
    * @param noBackslashEscapes must backslash be escaped
    * @return escaped string.
    */
  SQLString Utils::escapeString(const SQLString& value, bool noBackslashEscapes)
  {
    if (!(value.find_first_of('\'') != std::string::npos)){
      if (noBackslashEscapes){
        return value;
      }
      if (!(value.find_first_of('\\') != std::string::npos)){
        return value;
      }
    }
    SQLString escaped= replace(value, "'", "''");
    if (noBackslashEscapes){
      return escaped;
    }
    return replace(escaped, "\\", "\\\\");
  }

  void Utils::escapeData(const char* in, size_t len, bool noBackslashEscapes, SQLString& out)
  {
    std::string &realOut= StringImp::get(out);
    out.reserve(out.length() + len + 64);

    //TODO: can be easily optimized
    if (noBackslashEscapes) {
      for (size_t i = 0; i < len; i++) {
        if (QUOTE == in[i]) {
          realOut.push_back(QUOTE);
        }
        realOut.push_back(in[i]);
      }
    }
    else {
      for (size_t i = 0; i < len; i++) {
        if (in[i] == QUOTE
          || in[i] == BACKSLASH
          || in[i] == DBL_QUOTE
          || in[i] == ZERO_BYTE) {
          realOut.push_back('\\');
        }
        realOut.push_back(in[i]);
      }
    }
  }


  /**
    * Copies the original byte array content to a new byte array. The resulting byte array is always
    * "length" size. If length is smaller than the original byte array, the resulting byte array is
    * truncated. If length is bigger than the original byte array, the resulting byte array is filled
    * with zero bytes.
    *
    * @param orig the original byte array
    * @param length how big the resulting byte array will be
    * @return the copied byte array
    */
#ifdef THIS_FUNCTION_MAKES_SENSE
  // So far looks like orig won't be char* in real life
  char* Utils::copyWithLength(char* orig, int32_t length)
  {
    char* result= new char[length];
    int32_t howMuchToCopy= length < orig.length ? length : orig.length;

    System.arraycopy(orig, 0, result, 0, howMuchToCopy);
    return result;
  }

  /**
    * Copies from original byte array to a new byte array. The resulting byte array is always
    * "to-from" size.
    *
    * @param orig the original byte array
    * @param from index of first byte in original byte array which will be copied
    * @param to index of last byte in original byte array which will be copied. This can be outside
    *     of the original byte array
    * @return resulting array
    */
  static char* copyRange(char* orig,int32_t from,int32_t to){
    int32_t length= to -from;
    char* result= new char[length];
    int32_t howMuchToCopy= orig.length -from <length ?orig.length -from :length;
    System.arraycopy(orig,from,result,0,howMuchToCopy);
    return result;
  }
#endif
  /**
    * Helper function to replace function parameters in escaped string. 3 functions are handles :
    *
    * <ul>
    *   <li>CONVERT(value, type): replacing SQL_XXX types to convertible type, i.e SQL_BIGINT to
    *       INTEGER
    *   <li>TIMESTAMPDIFF(type, ...): replacing type SQL_TSI_XXX in type with XXX, i.e SQL_TSI_HOUR
    *       with HOUR
    *   <li>TIMESTAMPADD(type, ...): replacing type SQL_TSI_XXX in type with XXX, i.e SQL_TSI_HOUR
    *       with HOUR
    * </ul>
    *
    * <p>caution: this use MariaDB server conversion: 'SELECT CONVERT('2147483648', INTEGER)' will
    * return a BIGINT. MySQL will throw a syntax error.
    *
    * @param functionString input string
    * @param protocol protocol
    * @return unescaped string
    */
  SQLString Utils::replaceFunctionParameter(const SQLString& functionString, Protocol* protocol)
  {
    const char *input= functionString.c_str();
    SQLString sb;
    size_t index;

    for (index= 0; index < functionString.length(); ++index)
    {
      if (input[index] != ' '){
        break;
      }
    }

    for (;
        ((input[index] >= 'a' && input[index] <= 'z') || (input[index] >= 'A' && input[index] <= 'Z'))
        && index < functionString.length();
        ++index)
    {
      sb.append(input[index]);
    }

    SQLString func(sb);
    func.toLowerCase();

    if (func.compare("convert") == 0)
    {
      size_t lastCommaIndex= functionString.find_last_of(',');
      size_t firstParentheses= functionString.find_first_of('(');
      SQLString value= functionString.substr(firstParentheses +1, lastCommaIndex);

      for (index= lastCommaIndex +1; index < functionString.length(); index++) {
        if (!std::isspace(input[index])) {
          break;
        }
      }

      size_t endParam= index + 1;
      for (; endParam <functionString.length(); endParam++) {
        if ((input[endParam] <'a'||input[endParam] >'z')
          &&(input[endParam] <'A'||input[endParam] >'Z')
          &&input[endParam] !='_') {
          break;
        }
      }
      SQLString typeParam(std::string(input + index, static_cast<size_t>(endParam - index)));
      typeParam.toUpperCase();

      if (typeParam.startsWith("SQL_"))
      {
        typeParam= typeParam.substr(4);
      }

      if (typeParam.compare("BOOLEAN") == 0)
      {
        return "1="+value;
      }
      else if (typeParam.compare("BIGINT") == 0 || typeParam.compare("SMALLINT") == 0 || typeParam.compare("TINYINT") == 0)
      {
        typeParam= "SIGNED INTEGER";
      }
      else if (typeParam.compare("BIT") == 0)
      {
        typeParam= "UNSIGNED INTEGER";
      }
      else if (typeParam.compare("BLOB") == 0 || typeParam.compare("VARBINARY") == 0 ||
        typeParam.compare("LONGVARBINARY") == 0 || typeParam.compare("ROWID") == 0)
      {
        typeParam= "BINARY";
      }
      else if (typeParam.compare("NCHAR") == 0 || typeParam.compare("CLOB") == 0 || typeParam.compare("NCLOB") == 0 ||
        typeParam.compare("DATALINK") == 0 || typeParam.compare("VARCHAR") == 0 || typeParam.compare("NVARCHAR") == 0 ||
        typeParam.compare("LONGVARCHAR") == 0 || typeParam.compare("LONGNVARCHAR") == 0 ||
        typeParam.compare("SQLXML") == 0 || typeParam.compare("LONGNCHAR") == 0)
      {
        typeParam= "CHAR";
      }
      else if (typeParam.compare("DOUBLE") == 0 || typeParam.compare("FLOAT") == 0)
      {
        if (protocol->isServerMariaDb() || protocol->versionGreaterOrEqual(8, 0, 17))
        {
          typeParam= "DOUBLE";
        }
        else
        {
          return "0.0+"+value;
        }
      }
      else if (typeParam.compare("REAL") == 0 || typeParam.compare("NUMERIC") == 0)
      {
        typeParam= "DECIMAL";
      }
      else if (typeParam.compare("TIMESTAMP") == 0)
      {
        typeParam= "DATETIME";
      }

      return std::string(input, 0, index) + typeParam + std::string(input, endParam, functionString.length() - endParam);
    }
    else if (func.compare("timestampdiff") == 0 || func.compare("timestampadd") == 0)
    {
      for (; index < functionString.length(); index++)
      {
        if (!std::isspace(input[index]) && input[index] !='(')
        {
          break;
        }
      }
      if (index < functionString.length() - 8)
      {
        SQLString paramPrefix(std::string(input, index, 8));
        if (paramPrefix.compare("SQL_TSI_") == 0)
        {
          return std::string(input, 0, index) + std::string(input, index + 8, functionString.length() - (index + 8));
        }
      }
      return functionString;
    }
    return functionString;
  }

  SQLString Utils::resolveEscapes(const SQLString& escaped, Protocol* protocol)
  {
    if (escaped.at(0) != '{' || escaped.at(escaped.size() - 1) != '}')
    {
      throw SQLException("unexpected escaped string");
    }
    size_t endIndex= escaped.size() - 1;
    SQLString escapedLower(escaped), buffer;
    escapedLower.toLowerCase();

    if (escaped.startsWith("{fn "))
    {
      const SQLString resolvedParams(replaceFunctionParameter(escaped.substr(4, endIndex - 4), protocol));
      return nativeSql(resolvedParams, buffer, protocol);
    }
    else if (escapedLower.startsWith("{oj "))
    {
      return nativeSql(escaped.substr(4, endIndex - 4), buffer, protocol);
    }
    else if (escaped.startsWith("{d "))
    {
      return escaped.substr(3, endIndex - 3);
    }
    else if (escaped.startsWith("{t "))
    {
      return escaped.substr(3, endIndex - 3);
    }
    else if (escaped.startsWith("{ts "))
    {
      return escaped.substr(4, endIndex - 4);
    }
    else if (escaped.startsWith("{d'"))
    {
      return escaped.substr(2, endIndex - 2);
    }
    else if (escaped.startsWith("{t'"))
    {
      return escaped.substr(2, endIndex - 2);
    }
    else if (escaped.startsWith("{ts'"))
    {
      return escaped.substr(3, endIndex - 3);
    }
    else if (escapedLower.startsWith("{call "))
    {
      return nativeSql(escaped.substr(1, endIndex - 1), buffer, protocol);
    }
    else if (escaped.startsWith("{escape "))
    {
      return escaped.substr(1, endIndex - 1);
    }
    else if (escaped.startsWith("{?"))
    {
      if (escaped[2] == '=') {
        return nativeSql(escaped.substr(3, endIndex - 3), buffer, protocol);
      }
      else {
        return nativeSql(escaped.substr(2, endIndex - 2), buffer, protocol);
      }
    }
    else if (escaped.startsWith("{ ") || escaped.startsWith("{\n"))
    {
      for (size_t i= 2;i < escaped.size();i++)
      {
        if (!std::isspace(escaped.at(i)))
        {
          SQLString tmp("{");
          tmp.append(escaped.substr(i));
          return resolveEscapes(tmp, protocol);
        }
      }
    }
    else if (escaped.startsWith("{\r\n"))
    {
      for (size_t i= 3;i < escaped.size();i++)
      {
        if (!std::isspace(escaped.at(i)))
        {
          SQLString tmp("{");
          tmp.append(escaped.substr(i));
          return resolveEscapes(tmp, protocol);
        }
      }
    }
    return escaped;
    //throw SQLException("unknown escape sequence "+escaped);
  }


  const SQLString& Utils::nativeSql(const SQLString& sqlStr, SQLString& modifiedSqlBuf, Protocol* protocol)
  {
    const std::string &sql= StringImp::get(sqlStr);
    if (sql.find_first_of('{') == std::string::npos) {
      return sqlStr;
    }

    SQLString escapeSequence;
    std::string& escapeSequenceBuf= StringImp::get(escapeSequence);
    std::string& sqlBuffer= StringImp::get(modifiedSqlBuf);

    char lastChar= 0;
    bool inQuote= false;
    char quoteChar= 0;
    bool inComment= false;
    bool isSlashSlashComment= false;
    int32_t inEscapeSeq= 0;

    // To stars something from
    sqlBuffer.reserve((sql.length() + 7) / 8 * 8);
    escapeSequenceBuf.reserve(std::min(sql.length(), std::size_t(64)));

    for (auto it= sql.begin(); it < sql.end(); ++it)
    {
      char car= *it;
      if (lastChar == '\\' && !protocol->noBackslashEscapes()) {
        sqlBuffer.append(1, car);
        lastChar = 0;
        continue;
      }

      switch (car)
      {
        case '\'':
        case '"':
        case '`':
          if (!inComment){
            if (inQuote){
              if (quoteChar == car){
                inQuote= false;
              }
            }else {
              inQuote= true;
              quoteChar= car;
            }
          }
          break;

        case '*':
          if (!inQuote && !inComment && lastChar =='/'){
            inComment= true;
            isSlashSlashComment= false;
          }
          break;
        case '/':
        case '-':
          if (!inQuote) {
            if (inComment) {
              if (lastChar == '*' && !isSlashSlashComment) {
                inComment= false;
              }else if (lastChar == car && isSlashSlashComment) {
                inComment= false;
              }
            }else {
              if (lastChar ==car){
                inComment= true;
                isSlashSlashComment= true;
              }else if (lastChar == '*'){
                inComment= true;
                isSlashSlashComment= false;
              }
            }
          }
          break;
        case '\n':
          if (inComment && isSlashSlashComment) {
            inComment= false;
          }
          break;
        case '{':
          if (!inQuote && !inComment) {
            ++inEscapeSeq;
          }
          break;

        case '}':
          if (!inQuote && !inComment) {
            inEscapeSeq--;
            if (inEscapeSeq == 0) {
              escapeSequenceBuf.append(1, car);
              sqlBuffer.append(resolveEscapes(escapeSequence, protocol));
              escapeSequenceBuf= "";
              continue;
            }
          }
          break;

        default:
          break;
      }
      lastChar= car;
      if (inEscapeSeq > 0) {
        escapeSequenceBuf.append(1, car);
      } else {
        sqlBuffer.append(1, car);
      }
    }
    if (inEscapeSeq > 0){
      throw SQLException(
          "Invalid escape sequence , missing closing '}' character in '"+sqlBuffer);
    }
    return modifiedSqlBuf;
  }

  /**
    * Retrieve protocol corresponding to the failover options. if no failover option, protocol will
    * not be proxied. if a failover option is precised, protocol will be proxied so that any
    * connection error will be handle directly.
    *
    * @param urlParser urlParser corresponding to connection url string.
    * @param globalInfo global variable information
    * @return protocol
    * @throws SQLException if any error occur during connection
    */
  Shared::Protocol Utils::retrieveProxy(Shared::UrlParser& urlParser, GlobalStateInfo* globalInfo)
  {
    switch (urlParser->getHaMode())
    {
      case AURORA:
#ifdef AURORA_SUPPORT_IMPLEMENTED
        return getProxyLoggingIfNeeded(
            urlParser,
            (Protocol&)
            newProxyInstance(
              AuroraProtocol,
              Protocol,
              new FailoverProxy(new AuroraListener(urlParser,globalInfo))));
#endif
      case REPLICATION:
#ifdef REPLICATION_SUPPORT_IMPLEMENTED
        return getProxyLoggingIfNeeded(
            urlParser,
            (Protocol&)
            Proxy.newProxyInstance(
              MastersSlavesProtocol.class.getClassLoader(),
              new Class[] {Protocol&.class},
              new FailoverProxy(new MastersSlavesListener(urlParser,globalInfo))));
#endif
      case LOADBALANCE:
#ifdef LOADBALANCE_SUPPORT_IMPLEMENTED
        return getProxyLoggingIfNeeded(
            urlParser,
            (Protocol&)
            Proxy.newProxyInstance(
              MasterProtocol.class.getClassLoader(),
              new Class[] {Protocol&.class},
              new FailoverProxy(new MastersFailoverListener(urlParser,globalInfo))));
#else
        /* This exception supposed to be already thrown*/
        throw SQLFeatureNotImplementedException(SQLString("Support of the HA mode") + HaModeStrMap[urlParser->getHaMode()] + "is not yet implemented");
#endif
      case SEQUENTIAL:
      default:
        Shared::Protocol protocol(getProxyLoggingIfNeeded(urlParser, new MasterProtocol(urlParser, globalInfo)));
        protocol->connectWithoutProxy();

        return protocol;
    }
  }

  Protocol* Utils::getProxyLoggingIfNeeded(const Shared::UrlParser &urlParser, Protocol* protocol)
  {
    /* TODO: profileSql and slowQueryThresholdNanos should be probably hidded/disabled*/
    if (urlParser->getOptions()->profileSql
        || urlParser->getOptions()->slowQueryThresholdNanos > 0)
    {
      Shared::Protocol shProt(protocol);
      protocol= new ProtocolLoggingProxy(shProt, urlParser->getOptions());

      return protocol;
    }
    /* Probably reference won't work at some point */
    return protocol;
  }

  /**
    * Get timezone from Id. This differ from java implementation : by default, if timezone Id is
    * unknown, java return GMT timezone. GMT will be return only if explicitly asked.
    *
    * @param id timezone id
    * @return timezone.
    * @throws SQLException if no timezone is found for this Id
    */
#ifdef WE_DEAL_WITH_TIMEZONES
  static TimeZone& getTimeZone(const SQLString& id)
  {
    TimeZone& tz= TimeZone::getTimeZone(id);

    if (tz.getID().compare("GMT") == 0 && id.compare(id) != 0)
    {
      throw SQLException("invalid timezone id '" + id + "'");
    }
    return tz;
  }
#endif

  /**
    * Create socket accordingly to options.
    *
    * @param options Url options
    * @param host hostName ( mandatory only for named pipe)
    * @return a nex socket
    * @throws IOException if connection error occur
    */
  Socket* Utils::createSocket(Shared::Options& options, const SQLString& host)
  {
    return socketHandler->apply(options, host);
  }

  /**
    * Hexdump.
    *
    * @param bytes byte arrays
    * @return String
    */
  SQLString Utils::hexdump(const char* bytes, int32_t length)
  {
    return hexdump(INT32_MAX, 0, INT32_MAX, bytes, length);
  }

  /**
    * Hexdump.
    *
    * <p>String output example :
    *
    * <pre>{@code
    * 7D 00 00 01 C5 00 00                                 }......            &lt;- first byte array
    * 01 00 00 01 02 33 00 00  02 03 64 65 66 05 74 65     .....3....def.te   &lt;- second byte array
    * 73 74 6A 0A 74 65 73 74  5F 62 61 74 63 68 0A 74     stj.test_batch.t
    * 65 73 74 5F 62 61 74 63  68 02 69 64 02 69 64 0C     est_batch.id.id.
    * 3F 00 0B 00 00 00 03 03  42 00 00 00 37 00 00 03     ?.......B...7...
    * 03 64 65 66 05 74 65 73  74 6A 0A 74 65 73 74 5F     .def.testj.test_
    * 62 61 74 63 68 0A 74 65  73 74 5F 62 61 74 63 68     batch.test_batch
    * 04 74 65 73 74 04 74 65  73 74 0C 21 00 1E 00 00     .test.test.!....
    * 00 FD 00 00 00 00 00 05  00 00 04 FE 00 00 22 00     ..............".
    * 06 00 00 05 01 31 03 61  61 61 06 00 00 06 01 32     .....1.aaa.....2
    * 03 62 62 62 06 00 00 07  01 33 03 63 63 63 06 00     .bbb.....3.ccc..
    * 00 08 01 34 03 61 61 61  06 00 00 09 01 35 03 62     ...4.aaa.....5.b
    * 62 62 06 00 00 0A 01 36  03 63 63 63 05 00 00 0B     bb.....6.ccc....
    * FE 00 00 22 00                                       ...".
    * }</pre>
    *
    * @param maxQuerySizeToLog max log size
    * @param offset offset of last byte array
    * @param length length of last byte array
    * @param byteArr byte arrays. if many, only the last may have offset and size limitation others
    *     will be displayed completely.
    * @return String
    */
    SQLString Utils::hexdump(int32_t maxQuerySizeToLog, int32_t offset, int32_t length, const char* byteArr, int32_t arrLen)
    {
    switch (arrLen){
      case 0:
        return "";

      case 1:
      {
        const char *bytes= byteArr;
        if (arrLen <= offset) {
          return "";
        }
        int32_t dataLength= std::min(maxQuerySizeToLog, std::min(arrLen - offset, length));

        SQLString outputBuilder;
        outputBuilder.reserve(dataLength*5);
        outputBuilder.append("\n");
        writeHex(bytes, arrLen, offset, dataLength, outputBuilder);

        return outputBuilder;
      }
      default:
        SQLString sb;
        sb.append("\n");
        const char* arr;
        /* TODO: Seems lile byteArr here is array of arrays. so all this(family of functions) has to be changed */
        for (int32_t i= 0; i < arrLen - 1; i++)
        {
          arr= byteArr;
          writeHex(arr, arrLen, 0, arrLen, sb);
        }
        //arr= byteArr[arrLen -1];
        //int32_t dataLength2= std::min(maxQuerySizeToLog, std::min(arrLen - offset, length));
        //writeHex(arr, offset, dataLength2, sb);
        return sb;
    }
  }

  /**
    * Write bytes/hexadecimal value of a byte array to a StringBuilder.
    *
    * <p>String output example :
    *
    * <pre>{@code
    * 38 00 00 00 03 63 72 65  61 74 65 20 74 61 62 6C     8....create tabl
    * 65 20 42 6C 6F 62 54 65  73 74 63 6C 6F 62 74 65     e BlobTestclobte
    * 73 74 32 20 28 73 74 72  6D 20 74 65 78 74 29 20     st2 (strm text)
    * 43 48 41 52 53 45 54 20  75 74 66 38                 CHARSET utf8
    * }</pre>
    *
    * @param bytes byte array
    * @param offset offset
    * @param dataLength byte length to write
    * @param outputBuilder string builder
    */
  void Utils::writeHex(const char* bytes, int32_t arrLen, int32_t offset,int32_t dataLength, SQLString& outputBuilder){


    if (arrLen == 0){
      return;
    }

    char hexaValue[16];
    hexaValue[8]= ' ';

    int32_t pos= offset;
    int32_t posHexa= 0;

    while (pos <dataLength +offset){
      int32_t byteValue= bytes[pos] & 0xFF;
      outputBuilder
        .append(hexArray[byteValue >> 4])
        .append(hexArray[byteValue & 0x0F])
        .append(" ");

      hexaValue[posHexa++]= (byteValue >31 &&byteValue <127)?(char)byteValue :'.';

      if (posHexa ==8){
        outputBuilder.append(" ");
      }
      if (posHexa ==16){
        outputBuilder.append("    ").append(hexaValue).append("\n");
        posHexa= 0;
      }
      pos++;
    }

    int32_t remaining= posHexa;
    if (remaining >0){
      if (remaining <8){
        for (;remaining <8;remaining++){
          outputBuilder.append("   ");
        }
        outputBuilder.append(" ");
      }

      for (;remaining <16;remaining++){
        outputBuilder.append("   ");
      }

      outputBuilder.append("    ").append(std::string(hexaValue, 0, posHexa)).append("\n");
    }
  }

  SQLString Utils::getHex(const char* raw, size_t arrLen)
  {
    SQLString hex;
    hex.reserve(2 * arrLen);
    for (size_t i= 0; i < arrLen; ++i)
    {
      char b= raw[i];
      hex.append(hexArray[(b &0xF0)>>4]).append(hexArray[(b &0x0F)]);
    }
    return hex;
  }

  SQLString Utils::byteArrayToHexString(const char* bytes, size_t arrLen)
  {
    return (arrLen > 0 ? getHex(bytes, arrLen) : "");
  }

  /**
    * Convert int value to hexadecimal String.
    *
    * @param value value to transform
    * @return Hexadecimal String value of integer.
    */
  SQLString Utils::intToHexString(int32_t value)
  {
    SQLString hex;
    int32_t offset= 24;
    char b;
    bool nullEnd= false;

    while (offset >= 0)
    {
      b= (char)(value >> offset);
      offset -=8;
      if (b !=0 ||nullEnd)
      {
        nullEnd= true;
        hex.append(hexArray[(b & 0xF0) >> 4]).append(hexArray[(b & 0x0F)]);
      }
    }
    return hex;
  }

  /**
    * Parse the option "sessionVariable" to ensure having no injection. semi-column not in string
    * will be replaced by comma.
    *
    * @param sessionVariable option value
    * @return parsed String
    */
  SQLString Utils::parseSessionVariables(SQLString& sessionVariable)
  {
    SQLString out;
    SQLString sb;
    Parse state= Parse::Normal;
    bool iskey= true;
    bool singleQuotes= true;
    bool first= true;
    SQLString key;

    for (auto car : sessionVariable)
    {
      if (state == Parse::Escape)
      {
        sb.append(car);
        state= singleQuotes ? Parse::Quote : Parse::String;
        continue;
      }

      switch (car){
        case '"':
          if (state ==Parse::Normal)
          {
            state= Parse::String;
            singleQuotes= false;
          }else if (state == Parse::String && !singleQuotes){
            state= Parse::Normal;
          }
          break;

        case '\'':
          if (state ==Parse::Normal){
            state= Parse::String;
            singleQuotes= true;
          }else if (state == Parse::String &&singleQuotes){
            state= Parse::Normal;
          }
          break;

        case '\\':
          if (state == Parse::String){
            state= Parse::Escape;
          }
          break;

        case ';':
        case ',':
          if (state ==Parse::Normal){
            if (!iskey){
              if (!first){
                out.append(",");
              }
              out.append(key);
              out.append(sb);
              first= false;
            }else {
              key= sb.trim();
              if (!key.empty())
              {
                if (!first){
                  out.append(",");
                }
                out.append(key);
                first= false;
              }
            }
            iskey= true;
            key= "";
            sb= "";
            continue;
          }
          break;

        case '=':
          if (state == Parse::Normal && iskey)
          {
            key= sb;
            key.trim();
            iskey= false;
            sb= "";
          }
          break;
        //default:
        //  // Nothing
        //  1;
      }

      sb.append(car);
    }

    if (!iskey){
      if (!first){
        out.append(",");
      }
      out.append(key);
      out.append(sb);
    }else {
      SQLString tmpkey= sb;

      tmpkey.trim();
      if (!tmpkey.empty()&&!first){
        out.append(",");
      }
      out.append(tmpkey);
    }
    return out;
  }

  bool Utils::isIPv4(const SQLString& ip)
  {
    /* IP_V4(
    "^(([1-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){1}"
      "(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){2}"
      "([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$"); */
    Tokens groups= split(ip, ".");
    if (groups->size() == 4) {
      for (auto& group : *groups) {
        if (group.length() > 3)
          return false;
        for (auto c : StringImp::get(group)) {
          if (!std::isdigit(c))
            return false;
        }
        if (group.size() == 3 &&
            (group.at(0) > '2' ||
             (group.at(0) == '2' &&
              (group.at(1) > 5 || (group.at(1) == 5 && group.at(2) > 5)))))
          return false;
      }
    }
    return false;
  }

  bool Utils::isIPv6(const SQLString& ip)
  {
    /*      IP_V6("^[0-9a-fA-F]{1,4}(:[0-9a-fA-F]{1,4}){7}$")
IP_V6_COMPRESSED( "^(([0-9A-Fa-f]{1,4}(:[0-9A-Fa-f]{1,4}){0,5})?)::(([0-9A-Fa-f]{1,4}(:[0-9A-Fa-f]{1,4}){0,5})?)$"); */
    Tokens groups= split(ip, ":");
    auto& g= *groups;
    std::size_t end= g.size();

    if (end < 9 && end > 3) {
      bool noMoreEmpty= false;
      std::size_t begin= 0;
      if (g[0].empty()) {
        if (!g[1].empty()) {
          return false;
        }
        begin= 2;
        noMoreEmpty= true;
      }
      else if (g[end - 1].empty()) {
        if (!g[end - 2].empty()) {
          return false;
        }
        end-= 2;
        noMoreEmpty= true;
      }
      for (auto i= begin; i < end; ++i) {
        if (g[i].length() > 4) {
          return false;
        }
        if (g[i].length() == 0) {
          if (noMoreEmpty) {
            return false;
          }
          else {
            noMoreEmpty= true;
          }
        }
        else {
          for (auto c : StringImp::get(g[i])) {
            if (!std::isxdigit(c)) {
              return false;
            }
          }
        }
      }
    }
    return false;
  }

  /**
    * Traduce a String value of transaction isolation to corresponding java value.
    *
    * @param txIsolation String value
    * @return java corresponding value (Connection.TRANSACTION_READ_UNCOMMITTED,
    *     Connection.TRANSACTION_READ_COMMITTED, Connection.TRANSACTION_REPEATABLE_READ or
    *     Connection.TRANSACTION_SERIALIZABLE)
    * @throws SQLException if String value doesn't correspond
    *     to @@tx_isolation/@@transaction_isolation possible value
    */
  int32_t Utils::transactionFromString(const SQLString& txIsolation)
  {
    if (txIsolation.compare("READ-UNCOMMITTED") == 0)
    {
      return sql::TRANSACTION_READ_UNCOMMITTED;
    }
    else if (txIsolation.compare("READ-COMMITTED") == 0)
    {
      return sql::TRANSACTION_READ_COMMITTED;
    }
    else if (txIsolation.compare("REPEATABLE-READ") == 0)
    {
      return sql::TRANSACTION_REPEATABLE_READ;
    }
    else if (txIsolation.compare("SERIALIZABLE") == 0)
    {
      return sql::TRANSACTION_SERIALIZABLE;
    }
    else
    {
      throw SQLException("unknown transaction isolation level");
    }
  }

  /* The function expects that sring starting at it is not shorter then len, and str is lowercase */
  bool Utils::strnicmp(std::string::const_iterator & it, const char * str, std::size_t len)
  {
    while (len--) {
      if (std::tolower(*it) != *str++) {
        return true;
      }
      // Not movivng the iterator before we know current characters are equeal
      ++it;
    }
    return false;
  }


  std::size_t Utils::findstrni(const std::string & str, const char* substr, std::size_t len)
  {
    const char first[2]= {*substr, static_cast<char>(std::toupper(*substr))};
    std::size_t pos= 0, prev= 0;
    const std::size_t firstbad= str.length() - len + 1;

    /*Mix of iterator and index opertaions makes me think, that it could be done better */
    while ((pos= str.find_first_of(first, prev, 2)) < firstbad) {
      auto it= str.cbegin() + pos;
      if (!strnicmp(it, substr + 1, len - 1)) {
        return pos;
      }
      prev= pos + 1;
    }
    return std::string::npos;
  }


  std::string::const_iterator isLoadDataLocalInFile(const std::string& sql)
  {
    auto cit= sql.cbegin();

    Utils::skipCommentsAndBlanks(sql, cit);
    
    if (sql.cend() - cit < 23/* min len that, LOAD DATA LOCAL INFILE? can take */) {
      return sql.cend();
    }
    if (Utils::strnicmp(cit, "load", 4)) {
      return sql.cend();
    }
    Utils::skipCommentsAndBlanks(sql, cit);
    if (sql.cend() - cit < 18 /* DATA LOCAL INFILE? */) {
      return sql.cend();
    }
    if (Utils::strnicmp(cit, "data", 4)) {
      return sql.cend();
    }
    Utils::skipCommentsAndBlanks(sql, cit);

    auto optional= cit;
    bool move= false;
    if (sql.cend() - cit > 22) // Have enough place with optional CONCURRENT
    {
      if (!Utils::strnicmp(optional, "concurrent", 10)) {
        move= true;
      }
    }
    if (!move && (sql.end() - cit) > 24) // Have enough place with optional CONCURRENT
    {
      optional= cit;
      if (!Utils::strnicmp(optional, "low_priority", 12)) {
        move= true;
      }
    }
    if (move)
      cit= optional;

    if (sql.cend() - cit < 13 /* LOCAL INFILE? */) {
      return sql.cend();
    }
    if (Utils::strnicmp(cit, "local", 5)) {
      return sql.cend();
    }
    Utils::skipCommentsAndBlanks(sql, cit);
    if (sql.cend() - cit < 7 /* INFILE? */) {
      return sql.cend();
    }
    if (Utils::strnicmp(cit, "infile", 6)) {
      return sql.cend();
    }
    Utils::skipCommentsAndBlanks(sql, cit);

    return cit;
  }

  /**
    * Validate that file name correspond to send query.
    *
    * @param sql sql command
    * @param parameters sql parameter
    * @param fileName server file name
    * @return true if correspond
    */
  bool Utils::validateFileName( const SQLString& query, std::vector<ParameterHolder*>& parameters, const SQLString& fileName)
  {
    /*pattern(
         ("^(\\s*\\/\\*([^\\*]|\\*[^\\/])*\\*\\/)*\\s*LOAD\\s+DATA\\s+((LOW_PRIORITY|CONCURRENT)\\s+)?LOCAL\\s+INFILE\\s+'"
         +fileName
         +"'").c_str()*/
    const std::string &sql= StringImp::get(query);
    auto filename= isLoadDataLocalInFile(sql);

    if (filename < sql.cend()) {
      SQLString fn(fileName);
      fn.toLowerCase();
      if (parameters.size() > 0) {
        if (*filename == '?') {
          SQLString param(parameters[0]->toString().toLowerCase());
          return (param.compare("'" + fn + "'") == 0);
        }
      }
      else if (sql.cend() >= filename + fn.length()) {
        return !Utils::strnicmp(filename, fn, fn.length());
      }
    }
    return false;
  }


  std::size_t Utils::tokenize(std::vector<sql::bytes>& tokens, const char * cstring, const char * separator)
  {
    const char *current= cstring, *next= nullptr, *end= cstring + strlen(cstring);
    while ((next= std::strpbrk(current, separator)) != nullptr)
    {
      /* This is rather bad CArray API - constructor from const array creates copy, while constructor from array creates "wrapping" object,
       and here we need the wrapping one - there is no need to create copy, plus copy will not terminate array with \0, and that will create
       problems. Possibly more clear way would be to push empty object and than explicitly wrap the string area we need */
      tokens.emplace_back(const_cast<char*>(current), next - current);
      current= next + 1;
    }
    if (current < end)
    {
      tokens.emplace_back(const_cast<char*>(current), end - current);
    }
    return tokens.size();
  }

  std::string::const_iterator& Utils::skipCommentsAndBlanks(const std::string &sql, std::string::const_iterator &start)
  {
    LexState state= LexState::Normal;
    char lastChar= '\0';

    for (; start < sql.cend(); ++start) {
      char car= *start;

      switch (car) {
      case '*':
        if (state == LexState::Normal && lastChar == '/') {
          state= LexState::SlashStarComment;
        }
        break;

      case '/':
        if (state == LexState::SlashStarComment && lastChar == '*') {
          state= LexState::Normal;
        }
        else if (state == LexState::Normal && lastChar == '/') {
          state= LexState::EOLComment;
        }
        break;

      case '#':
        if (state == LexState::Normal) {
          state= LexState::EOLComment;
        }
        break;

      case '-':
        if (state == LexState::Normal && lastChar == '-') {
          state= LexState::EOLComment;
        }
        break;

      case '\n':
        if (state == LexState::EOLComment) {
          state= LexState::Normal;
        }
        break;
      default:
        if (state == LexState::Normal) {
          if (std::isspace(car)) {
            continue;
          }
          else {
            return start;
          }
        }
      }
      lastChar= car;
    }
    /* Like we have only comments and/or blanks */
    return start;
  }


  std::size_t Utils::skipCommentsAndBlanks(const std::string &sql, std::size_t start)
  {
    auto it= sql.cbegin() + start;
    skipCommentsAndBlanks(sql, it);
    return it - sql.cbegin();
  }
}
}
