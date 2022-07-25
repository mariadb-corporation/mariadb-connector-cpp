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


#include <random>
#include <chrono>

#include "ConnectProtocol.h"

#include "logger/LoggerFactory.h"
#include "protocol/MasterProtocol.h"
#include "Results.h"
#include "ExceptionFactory.h"
#include "util/Utils.h"
#include "util/LogQueryTool.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
  static const char OptionSelected= 1, OptionNotSelected= 0;
  static const unsigned int uintOptionSelected= 1, uintOptionNotSelected= 0;

  const SQLString ConnectProtocol::SESSION_QUERY("SELECT @@max_allowed_packet,"
    "@@system_time_zone,"
    "@@time_zone,"
    "@@auto_increment_increment");
  const SQLString ConnectProtocol::IS_MASTER_QUERY("select @@innodb_read_only");
  Shared::Logger ConnectProtocol::logger(LoggerFactory::getLogger(typeid(ConnectProtocol)));
  static const SQLString MARIADB_RPL_HACK_PREFIX("5.5.5-");

  /**
   * Get a protocol instance.
   *
   * @param urlParser connection URL information
   * @param globalInfo server global variables information
   * @param lock the lock for thread synchronisation
   */
  ConnectProtocol::ConnectProtocol(std::shared_ptr<UrlParser>& _urlParser, GlobalStateInfo* _globalInfo, Shared::mutex& lock)
    : lock(lock)
    , urlParser(_urlParser)
    , options(_urlParser->getOptions())
    , database(_urlParser->getDatabase())
    , username(_urlParser->getUsername())
    , globalInfo(_globalInfo)
    , connection(nullptr, &mysql_close)
    , currentHost(localhost, 3306)
    , explicitClosed(false)
    , majorVersion(0)
    , minorVersion(0)
    , patchVersion(0)
    , proxy(nullptr)
    , connected(false)
    , serverPrepareStatementCache(nullptr)
    , autoIncrementIncrement(_globalInfo ? _globalInfo->getAutoIncrementIncrement() : 1)
    , eofDeprecated(false)
    , hasWarningsFlag(false)
    , hostFailed(false)
    , readOnly(false)
    , serverCapabilities(0)
    , serverMariaDb(true)
    , serverStatus(0)
    , serverThreadId(0)
    , socketTimeout(0)
    , timeZone(nullptr)
  {
    urlParser->auroraPipelineQuirks();
    if (options->cachePrepStmts && options->useServerPrepStmts){
      //ServerPrepareStatementCache::newInstance(options->prepStmtCacheSize, this);
    }
  }


  void ConnectProtocol::closeSocket()
  {
    try {
      connection.reset();
    }catch (std::exception& ){
    }
  }

  MYSQL* ConnectProtocol::createSocket(const SQLString& host, int32_t port, const Shared::Options& options)
  {
    //TODO: Shouldn't be Socket be an interface, and wrap MYSQL handle in case of C API use?

    MYSQL* socket= mysql_init(NULL);
    unsigned int inSeconds;

    // Is there similar option in the C API?
    //socket->setTcpNoDelay(options->tcpNoDelay);

    if (options->connectTimeout) {
      inSeconds= (options->connectTimeout + 999) / 1000;
      mysql_optionsv(socket, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&inSeconds);
    }
    if (options->socketTimeout){
      inSeconds= (options->socketTimeout + 999) / 1000;
      mysql_optionsv(socket, MYSQL_OPT_READ_TIMEOUT, (const char *)&inSeconds);
    }
    if (options->autoReconnect){
      mysql_optionsv(socket, MYSQL_OPT_RECONNECT, &OptionSelected);
    }
    if (options->tcpRcvBuf > 0){
      mysql_optionsv(socket, MYSQL_OPT_NET_BUFFER_LENGTH, &options->tcpRcvBuf);
    }
    if (options->tcpSndBuf > 0 && options->tcpSndBuf > options->tcpRcvBuf){
      mysql_optionsv(socket, MYSQL_OPT_NET_BUFFER_LENGTH, &options->tcpSndBuf);
    }
    if (options->tcpAbortiveClose){
      //socket->setSoLinger(true,0);
    }

    // Bind the socket to a particular interface if the connection property
    // localSocketAddress has been defined.
    if (!options->localSocket.empty()) {
      mysql_optionsv(socket, MARIADB_OPT_UNIXSOCKET, (void *)options->localSocket.c_str());
      int protocol= MYSQL_PROTOCOL_SOCKET;
      mysql_optionsv(socket, MYSQL_OPT_PROTOCOL, (void*)&protocol);
    }
    else if (!options->pipe.empty()) {
      mysql_optionsv(socket, MYSQL_OPT_NAMED_PIPE, (void *)options->pipe.c_str());
      int protocol= MYSQL_PROTOCOL_PIPE;
      mysql_optionsv(socket, MYSQL_OPT_PROTOCOL, (void*)&protocol);
    }
    else {
      mysql_optionsv(socket, MARIADB_OPT_HOST, (void *)host.c_str());
      mysql_optionsv(socket, MARIADB_OPT_PORT, (void *)&port);
      int protocol= MYSQL_PROTOCOL_TCP;
      mysql_optionsv(socket, MYSQL_OPT_PROTOCOL, (void*)&protocol);
    }

    if (!options->useCharacterEncoding.empty()) {
      mysql_optionsv(socket, MYSQL_SET_CHARSET_NAME, options->useCharacterEncoding.c_str());
    }

    return socket;
  }

  int64_t ConnectProtocol::initializeClientCapabilities(
      const Shared::Options& options, int64_t serverCapabilities, const SQLString& database)
  {
    int64_t capabilities =
      MariaDbServerCapabilities::IGNORE_SPACE
      | MariaDbServerCapabilities::CLIENT_PROTOCOL_41_
      | MariaDbServerCapabilities::TRANSACTIONS
      | MariaDbServerCapabilities::SECURE_CONNECTION
      | MariaDbServerCapabilities::MULTI_RESULTS
      | MariaDbServerCapabilities::PS_MULTI_RESULTS
      | MariaDbServerCapabilities::PLUGIN_AUTH
      | MariaDbServerCapabilities::CONNECT_ATTRS
      | MariaDbServerCapabilities::PLUGIN_AUTH_LENENC_CLIENT_DATA
      | MariaDbServerCapabilities::CLIENT_SESSION_TRACK;

    if (options->allowLocalInfile){
      capabilities|= MariaDbServerCapabilities::LOCAL_FILES;
    }

    if (!options->useAffectedRows){
      capabilities|= MariaDbServerCapabilities::FOUND_ROWS;
    }

    if (options->allowMultiQueries || (options->rewriteBatchedStatements)){
      capabilities|= MariaDbServerCapabilities::MULTI_STATEMENTS;
    }

    if ((serverCapabilities & MariaDbServerCapabilities::CLIENT_DEPRECATE_EOF)!=0){
      capabilities|= MariaDbServerCapabilities::CLIENT_DEPRECATE_EOF;
    }

    if (options->useCompression){
      if ((serverCapabilities &MariaDbServerCapabilities::COMPRESS)==0){

        options->useCompression= false;
      }else {
        capabilities|= MariaDbServerCapabilities::COMPRESS;
      }
    }

    if (options->interactiveClient){
      capabilities|= MariaDbServerCapabilities::CLIENT_INTERACTIVE_;
    }


    if (!database.empty() && !options->createDatabaseIfNotExist){
      capabilities|= MariaDbServerCapabilities::CONNECT_WITH_DB;
    }

    return capabilities;
  }

  /**
   * Return possible protocols : values of option enabledTlsProtocolSuites is set, or default to
   * "TLSv1,TLSv1.1". MariaDB versions &ge; 10.0.15 and &ge; 5.5.41 supports TLSv1.2 if compiled
   * with openSSL (default). MySQL community versions &ge; 5.7.10 is compile with yaSSL, so max TLS
   * is TLSv1.1.
   *
   * @param sslSocket current sslSocket
   * @throws SQLException if protocol isn't a supported protocol
   */
  void ConnectProtocol::enabledTlsProtocolSuites(MYSQL* socket, const Shared::Options& options)
  {
    static SQLString possibleProtocols= "TLSv1.1, TLSv1.2, TLSv1.3";

    if (!options->enabledTlsProtocolSuites.empty()) {
      Tokens protocols= split(options->enabledTlsProtocolSuites, "[,;\\s]+");
      for (const auto& protocol : *protocols){
        if (StringImp::get(possibleProtocols).find(protocol) == std::string::npos){
          throw SQLException(
              "Unsupported TLS protocol '"
              +protocol
              +"'. Supported protocols : " + possibleProtocols);
        }
      }
      mysql_optionsv(socket, MARIADB_OPT_TLS_VERSION, (void*)options->enabledTlsProtocolSuites.c_str());
    }
  }

  /**
   * Set ssl socket cipher according to options->
   *
   * @param sslSocket current ssl socket
   * @throws SQLException if a cipher isn't known
   */
  void ConnectProtocol::enabledTlsCipherSuites(MYSQL* sslSocket, const Shared::Options& options)
  {
    if (!options->enabledTlsCipherSuites.empty()){
#ifdef POSSIBLE_CIPHERS_DEFINED
      SQLString possibleCiphers;
      Tokens ciphers(split(options->enabledTlsCipherSuites, "[,;\\s]+"));
      for (const auto& cipher : *ciphers){
        if (!(possibleCiphers.find(cipher) != std::string::npos)){
          throw SQLException(
              "Unsupported SSL cipher '"
              +cipher
              +"'. Supported ciphers : "
              );
        }
      }
#endif
      mysql_optionsv(sslSocket, MYSQL_OPT_SSL_CIPHER, options->enabledTlsCipherSuites.c_str());
    }
  }

  /** Closes socket and stream readers/writers Attempts graceful shutdown. */
  void ConnectProtocol::close()
  {
    std::unique_lock<std::mutex> localScopeLock(*lock);
    this->connected= false;
    try {
      // skip acquires lock
      localScopeLock.unlock();
      skip();
    }catch (std::runtime_error& ){
    }
    localScopeLock.lock();
    closeSocket();
    cleanMemory();
  }

  /** Force closes socket and stream readers/writers. */
  void ConnectProtocol::abort()
  {
    this->explicitClosed= true;
    bool lockStatus= false;

    if (lock){
      lockStatus= lock->try_lock();
    }
    this->connected= false;

    abortActiveStream();

    if (!lockStatus){

      forceAbort();
      try {
        //->setSoTimeout(10);
        //->setSoLinger(true,0);
      }catch (std::exception& ){

      }
    }else {
    }

    closeSocket();
    cleanMemory();

    if (lockStatus){
      lock->unlock();
    }
  }

  void ConnectProtocol::forceAbort()
  {
    try {
      Shared::mutex forCopied(new std::mutex());
      std::unique_ptr<MasterProtocol> copiedProtocol(new MasterProtocol(urlParser, new GlobalStateInfo(), forCopied));
      copiedProtocol->setHostAddress(getHostAddress());
      copiedProtocol->connect();

      copiedProtocol->executeQuery("KILL " + std::to_string(serverThreadId));
    }catch (SQLException& ){

    }
  }

  void ConnectProtocol::abortActiveStream()
  {
    try {
      Shared::Results activeStream= activeStreamingResult.lock();
      if (activeStream){
        activeStream->abort();
        activeStreamingResult.reset();
      }
    }catch (std::runtime_error& ){

    }
  }

  /**
   * Skip packets not read that are not needed. Packets are read according to needs. If some data
   * have not been read before next execution, skip it. <i>Lock must be set before using this
   * method</i>
   *
   * @throws SQLException exception
   */
  void ConnectProtocol::skip()
  {
    Shared::Results activeStream = activeStreamingResult.lock();
    if (activeStream) {
      activeStream->loadFully(true, this);
      activeStreamingResult.reset();
    }
  }

  void ConnectProtocol::cleanMemory()
  {
    if (options->cachePrepStmts && options->useServerPrepStmts && serverPrepareStatementCache){
      //serverPrepareStatementCache.clear();
    }
    if (options->enablePacketDebug){
      //traceCache->clearMemory();
    }
  }

  void ConnectProtocol::setServerStatus(uint32_t serverStatus)
  {
    this->serverStatus= serverStatus;
  }

  /** Remove flag has more results. */
  void ConnectProtocol::removeHasMoreResults()
  {
    if (hasMoreResults()){
      this->serverStatus= static_cast<int16_t>((serverStatus) ^ServerStatus::MORE_RESULTS_EXISTS);
    }
  }

  /**
   * Connect to currentHost.
   *
   * @throws SQLException exception
   */
  void ConnectProtocol::connect()
  {
    if (!isClosed()){
      close();
    }

    try {
      createConnection(&currentHost, username);
    }catch (SQLException& exception){
      ExceptionFactory::INSTANCE.create(
          "Could not connect to "+currentHost.toString() +". "+exception.getMessage() + getTraces(), "08000", &exception).Throw();
    }
  }


  void ConnectProtocol::createConnection(HostAddress* hostAddress, const SQLString& username)
  {

    SQLString host(hostAddress != nullptr ? hostAddress->host : "");
    int32_t port= hostAddress != nullptr ? hostAddress->port :3306;

    Unique::Credential credential;
    std::shared_ptr<CredentialPlugin> credentialPlugin(urlParser->getCredentialPlugin());
    if (credentialPlugin){
      credential.reset(credentialPlugin->initialize(options, username, *hostAddress)->get());
    }else {
      credential.reset(new Credential(username, urlParser->getPassword()));
    }

    connection.reset(createSocket(host, port, options));

    assignStream(options);

    try {

      int8_t  exchangeCharset= decideLanguage(/*greetingPacket.getServerLanguage()*/224 & 0xFF);
      int64_t clientCapabilities= initializeClientCapabilities(options, serverCapabilities, database);
      exceptionFactory.reset(ExceptionFactory::of(serverThreadId, options));

      sslWrapper(
          host,
          options,
          clientCapabilities,
          exchangeCharset);

      SQLString authenticationPluginType;

      if (credentialPlugin && !credentialPlugin->defaultAuthenticationPluginType().empty()){
        authenticationPluginType= credentialPlugin->defaultAuthenticationPluginType();
      }
      sql::bytes dummy;

      authenticationHandler(
          exchangeCharset,
          clientCapabilities,
          authenticationPluginType,
          dummy,
          options,
          database,
          credential.get(),
          host);

      compressionHandler(options);

    }catch (SQLException& sqlException){
      destroySocket();
      throw sqlException;

    }catch (std::exception& ioException){
      destroySocket();
      if (!host.empty()){
        ExceptionFactory::INSTANCE.create(
            "Could not connect to socket : " + SQLString(ioException.what()), "08000", &ioException).Throw();
      }
      ExceptionFactory::INSTANCE.create(
          "Could not connect to "
          +host
          +":"
          + std::to_string(port)
          +" : "
          + SQLString(ioException.what()),
          "08000",
          &ioException).Throw();
    }
    mysql_optionsv(connection.get(), MYSQL_REPORT_DATA_TRUNCATION, &uintOptionSelected);
    mysql_optionsv(connection.get(), MYSQL_OPT_LOCAL_INFILE, (options->allowLocalInfile ? &uintOptionSelected : &uintOptionNotSelected));

    if (mysql_real_connect(connection.get(), NULL, NULL, NULL, NULL, 0, NULL, CLIENT_MULTI_STATEMENTS) == nullptr)
    {
      throw SQLException(mysql_error(connection.get()), mysql_sqlstate(connection.get()), mysql_errno(connection.get()));
    }

    connected= true;

    this->serverThreadId= mysql_thread_id(connection.get());

    this->serverVersion= mysql_get_server_info(connection.get());// mysql_get_server_version(connection);
    parseVersion(serverVersion);

    if (serverVersion.startsWith(MARIADB_RPL_HACK_PREFIX)) {
      serverMariaDb = true;
      serverVersion = serverVersion.substr(MARIADB_RPL_HACK_PREFIX.length());
    }
    else {
      serverMariaDb = StringImp::get(serverVersion).find("MariaDB") != std::string::npos;
    }
    unsigned long baseCaps, extCaps;
    mariadb_get_infov(connection.get(), MARIADB_CONNECTION_EXTENDED_SERVER_CAPABILITIES, (void*)&extCaps);
    mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_CAPABILITIES, (void*)&baseCaps);
    int64_t serverCaps= extCaps;
    serverCaps= serverCaps << 32;
    serverCaps|= baseCaps;
    this->serverCapabilities= serverCaps;


    if (this->options->socketTimeout > 0){
      this->socketTimeout= this->options->socketTimeout;
      setTimeout(socketTimeout);
    }
    if ((serverCapabilities & MariaDbServerCapabilities::CLIENT_DEPRECATE_EOF)!=0){
      eofDeprecated= true;
    }

    postConnectionQueries();

    activeStreamingResult.reset();
    hostFailed= false;
  }

  /** Closing socket in case of Connection error after socket creation. */
  void ConnectProtocol::destroySocket()
  {
    if (connection){
      try {
        connection.reset();
      }catch (std::exception&){

      }
    }
  }

  void ConnectProtocol::sslWrapper(
      const SQLString& host,
      const Shared::Options& options,
      int64_t& clientCapabilities,
      int8_t exchangeCharset)
  {
    const unsigned int safeCApiTrue= 0x01010101;

    if (options->useTls)
    {
      clientCapabilities|=  MariaDbServerCapabilities::SSL;
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_ENFORCE, (const char*)&safeCApiTrue);
    }

    this->enabledTlsProtocolSuites(connection.get(), options);
    this->enabledTlsCipherSuites(connection.get(), options);

    if (!options->tlsKey.empty()) {
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_KEY, options->tlsKey.c_str());
      if (!options->keyPassword.empty()) {
        mysql_optionsv(connection.get(), MARIADB_OPT_TLS_PASSPHRASE, options->keyPassword.c_str());
      }
    }

    if (!options->tlsCert.empty()) {
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_CERT, options->tlsCert.c_str());
    }
    if (!options->tlsCA.empty()) {
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_CA, options->tlsCA.c_str());
    }
    if (!options->tlsCAPath.empty()) {
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_CAPATH, options->tlsCAPath.c_str());
    }
    if (!options->tlsCRL.empty()) {
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_CRL, options->tlsCRL.c_str());
    }
    if (!options->tlsCRLPath.empty()) {
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_CRL, options->tlsCRLPath.c_str());
    }
    if (!options->tlsPeerFP.empty()) {
      mysql_optionsv(connection.get(), MARIADB_OPT_TLS_PEER_FP, options->tlsPeerFP.c_str());
    }


    // This is not quite a TLS option, but still putting it here
    if (!options->serverRsaPublicKeyFile.empty()) {
      mysql_optionsv(connection.get(), MYSQL_SERVER_PUBLIC_KEY, (void*)options->serverRsaPublicKeyFile.c_str());
    }
    //sslSocket->setUseClientMode(true);
    //sslSocket->startHandshake();

    if (!options->disableSslHostnameVerification && !options->trustServerCertificate) {
      mysql_optionsv(connection.get(), MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (const char*)&safeCApiTrue);
    }

    assignStream(options);
  }

  void ConnectProtocol::authenticationHandler(
    int8_t exchangeCharset,
    int64_t clientCapabilities, const SQLString& authenticationPluginType,
    sql::bytes& seed,
    const Shared::Options& options, const SQLString& database,
    Credential* credential, const SQLString& host)
  {
    mysql_optionsv(connection.get(), MARIADB_OPT_USER, (void*)credential->getUser().c_str());
    mysql_optionsv(connection.get(), MARIADB_OPT_PASSWORD, (void*)credential->getPassword().c_str());
    mysql_optionsv(connection.get(), MARIADB_OPT_SCHEMA, (void*)database.c_str());
    if (!options->credentialType.empty()) {
      mysql_optionsv(connection.get(), MYSQL_DEFAULT_AUTH, (void*)options->credentialType.c_str());
    }
  }

  void ConnectProtocol::compressionHandler(const Shared::Options& options)
  {
    if (options->useCompression){
      mysql_optionsv(connection.get(), MYSQL_OPT_COMPRESS, NULL);
    }
  }

  void ConnectProtocol::assignStream(const Shared::Options& options)
  {
    try {

      if (options->enablePacketDebug){
        /*writer->setTraceCache(traceCache);
        reader->setTraceCache(traceCache);*/
      }

    }catch (std::exception& ioe){
      destroySocket();
      ExceptionFactory::INSTANCE.create(SQLString("Socket error: ") + ioe.what(), "08000", &ioe).Throw();
    }
  }

  void ConnectProtocol::postConnectionQueries()
  {
    try {

      bool mustLoadAdditionalInfo= true;

      if (globalInfo){
        if (globalInfo->isAutocommit() == options->autocommit){
          mustLoadAdditionalInfo= false;
        }
      }

      if (mustLoadAdditionalInfo){
        std::map<SQLString,SQLString> serverData;
        if (options->usePipelineAuth && !options->createDatabaseIfNotExist){
          try {
            sendPipelineAdditionalData();
            readPipelineAdditionalData(serverData);
          }catch (SQLException& sqle){
            if (sqle.getSQLState().startsWith("08")){
              throw sqle;
            }
            // in case pipeline is not supported
            // (proxy flush socket after reading first packet)
            additionalData(serverData);
          }
        }else {
          additionalData(serverData);
        }

        std::size_t maxAllowedPacket= static_cast<std::size_t>(std::stoi(StringImp::get(serverData["max_allowed_packet"])));
        mysql_optionsv(connection.get(), MYSQL_OPT_MAX_ALLOWED_PACKET, &maxAllowedPacket);
        autoIncrementIncrement= std::stoi(StringImp::get(serverData["auto_increment_increment"]));
        loadCalendar(serverData["time_zone"],serverData["system_time_zone"]);

      }else {
        size_t maxAllowedPacket= static_cast<size_t>(globalInfo->getMaxAllowedPacket());
        mysql_optionsv(connection.get(), MYSQL_OPT_MAX_ALLOWED_PACKET, &maxAllowedPacket);
        autoIncrementIncrement= globalInfo->getAutoIncrementIncrement();
        loadCalendar(globalInfo->getTimeZone(), globalInfo->getSystemTimeZone());
      }

      activeStreamingResult.reset();
      hostFailed= false;
    }catch (SQLException& sqlException){
      destroySocket();
      throw sqlException;
    }catch (std::runtime_error& ioException){
      destroySocket();
      exceptionFactory->create(
          SQLString("Socket error during post connection queries: ") + ioException.what(), "08000", &ioException).Throw();
    }
  }

  /**
   * Send all additional needed values. Command are send one after the other, assuming that command
   * are less than 65k (minimum hosts TCP/IP buffer size)
   *
   * @throws IOException if socket exception occur
   */
  void ConnectProtocol::sendPipelineAdditionalData()
  {
    sendSessionInfos();
    sendRequestSessionVariables();

    sendPipelineCheckMaster();
  }

  void ConnectProtocol::sendSessionInfos()
  {

    SQLString sessionOption("autocommit=");
    sessionOption.append(options->autocommit ? "1" : "0");

    if ((serverCapabilities & MariaDbServerCapabilities::CLIENT_SESSION_TRACK)!=0){
      sessionOption.append(", session_track_schema=1");
      if (options->rewriteBatchedStatements){
        sessionOption.append(", session_track_system_variables= 'auto_increment_increment' ");
      }
    }

    if (options->jdbcCompliantTruncation){
      sessionOption.append(", sql_mode = concat(@@sql_mode,',STRICT_TRANS_TABLES')");
    }

    if (!options->sessionVariables.empty()){
      sessionOption.append(",").append(Utils::parseSessionVariables(options->sessionVariables));
    }

    SQLString query("set "+sessionOption);
    realQuery(query);
  }

  void ConnectProtocol::sendRequestSessionVariables()
  {
    realQuery(SESSION_QUERY);
  }

  void ConnectProtocol::readRequestSessionVariables(std::map<SQLString, SQLString>& serverData)
  {
    Unique::Results results(new Results());
    getResult(results.get());

    results->commandEnd();
    ResultSet* resultSet= results->getResultSet();
    if (resultSet){
      resultSet->next();

      serverData.emplace("max_allowed_packet",resultSet->getString(1));
      serverData.emplace("system_time_zone",resultSet->getString(2));
      serverData.emplace("time_zone",resultSet->getString(3));
      serverData.emplace("auto_increment_increment", resultSet->getString(4));

    }else {
      throw SQLException(mysql_get_socket(connection.get()) == MARIADB_INVALID_SOCKET ?
        "Error reading SessionVariables results. Socket is NOT connected" :
        "Error reading SessionVariables results. Socket IS connected");
    }
  }

  void ConnectProtocol::sendCreateDatabaseIfNotExist(const SQLString& quotedDb)
  {
    SQLString query("CREATE DATABASE IF NOT EXISTS "+ quotedDb);
    mysql_real_query(connection.get(), query.c_str(), static_cast<unsigned long>(query.length()));
  }

  void ConnectProtocol::sendUseDatabaseIfNotExist(const SQLString& quotedDb)
  {
    SQLString query("USE "+quotedDb);
    mysql_real_query(connection.get(), query.c_str(), static_cast<unsigned long>(query.length()));
  }

  void ConnectProtocol::readPipelineAdditionalData(std::map<SQLString, SQLString>& serverData)
  {

    MariaDBExceptionThrower resultingException;

    try {
      Unique::Results res(new Results());
      getResult(res.get());
    }catch (SQLException& sqlException){

      resultingException.take(sqlException);
    }

    bool canTrySessionWithShow= false;

    try {
      readRequestSessionVariables(serverData);
    }catch (SQLException& sqlException){
      if (!resultingException){
        resultingException.assign(exceptionFactory->create("could not load system variables", "08000", &sqlException));
        canTrySessionWithShow= true;
      }
    }

    try {
      readPipelineCheckMaster();
    }catch (SQLException& sqlException){
      canTrySessionWithShow= false;
      if (!resultingException){
        exceptionFactory->create(
            "could not identified if server is master", "08000", &sqlException).Throw();
      }
    }

    if (canTrySessionWithShow){

      requestSessionDataWithShow(serverData);
      connected= true;
      return;
    }

    if (resultingException){
      resultingException.Throw();
    }
    connected= true;
  }


  void ConnectProtocol::requestSessionDataWithShow(std::map<SQLString, SQLString>& serverData)
  {
    try {
      Shared::Results results(new Results());
      executeQuery(
          true,
          results,
          "SHOW VARIABLES WHERE Variable_name in ("
          "'max_allowed_packet',"
          "'system_time_zone',"
          "'time_zone',"
          "'auto_increment_increment')");
      results->commandEnd();
      ResultSet* resultSet= results->getResultSet();
      if (resultSet){
        while (resultSet->next()){
          if (logger->isDebugEnabled()){
            logger->debug("server data " + resultSet->getString(1) + " = " + resultSet->getString(2));
          }
          serverData.emplace(resultSet->getString(1),resultSet->getString(2));
        }
        if (serverData.size()<4){
          exceptionFactory->create(mysql_get_socket(connection.get()) == MARIADB_INVALID_SOCKET ?
              "could not load system variables. socket connected: No" : "could not load system variables. socket connected: Yes", "08000").Throw();
        }
      }

    }catch (SQLException& sqlException){
      exceptionFactory->create("could not load system variables", &sqlException).Throw();
    }
  }


  void ConnectProtocol::additionalData(std::map<SQLString, SQLString>& serverData)
  {
    Unique::Results res(new Results());
    sendSessionInfos();
    getResult(res.get());

    try {
      sendRequestSessionVariables();
      readRequestSessionVariables(serverData);
    }catch (SQLException& ){
      requestSessionDataWithShow(serverData);
    }

    sendPipelineCheckMaster();
    readPipelineCheckMaster();

    if (options->createDatabaseIfNotExist && !database.empty()){

      SQLString quotedDb(MariaDbConnection::quoteIdentifier(this->database));
      sendCreateDatabaseIfNotExist(quotedDb);

      Unique::Results res(new Results());
      getResult(res.get());

      sendUseDatabaseIfNotExist(quotedDb);
      res.reset(new Results());
      getResult(res.get());
    }
  }

  /**
   * Is the connection closed.
   *
   * @return true if the connection is closed
   */
  bool ConnectProtocol::isClosed()
  {
    return !this->connected;
  }

  void ConnectProtocol::loadCalendar(const SQLString& srvTimeZone, const SQLString& srvSystemTimeZone)
  {

    timeZone= nullptr;// Calendar.getInstance().getTimeZone();

#ifdef WE_HAVE_NEED_TIMEZONE
    else {

      SQLString tz= options->serverTimezone;
      if (!tz){
        tz= srvTimeZone;
        if (){
          tz= srvSystemTimeZone;
        }
      }

      if (tz/*.empty() == true*/
          &&tz.size()>=2
          &&(tz.startsWith("+")||tz.startsWith("-"))
          &&Character.isDigit(tz.at(1))){
        tz= "GMT"+tz;
      }

      try {
        timeZone= Utils::getTimeZone(tz);
      }catch (SQLException& e){
        if (options->serverTimezone/*.empty() == true*/){
          throw SQLException(
              "The server time_zone '"
              +tz
              +"' defined in the 'serverTimezone' parameter cannot be parsed "
              "by java TimeZone implementation. See java.util.TimeZone#getAvailableIDs() for available TimeZone, depending on your "
              "JRE implementation.",
              "01S00");
        }else {
          throw SQLException(
              "The server time_zone '"
              +tz
              +"' cannot be parsed. The server time zone must defined in the "
              "jdbc url string with the 'serverTimezone' parameter (or server time zone must be defined explicitly with "
              "sessionVariables=time_zone='Canada/Atlantic' for example).  See "
              "java.util.TimeZone#getAvailableIDs() for available TimeZone, depending on your JRE implementation.",
              "01S00");
        }
      }
    }
#endif
  }

  /**
   * Check that current connection is a master connection (not read-only).
   *
   * @return true if master
   * @throws SQLException if requesting infos for server fail.
   */
  bool ConnectProtocol::checkIfMaster()
  {
    return isMasterConnection();
  }

  /**
   * Default collation used for string exchanges with server.
   *
   * @param serverLanguage server default collation
   * @return collation byte
   */
  int8_t ConnectProtocol::decideLanguage(int32_t serverLanguage)
  {

    if (serverLanguage == 45
        ||serverLanguage == 46
        ||(serverLanguage >=224 &&serverLanguage <=247)){
      return (int8_t)serverLanguage;
    }
    if (getMajorServerVersion() == 5 && getMinorServerVersion() <= 1){

      return (int8_t)33;
    }
    return (int8_t)224;
  }

  /**
   * Check that next read packet is a End-of-file packet.
   *
   * @throws SQLException if not a End-of-file packet
   * @throws IOException if connection error occur
   */
  void ConnectProtocol::readEofPacket()
  {
    if (mysql_errno(connection.get()) == 0)
    {
      this->hasWarningsFlag= (mysql_warning_count(connection.get()) > 0);
      mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_STATUS, (void*)&this->serverStatus);
    }
    else
    {
      exceptionFactory->create(SQLString("Could not connect: ") + mysql_error(connection.get()),
        mysql_sqlstate(connection.get()), mysql_errno(connection.get()), true).Throw();
    }
  }


  uint32_t ConnectProtocol::getServerStatus()
  {
    mariadb_get_infov(connection.get(), MARIADB_CONNECTION_SERVER_STATUS, (void*)&this->serverStatus);
    return serverStatus;
  }

  /**
   * Check that next read packet is a End-of-file packet.
   *
   * @throws SQLException if not a End-of-file packet
   * @throws IOException if connection error occur
   */
  void ConnectProtocol::skipEofPacket()
  {
  }

  void ConnectProtocol::setHostFailedWithoutProxy()
  {
    hostFailed= true;
    close();
  }

  const UrlParser& ConnectProtocol::getUrlParser() const
  {
    return *urlParser;
  }

  /**
   * Indicate if current protocol is a master protocol.
   *
   * @return is master flag
   */
  bool ConnectProtocol::isMasterConnection()
  {
    return currentHost.host.empty() || (ParameterConstant::TYPE_MASTER.compare(currentHost.type) == 0);
  }

  /**
   * Send query to identify if server is master.
   *
   * @throws IOException in case of socket error.
   */
  void ConnectProtocol::sendPipelineCheckMaster()
  {
    if (urlParser->getHaMode() == HaMode::AURORA) {
      mysql_real_query(connection.get(), IS_MASTER_QUERY.c_str(), static_cast<unsigned long>(IS_MASTER_QUERY.length()));
    }
  }

  void ConnectProtocol::readPipelineCheckMaster()
  {

  }

  bool ConnectProtocol::mustBeMasterConnection()
  {
    return true;
  }

  bool ConnectProtocol::noBackslashEscapes()
  {
    return ((serverStatus & ServerStatus::NO_BACKSLASH_ESCAPES)!=0);
  }

  /**
   * Connect without proxy. (use basic failover implementation)
   *
   * @throws SQLException exception
   */
  void ConnectProtocol::connectWithoutProxy()
  {
    if (!isClosed()){
      close();
    }

    std::vector<HostAddress>& addrs= urlParser->getHostAddresses();
    std::vector<HostAddress> hosts(addrs);


    if (urlParser->getHaMode() == HaMode::LOADBALANCE) {
      static auto rnd= std::default_random_engine{};
      std::shuffle(hosts.begin(), hosts.end(), rnd);
    }

    if (hosts.empty() && !options->pipe.empty()){
      try {
        createConnection(nullptr, username);
        return;
      }catch (SQLException& exception){
        ExceptionFactory::INSTANCE.create(
            "Could not connect to named pipe '"
            +options->pipe
            +"' : "
            +exception.getMessage()
            +getTraces(),
            "08000",
            &exception).Throw();
      }
    }

    while (!hosts.empty()){
      currentHost= hosts.back();
      hosts.pop_back();
      try {
        createConnection(&currentHost, username);
        return;
      }catch (SQLException& e){
        if (hosts.empty()){
          if (!e.getSQLState().empty()){
            ExceptionFactory::INSTANCE.create(
                "Could not connect to "
                + HostAddress::toString(addrs)
                + " : "
                + e.getMessage()
                + getTraces(),
                e.getSQLState(),
                e.getErrorCode(),
                &e).Throw();
          }
          ExceptionFactory::INSTANCE.create(
              "Could not connect to " + currentHost.toString() +". "+e.getMessage()+getTraces(), "08000", &e).Throw();
        }
      }
    }
  }

  /**
   * Indicate for Old reconnection if can reconnect without throwing exception.
   *
   * @return true if can reconnect without issue
   */
  bool ConnectProtocol::shouldReconnectWithoutProxy()
  {
    return (((serverStatus & ServerStatus::IN_TRANSACTION)==0)
        && hostFailed
        && urlParser->getOptions()->autoReconnect);
  }

  const SQLString& ConnectProtocol::getServerVersion() const
  {
    return serverVersion;
  }

  bool ConnectProtocol::getReadonly() const
  {
    return readOnly;
  }

  void ConnectProtocol::setReadonly(bool readOnly)
  {
    this->readOnly= readOnly;
  }

  const HostAddress& ConnectProtocol::getHostAddress() const
  {
    return currentHost;
  }

  void ConnectProtocol::setHostAddress(const HostAddress& host)
  {
    this->currentHost= host;
    this->readOnly= (ParameterConstant::TYPE_SLAVE.compare(this->currentHost.type) == 0);
  }

  const SQLString& ConnectProtocol::getHost() const
  {
    return currentHost.host;
  }

  FailoverProxy* ConnectProtocol::getProxy()
  {
    return proxy;
  }

  void ConnectProtocol::setProxy(FailoverProxy* proxy)
  {
    this->proxy= proxy;
  }

  int32_t ConnectProtocol::getPort() const
  {
    return (currentHost.port > 0) ? currentHost.port : 3306;
  }

  const SQLString& ConnectProtocol::getDatabase() const
  {
    return database;
  }

  const SQLString& ConnectProtocol::getUsername() const
  {
    return username;
  }

  void ConnectProtocol::parseVersion(const SQLString& _serverVersion)
  {
    size_t length= _serverVersion.length();
    char car;
    size_t offset= 0;
    int32_t type= 0;
    int32_t val= 0;
    for (;offset < length; offset++){
      car= _serverVersion.at(offset);
      if (car <'0'||car >'9'){
        switch (type){
          case 0:
            majorVersion= val;
            break;
          case 1:
            minorVersion= val;
            break;
          case 2:
            patchVersion= val;
            return;
          default:
            break;
        }
        type++;
        val= 0;
      }else {
        val= val *10 +car -48;
      }
    }


    if (type == 2){
      patchVersion= val;
    }
  }


  uint32_t ConnectProtocol::getMajorServerVersion()
  {
    return majorVersion;
  }

  uint32_t ConnectProtocol::getMinorServerVersion()
  {
    return minorVersion;
  }

  uint32_t ConnectProtocol::getPatchServerVersion()
  {
    return patchVersion;
  }

  /**
   * Utility method to check if database version is greater than parameters.
   *
   * @param major major version
   * @param minor minor version
   * @param patch patch version
   * @return true if version is greater than parameters
   */
  bool ConnectProtocol::versionGreaterOrEqual(uint32_t major, uint32_t minor, uint32_t patch) const
  {
    if (this->majorVersion > major) {
      return true;
    }

    if (this->majorVersion < major) {
      return false;
    }

    /*
    * Major versions are equal, compare minor versions
    */
    if (this->minorVersion > minor) {
      return true;
    }
    if (this->minorVersion < minor) {
      return false;
    }

    // Minor versions are equal, compare patch version.
    return (this->patchVersion >= patch);
  }


  bool ConnectProtocol::getPinGlobalTxToPhysicalConnection() const
  {
    return this->options->pinGlobalTxToPhysicalConnection;
  }

  /**
   * Has warnings.
   *
   * @return true if as warnings.
   */
  bool ConnectProtocol::hasWarnings()
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);
    return hasWarningsFlag;
  }

  /**
   * Is connected.
   *
   * @return true if connected
   */
  bool ConnectProtocol::isConnected()
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);
    return connected;
  }

  int64_t ConnectProtocol::getServerThreadId()
  {
    return serverThreadId;
  }

  /*Socket* ConnectProtocol::getSocket()
  {
    return mysql_get_socket(Dbc->mariadb) == MARIADB_INVALID_SOCKET;
  }*/

  bool ConnectProtocol::isExplicitClosed()
  {
    return explicitClosed;
  }

  TimeZone* ConnectProtocol::getTimeZone()
  {
    return timeZone;
  }

  const Shared::Options& ConnectProtocol::getOptions() const
  {
    return options;
  }

  void ConnectProtocol::setHasWarnings(bool _hasWarnings)
  {
    this->hasWarningsFlag= _hasWarnings;
  }

  Shared::Results ConnectProtocol::getActiveStreamingResult()
  {
    return activeStreamingResult.lock();
  }

  void ConnectProtocol::setActiveStreamingResult(Shared::Results& _activeStreamingResult)
  {
    this->activeStreamingResult= _activeStreamingResult;
  }

  /** Remove exception result and since totally fetched, set fetch size to 0. */
  void ConnectProtocol::removeActiveStreamingResult()
  {
    Shared::Results activeStream = getActiveStreamingResult();
    if (activeStream) {
      activeStream->removeFetchSize();
      this->activeStreamingResult.reset();
    }
  }

  Shared::mutex& ConnectProtocol::getLock()
  {
    return lock;
  }

  bool ConnectProtocol::hasMoreResults()
  {
    return (serverStatus & ServerStatus::MORE_RESULTS_EXISTS) != 0;
  }

  ServerPrepareStatementCache* ConnectProtocol::prepareStatementCache()
  {
    return serverPrepareStatementCache;
  }

  /**
   * Change Socket TcpNoDelay option.
   *
   * @param setTcpNoDelay value to set.
   */
  void ConnectProtocol::changeSocketTcpNoDelay(bool setTcpNoDelay)
  {
    try {
      //TODO is there anything libmariadb for that?
      //socket->setTcpNoDelay(setTcpNoDelay);
    }catch (std::runtime_error& ){

    }
  }

  void ConnectProtocol::changeSocketSoTimeout(int32_t millis)
  {
    this->socketTimeout = millis;
    // Making seconds out of millies, as MYSQL_OPT_READ_TIMEOUT needs seconds
    millis= (millis + 999) / 1000;
    //socket->setSoTimeout(this->socketTimeout);
    mysql_optionsv(connection.get(), MYSQL_OPT_READ_TIMEOUT, (void*)&millis);
  }

  bool ConnectProtocol::isServerMariaDb()
  {
    return serverMariaDb;
  }

  /*PacketInputStream ConnectProtocol::getReader()
  {
    return reader;
  }

  PacketOutputStream ConnectProtocol::getWriter()
  {
    return writer;
  }*/

  bool ConnectProtocol::isEofDeprecated()
  {
    return eofDeprecated;
  }

  bool ConnectProtocol::sessionStateAware()
  {
    return (serverCapabilities & MariaDbServerCapabilities::CLIENT_SESSION_TRACK)!=0;
  }

  /**
   * Get a String containing readable information about last 10 send/received packets.
   *
   * @return String value
   */
  SQLString ConnectProtocol::getTraces()
  {
    if (options->enablePacketDebug){
      //return traceCache.printStack();
    }
    return "";
  }

  void ConnectProtocol::realQuery(const SQLString& sql)
  {
    if (capi::mysql_real_query(connection.get(), sql.c_str(), static_cast<unsigned long>(sql.length()))) {
      throw SQLException(capi::mysql_error(connection.get()), capi::mysql_sqlstate(connection.get()),
                        capi::mysql_errno(connection.get()));
    }
  }

  void ConnectProtocol::reconnect()
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);

    if (!options->autoReconnect)
    {
      mysql_optionsv(connection.get(), MYSQL_OPT_RECONNECT, &OptionSelected);
    }
    if (capi::mariadb_reconnect(connection.get()) != 0) {
      throw SQLException(capi::mysql_error(connection.get()), capi::mysql_sqlstate(connection.get()),
        capi::mysql_errno(connection.get()));
    }
    connected= true;
    if (!options->autoReconnect)
    {
      mysql_optionsv(connection.get(), MYSQL_OPT_RECONNECT, &OptionNotSelected);
    }
  }
}
}
}
