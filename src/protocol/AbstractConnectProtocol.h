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


#ifndef _ABSTRACTCONNECTPROTOCOL_H_
#define _ABSTRACTCONNECTPROTOCOL_H_

#include <atomic>
#include <map>

#include "Consts.h"

#include "Protocol.h"

#include "pool/GlobalStateInfo.h"

namespace sql
{
namespace mariadb
{
  class PacketInputStream;
  class PacketOutputStream;
  class Socket;
  class SSLSocket;
  class Credential;

  class AbstractConnectProtocol : public Protocol
  {

    static const SQLString SESSION_QUERY; /*("SELECT @@max_allowed_packet,"
    +"@@system_time_zone,"
    +"@@time_zone,"
    +"@@auto_increment_increment")
    .getBytes(StandardCharsets.UTF_8)*/
    static const SQLString IS_MASTER_QUERY; /*"select @@innodb_read_only".getBytes(StandardCharsets.UTF_8)*/
    static const Shared::Logger logger; /*LoggerFactory.getLogger(typeid(AbstractConnectProtocol))*/

  protected:
    Shared::mutex lock;
    const UrlParser& urlParser;
    const Shared::Options options;

  private:
    const SQLString username;
    //const LruTraceCache traceCache; /*new LruTraceCache()*/
    const GlobalStateInfo globalInfo;

  public:
    bool hasWarningsFlag; /*false*/
    Results* activeStreamingResult; /*NULL*/
    int16_t serverStatus;

  protected:
    int32_t autoIncrementIncrement;
    Socket* socket;
    PacketOutputStream* writer;
    bool readOnly; /*false*/
    PacketInputStream* reader;
    FailoverProxy* proxy;
    volatile bool connected; /*false*/
    bool explicitClosed; /*false*/
    SQLString database;
    int64_t serverThreadId;
    ServerPrepareStatementCache* serverPrepareStatementCache;
    bool eofDeprecated; /*false*/
    int64_t serverCapabilities;
    int32_t socketTimeout;

  private:
    HostAddress currentHost;
    bool hostFailed;
    SQLString serverVersion;
    bool serverMariaDb;
    int32_t majorVersion;
    int32_t minorVersion;
    int32_t patchVersion;
    TimeZone* timeZone;

  public:
    AbstractConnectProtocol(const UrlParser& urlParser, const GlobalStateInfo& globalInfo, Shared::mutex& lock);

  private:
    static void closeSocket(PacketInputStream* packetInputStream, PacketOutputStream* packetOutputStream, Socket* socket);
    static Socket createSocket(const SQLString& host, int32_t port, const Shared::Options& options);
    static int64_t initializeClientCapabilities(const Shared::Options& options, int64_t serverCapabilities, const SQLString& database);
    static void enabledTlsProtocolSuites(SSLSocket* sslSocket, const Shared::Options& options);
    static void enabledTlsCipherSuites(SSLSocket* sslSocket, const Shared::Options& options);

  public:
    void close();
    void abort();

  private:
    void forceAbort();
    void abortActiveStream();

  public:
    void skip();

  private:
    void cleanMemory();

  public:
    void setServerStatus(uint32_t serverStatus);
    void removeHasMoreResults();
    void connect();

  private:
    /* hostAddress may be NULL (e.g. pipe)*/
    void createConnection(HostAddress* hostAddress, const SQLString& username);

  public:
    void destroySocket();

  private:

    void sslWrapper(
      const SQLString& host,
      const Socket* socket,
      const Shared::Options& options,
      const int64_t serverCapabilities,
      int64_t clientCapabilities,
      const int8_t exchangeCharset,
      int64_t serverThreadId);

    void authenticationHandler(
      int8_t exchangeCharset,
      int64_t clientCapabilities, const SQLString& authenticationPluginType,
      sql::bytes& seed,
      const Shared::Options& options, const SQLString& database,
      Credential* credential, const SQLString& host);


    void compressionHandler(Shared::Options options);
    void assignStream(Socket socket, Shared::Options options);
    void postConnectionQueries();
    void sendPipelineAdditionalData();
    void sendSessionInfos();
    void sendRequestSessionVariables();
    void readRequestSessionVariables(std::map<SQLString, SQLString> serverData);
    void sendCreateDatabaseIfNotExist(const SQLString& quotedDb);
    void sendUseDatabaseIfNotExist(const SQLString& quotedDb);
    void readPipelineAdditionalData(std::map<SQLString, SQLString>serverData);
    void requestSessionDataWithShow(std::map<SQLString, SQLString>serverData);
    void additionalData(std::map<SQLString, SQLString>serverData);

  public:
    bool isClosed();

  private:
    void loadCalendar(const SQLString& srvTimeZone, const SQLString& srvSystemTimeZone);

  public:
    bool checkIfMaster();

  private:
    int8_t decideLanguage(int32_t serverLanguage);

  public:
    void readEofPacket();
    void skipEofPacket();
    void setHostFailedWithoutProxy();
    UrlParser& getUrlParser();
    bool isMasterConnection();

  private:
    void sendPipelineCheckMaster();

  public:
    void readPipelineCheckMaster();
    bool mustBeMasterConnection();
    bool noBackslashEscapes();
    void connectWithoutProxy();
    bool shouldReconnectWithoutProxy();
    SQLString getServerVersion();
    bool getReadonly();
    void setReadonly(bool readOnly);
    HostAddress& getHostAddress();
    void setHostAddress(HostAddress& host);
    const SQLString& getHost() const;
    FailoverProxy* getProxy();
    void setProxy(FailoverProxy* proxy);
    int32_t getPort() const;
    SQLString getDatabase();
    const SQLString& getUsername();

  private:
    void parseVersion(const SQLString& serverVersion);

  public:
    int32_t getMajorServerVersion();
    int32_t getMinorServerVersion();
    bool versionGreaterOrEqual(int32_t major, int32_t minor, int32_t patch);

    bool getPinGlobalTxToPhysicalConnection();
    bool hasWarnings();
    bool isConnected();
    int64_t getServerThreadId();
    Socket getSocket();
    bool isExplicitClosed();
    TimeZone* getTimeZone();
    Shared::Options& getOptions();
    void setHasWarnings(bool hasWarnings);
    Results* getActiveStreamingResult();
    void setActiveStreamingResult(Shared::Results& activeStreamingResult);
    void removeActiveStreamingResult();
    Shared::mutex& getLock();
    bool hasMoreResults();
    ServerPrepareStatementCache* prepareStatementCache();
    void executeQuery(const SQLString& sql);
    void changeSocketTcpNoDelay(bool setTcpNoDelay);
    void changeSocketSoTimeout(int32_t setSoTimeout);
    bool isServerMariaDb();
    PacketInputStream* getReader();
    PacketOutputStream* getWriter();
    bool isEofDeprecated();
    bool sessionStateAware();
    SQLString getTraces();
  };
}
}
#endif