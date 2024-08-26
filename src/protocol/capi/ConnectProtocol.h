/************************************************************************************
   Copyright (C) 2020,2024 MariaDB Corporation plc

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

#include "lru/pscache.h"
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

namespace capi
{

#include <mysql.h>

  class ConnectProtocol : public Protocol
  {
    static const SQLString SESSION_QUERY; /*("SELECT @@max_allowed_packet,"
    +"@@system_time_zone,"
    +"@@time_zone,"
    +"@@auto_increment_increment")*/
    static const SQLString IS_MASTER_QUERY; /*"SELECT @@innodb_read_only"*/
    static Logger* logger;

  protected:
    std::unique_ptr<MYSQL, decltype(&mysql_close)> connection;
    std::mutex lock;
    std::shared_ptr<UrlParser> urlParser;
    Shared::Options options;
    Shared::ExceptionFactory exceptionFactory;
    virtual ~ConnectProtocol() {}

  private:
    const SQLString username;
    std::unique_ptr<GlobalStateInfo> globalInfo;

  public:
    bool hasWarningsFlag= false;
    /* This cannot be Shared as long as C/C stmt handle is owned by  statement(SSPS class in this case) object */

    Results* activeStreamingResult= nullptr;
    uint32_t serverStatus= 0;

  protected:
    int32_t autoIncrementIncrement;

    bool readOnly= false;
    FailoverProxy* proxy= nullptr;
    volatile bool connected= false;
    bool explicitClosed= false;
    SQLString database;
    int64_t serverThreadId= 0;
    std::unique_ptr<Cache> serverPrepareStatementCache;
    bool eofDeprecated= false;
    int64_t serverCapabilities= 0;
    int32_t socketTimeout= 0;

  private:
    HostAddress currentHost;
    bool hostFailed= false;
    SQLString serverVersion;
    bool serverMariaDb= true;
    uint32_t majorVersion= 0;
    uint32_t minorVersion= 0;
    uint32_t patchVersion= 0;
    TimeZone* timeZone= nullptr;

  public:
    ConnectProtocol(std::shared_ptr<UrlParser>& urlParser, GlobalStateInfo* globalInfo);

  private:
    void closeSocket();
    static MYSQL* createSocket(const SQLString& host, int32_t port, const Shared::Options& options);
    static int64_t initializeClientCapabilities(const Shared::Options& options, int64_t serverCapabilities, const SQLString& database);
    static void enabledTlsProtocolSuites(MYSQL* socket, const Shared::Options& options);
    static void enabledTlsCipherSuites(MYSQL* sslSocket, const Shared::Options& options);

  protected:
    void realQuery(const SQLString& sql);
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
    uint32_t getServerStatus();
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
      const Shared::Options& options,
      int64_t& clientCapabilities,
      const int8_t exchangeCharset);

    void authenticationHandler(
      int8_t exchangeCharset,
      int64_t clientCapabilities, const SQLString& authenticationPluginType,
      sql::bytes& seed,
      const Shared::Options& options, const SQLString& database,
      Credential* credential, const SQLString& host);


    void compressionHandler(const Shared::Options& options);
    void setConnectionAttributes(const SQLString& attributes);
    void assignStream(const Shared::Options& options);
    void postConnectionQueries();
    void sendPipelineAdditionalData();
    void sendSessionInfos();
    void sendRequestSessionVariables();
    void readRequestSessionVariables(std::map<SQLString, SQLString>& serverData);
    void sendCreateDatabaseIfNotExist(const SQLString& quotedDb);
    void sendUseDatabaseIfNotExist(const SQLString& quotedDb);
    void readPipelineAdditionalData(std::map<SQLString, SQLString>& serverData);
    void requestSessionDataWithShow(std::map<SQLString, SQLString>& serverData);
    void additionalData(std::map<SQLString, SQLString>& serverData);

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
    const UrlParser& getUrlParser() const;
    bool isMasterConnection();

  private:
    void sendPipelineCheckMaster();

  public:
    void readPipelineCheckMaster();
    bool mustBeMasterConnection();
    bool noBackslashEscapes();
    void connectWithoutProxy();
    bool shouldReconnectWithoutProxy();
    const SQLString& getServerVersion() const;
    bool getReadonly() const;
    void setReadonly(bool readOnly);
    const HostAddress& getHostAddress() const;
    void setHostAddress(const HostAddress& host);
    const SQLString& getHost() const;
    FailoverProxy* getProxy();
    void setProxy(FailoverProxy* proxy);
    int32_t getPort() const;
    const SQLString& getDatabase() const;
    const SQLString& getUsername() const;

  private:
    void parseVersion(const SQLString& serverVersion);

  public:
    uint32_t getMajorServerVersion();
    uint32_t getMinorServerVersion();
    uint32_t getPatchServerVersion();
    bool versionGreaterOrEqual(uint32_t major, uint32_t minor, uint32_t patch) const;

    bool getPinGlobalTxToPhysicalConnection() const;
    bool hasWarnings();
    bool isConnected();
    int64_t getServerThreadId();
    //Socket* getSocket();
    bool isExplicitClosed();
    TimeZone* getTimeZone();
    const Shared::Options& getOptions() const;
    void setHasWarnings(bool hasWarnings);
    Results* getActiveStreamingResult() override;
    void setActiveStreamingResult(Results* activeStreamingResult) override;
    void removeActiveStreamingResult();
    std::mutex *const getLock();
    bool hasMoreResults();
    Cache* prepareStatementCache();
    void changeSocketTcpNoDelay(bool setTcpNoDelay);
    void changeSocketSoTimeout(int32_t setSoTimeout);
    bool isServerMariaDb();
    /*PacketInputStream* getReader();
    PacketOutputStream* getWriter();*/
    bool isEofDeprecated();
    bool sessionStateAware();
    SQLString getTraces();
    void reconnect();
  };
} // capi
} // mariadb
} // sql
#endif
