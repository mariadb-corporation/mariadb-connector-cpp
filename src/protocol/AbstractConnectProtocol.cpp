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


#include "AbstractConnectProtocol.h"

namespace sql
{
namespace mariadb
{
  const SQLString AbstractConnectProtocol::SESSION_QUERY("SELECT @@max_allowed_packet,"
    +"@@system_time_zone,"
    +"@@time_zone,"
    +"@@auto_increment_increment");
  const SQLString IS_MASTER_QUERY("select @@innodb_read_only");
  const Shared::Logger AbstractConnectProtocol::logger= LoggerFactory::getLogger(typeid(AbstractConnectProtocol));
  /**
   * Get a protocol instance.
   *
   * @param urlParser connection URL information
   * @param globalInfo server global variables information
   * @param lock the lock for thread synchronisation
   */
  AbstractConnectProtocol::AbstractConnectProtocol(const UrlParser& urlParser, const GlobalStateInfo& globalInfo, Shared::mutex& lock)
    : lock(lock)
      , urlParser(urlParser)
      , options(urlParser->getOptions())
      , database((urlParser->getDatabase()/*.empty() == true*/ ?"":urlParser->getDatabase()))
        , username((urlParser->getUsername()/*.empty() == true*/ ?"":urlParser->getUsername()))
        , globalInfo(globalInfo)
        {
          urlParser->auroraPipelineQuirks();
          if (options.cachePrepStmts &&options.useServerPrepStmts){
            serverPrepareStatementCache =
              ServerPrepareStatementCache.newInstance(options.prepStmtCacheSize,this);
          }
        }

  void AbstractConnectProtocol::closeSocket(PacketInputStream* packetInputStream, PacketOutputStream* packetOutputStream, Socket* socket)
  {
    try {
      try {
        int64_t maxCurrentMillis= System.currentTimeMillis()+10;
        socket->shutdownOutput();
        socket->setSoTimeout(3);
        InputStream is= socket->getInputStream();

        while (is.read()!=-1 &&System.currentTimeMillis()<maxCurrentMillis){

        }
      }catch (std::exception& t){

      }
      packetOutputStream.close();
      packetInputStream.close();
    }catch (std::exception& t){

    }/* TODO: something with the finally was once here */ {
      try {
        socket->close();
      }catch (std::exception& t){

      }
    }
  }

  Socket AbstractConnectProtocol::createSocket(const SQLString& host, int32_t port, const Shared::Options& options)
  {
    Socket socket;
    try {
      socket= Utils::createSocket(options,host);
      socket->setTcpNoDelay(options->tcpNoDelay);

      if (options->socketTimeout/*.empty() != true*/){
        socket->setSoTimeout(options->socketTimeout);
      }
      if (options->tcpKeepAlive){
        socket->setKeepAlive(true);
      }
      if (options->tcpRcvBuf/*.empty() != true*/){
        socket->setReceiveBufferSize(options->tcpRcvBuf);
      }
      if (options->tcpSndBuf/*.empty() != true*/){
        socket->setSendBufferSize(options->tcpSndBuf);
      }
      if (options->tcpAbortiveClose){
        socket->setSoLinger(true,0);
      }



      if (options->localSocketAddress/*.empty() != true*/){
        InetSocketAddress localAddress= new InetSocketAddress(options->localSocketAddress,0);
        socket->bind(localAddress);
      }

      if (!socket->isConnected()){
        InetSocketAddress sockAddr =
          options->pipe/*.empty() != true*/ ?new InetSocketAddress(host,port):NULL;
        socket->connect(sockAddr,options->connectTimeout);
      }
      return socket;

    }catch (IOException& ioe){
      throw ExceptionMapper::connException(
          "Socket fail to connect to host:"+host +", port:"+port +". "+ioe.getMessage(),
          ioe);
    }
  }

  int64_t AbstractConnectProtocol::initializeClientCapabilities(
      const Shared::Options& options, int64_t serverCapabilities, const SQLString& database)
  {
    int64_t capabilities =
      MariaDbServerCapabilities::IGNORE_SPACE
      |MariaDbServerCapabilities::CLIENT_PROTOCOL_41
      |MariaDbServerCapabilities::TRANSACTIONS
      |MariaDbServerCapabilities::SECURE_CONNECTION
      |MariaDbServerCapabilities::MULTI_RESULTS
      |MariaDbServerCapabilities::PS_MULTI_RESULTS
      |MariaDbServerCapabilities::PLUGIN_AUTH
      |MariaDbServerCapabilities::CONNECT_ATTRS
      |MariaDbServerCapabilities::PLUGIN_AUTH_LENENC_CLIENT_DATA
      |MariaDbServerCapabilities::CLIENT_SESSION_TRACK;

    if (options->allowLocalInfile){
      capabilities |=MariaDbServerCapabilities::LOCAL_FILES;
    }




    if (!options->useAffectedRows){
      capabilities |=MariaDbServerCapabilities::FOUND_ROWS;
    }

    if (options->allowMultiQueries ||(options->rewriteBatchedStatements)){
      capabilities |=MariaDbServerCapabilities::MULTI_STATEMENTS;
    }

    if ((serverCapabilities &MariaDbServerCapabilities::CLIENT_DEPRECATE_EOF)!=0){
      capabilities |=MariaDbServerCapabilities::CLIENT_DEPRECATE_EOF;
    }

    if (options->useCompression){
      if ((serverCapabilities &MariaDbServerCapabilities::COMPRESS)==0){

        options->useCompression= false;
      }else {
        capabilities |=MariaDbServerCapabilities::COMPRESS;
      }
    }

    if (options->interactiveClient){
      capabilities |=MariaDbServerCapabilities::CLIENT_INTERACTIVE;
    }



    if (!database.empty()&&!options->createDatabaseIfNotExist){
      capabilities |=MariaDbServerCapabilities::CONNECT_WITH_DB;
    }
    return capabilities;
  }

  /**
   * Return possible protocols : values of option enabledSslProtocolSuites is set, or default to
   * "TLSv1,TLSv1.1". MariaDB versions &ge; 10.0.15 and &ge; 5.5.41 supports TLSv1.2 if compiled
   * with openSSL (default). MySQL community versions &ge; 5.7.10 is compile with yaSSL, so max TLS
   * is TLSv1.1.
   *
   * @param sslSocket current sslSocket
   * @throws SQLException if protocol isn't a supported protocol
   */
  void AbstractConnectProtocol::enabledSslProtocolSuites(SSLSocket* sslSocket, const Shared::Options& options)
  {
    if (options->enabledSslProtocolSuites/*.empty() != true*/){
      std::vector<SQLString>possibleProtocols= Arrays.asList(sslSocket->getSupportedProtocols());
      SQLString* protocols= options->enabledSslProtocolSuites.split("[,;\\s]+");
      for (SQLString protocol :protocols){
        if (!(possibleProtocols.find_first_of(protocol) != std::string::npos)){
          throw SQLException(
              "Unsupported SSL protocol '"
              +protocol
              +"'. Supported protocols : "
              +replace(possibleProtocols/*.toString()*/, "[", "").replace("]",""));
        }
      }
      sslSocket->setEnabledProtocols(protocols);
    }
  }

  /**
   * Set ssl socket cipher according to options.
   *
   * @param sslSocket current ssl socket
   * @throws SQLException if a cipher isn't known
   */
  void AbstractConnectProtocol::enabledSslCipherSuites(SSLSocket* sslSocket,Shared::Options& options)
  {
    if (options->enabledSslCipherSuites/*.empty() != true*/){
      std::vector<SQLString>possibleCiphers= Arrays.asList(sslSocket->getSupportedCipherSuites());
      SQLString* ciphers= options->enabledSslCipherSuites.split("[,;\\s]+");
      for (SQLString cipher :ciphers){
        if (!(possibleCiphers.find_first_of(cipher) != std::string::npos)){
          throw SQLException(
              "Unsupported SSL cipher '"
              +cipher
              +"'. Supported ciphers : "
              +replace(possibleCiphers/*.toString()*/, "[", "").replace("]",""));
        }
      }
      sslSocket->setEnabledCipherSuites(ciphers);
    }
  }

  /** Closes socket and stream readers/writers Attempts graceful shutdown. */
  void AbstractConnectProtocol::close()
  {
    bool locked= false;
    if (lock){
      locked= lock->try_lock();
    }
    this->connected= false;
    try {

      skip();
    }catch (Exception& e){

    }

    SendClosePacket.send(writer);
    closeSocket(reader,writer,socket);
    cleanMemory();
    if (locked){
      lock->unlock();
    }
  }

  /** Force closes socket and stream readers/writers. */
  void AbstractConnectProtocol::abort()
  {
    this->explicitClosed= true;

    bool lockStatus= false;
    if (lock/*.empty() != true*/){
      lockStatus= lock.tryLock();
    }
    this->connected= false;

    abortActiveStream();

    if (!lockStatus){


      forceAbort();
      try {
        socket->setSoTimeout(10);
        socket->setSoLinger(true,0);
      }catch (IOException& ioException){

      }
    }else {
      SendClosePacket.send(writer);
    }

    closeSocket(reader,writer,socket);
    cleanMemory();
    if (lockStatus){
      lock.unlock();
    }
  }

  void AbstractConnectProtocol::forceAbort()
  {
    try (MasterProtocol copiedProtocol =
        new MasterProtocol(urlParser,new GlobalStateInfo(),new std::mutex())){
      copiedProtocol.setHostAddress(getHostAddress());
      copiedProtocol.connect();

      copiedProtocol.executeQuery("KILL "+serverThreadId);
    }catch (SQLException& sqle){

    }
  }

  void AbstractConnectProtocol::abortActiveStream()
  {
    try {

      if (activeStreamingResult/*.empty() != true*/){
        activeStreamingResult.abort();
        activeStreamingResult= NULL;
      }
    }catch (Exception& e){

    }
  }

  /**
   * Skip packets not read that are not needed. Packets are read according to needs. If some data
   * have not been read before next execution, skip it. <i>Lock must be set before using this
   * method</i>
   *
   * @throws SQLException exception
   */
  void AbstractConnectProtocol::skip()
  {
    if (activeStreamingResult/*.empty() != true*/){
      activeStreamingResult.loadFully(true,this);
      activeStreamingResult= NULL;
    }
  }

  void AbstractConnectProtocol::cleanMemory()
  {
    if (options->cachePrepStmts &&options->useServerPrepStmts){
      serverPrepareStatementCache.clear();
    }
    if (options->enablePacketDebug){
      traceCache.clearMemory();
    }
  }

  void AbstractConnectProtocol::setServerStatus(int16_t serverStatus)
  {
    this->serverStatus= serverStatus;
  }

  /** Remove flag has more results. */  void AbstractConnectProtocol::removeHasMoreResults()
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
  void AbstractConnectProtocol::connect()
  {
    if (!isClosed()){
      close();
    }

    try {
      createConnection(currentHost,username);
    }catch (SQLException& exception){
      throw ExceptionMapper::connException(
          "Could not connect to "+currentHost +". "+exception.getMessage()+getTraces(),
          exception);
    }
  }

  void AbstractConnectProtocol::createConnection(HostAddress* hostAddress, const SQLString& username)
  {

    SQLString host= hostAddress != nullptr ? hostAddress->host : "";
    int32_t port= hostAddress != nullptr ? hostAddress->port :3306;

    Credential credential;
    CredentialPlugin credentialPlugin= urlParser->getCredentialPlugin();
    if (credentialPlugin){
      credential= credentialPlugin->initialize(options,username,hostAddress).get();
    }else {
      credential= new Credential(username,urlParser->getPassword());
    }

    this->socket= createSocket(host,port,options);
    assignStream(this->socket,options);

    try {


      const ReadInitialHandShakePacket greetingPacket= new ReadInitialHandShakePacket(reader);
      this->serverThreadId= greetingPacket.getServerThreadId();
      this->serverVersion= greetingPacket.getServerVersion();
      this->serverMariaDb= greetingPacket.isServerMariaDb();
      this->serverCapabilities= greetingPacket.getServerCapabilities();
      this->reader.setServerThreadId(serverThreadId,NULL);
      this->writer->setServerThreadId(serverThreadId,NULL);

      parseVersion(greetingPacket.getServerVersion());

      int8_t exchangeCharset= decideLanguage(greetingPacket.getServerLanguage()&0xFF);
      int64_t clientCapabilities= initializeClientCapabilities(options,serverCapabilities,database);

      sslWrapper(
          host,
          socket,
          options,
          greetingPacket.getServerCapabilities(),
          clientCapabilities,
          exchangeCharset,
          serverThreadId);

      SQLString authenticationPluginType= greetingPacket.getAuthenticationPluginType();
      if (credentialPlugin/*.empty() != true*/ &&credentialPlugin.defaultAuthenticationPluginType()/*.empty() != true*/){
        authenticationPluginType= credentialPlugin.defaultAuthenticationPluginType();
      }

      authenticationHandler(
          exchangeCharset,
          clientCapabilities,
          authenticationPluginType,
          greetingPacket.getSeed(),
          options,
          database,
          credential,
          host);

      compressionHandler(options);
    }catch (IOException& ioException){
      destroySocket();
      if (host/*.empty() != true*/){
        throw ExceptionMapper::connException(
            "Could not connect to socket : "+ioException.getMessage(),ioException);
      }
      throw ExceptionMapper::connException(
          "Could not connect to "
          +host
          +":"
          +socket->getPort()
          +" : "
          +ioException.getMessage(),
          ioException);
    }catch (IOException& ioException){
      destroySocket();
      throw sqlException;
    }

    connected= true;

    this->reader.setServerThreadId(this->serverThreadId,isMasterConnection());
    this->writer->setServerThreadId(this->serverThreadId,isMasterConnection());

    if (this->options->socketTimeout/*.empty() != true*/){
      this->socketTimeout= this->options->socketTimeout;
    }
    if ((serverCapabilities &MariaDbServerCapabilities::CLIENT_DEPRECATE_EOF)!=0){
      eofDeprecated= true;
    }

    postConnectionQueries();

    activeStreamingResult= NULL;
    hostFailed= false;
  }

  /** Closing socket in case of Connection error after socket creation. */  void AbstractConnectProtocol::destroySocket()
  {
    if (this->reader/*.empty() != true*/){
      try {
        this->reader.close();
      }catch (IOException& ee){

      }
    }
    if (this->writer){
      try {
        this->writer->close();
      }catch (std::exception& ee){

      }
    }
    if (this->socket){
      try {
        this->socket->close();
      }catch (std::exception& ee){

      }
    }
  }

  void AbstractConnectProtocol::sslWrapper(
      const SQLString& host,
      const Socket* socket,
      const Shared::Options& options,
      int64_t serverCapabilities,
      int64_t clientCapabilities,
      int8_t exchangeCharset,
      int64_t serverThreadId)
    {
      if (options->useSsl)
      {
        if ((serverCapabilities & MariaDbServerCapabilities::SSL)==0){
          throw SQLException("Trying to connect with ssl, but ssl not enabled in the server");
        }
        clientCapabilities |=MariaDbServerCapabilities::SSL;
        SendSslConnectionRequestPacket.send(writer,clientCapabilities,exchangeCharset);
        TlsSocketPlugin socketPlugin= TlsSocketPluginLoader.get(options->tlsSocketType);
        SSLSocketFactory sslSocketFactory= socketPlugin.getSocketFactory(options);
        SSLSocket sslSocket= socketPlugin.createSocket(socket,sslSocketFactory);

        enabledSslProtocolSuites(sslSocket,options);
        enabledSslCipherSuites(sslSocket,options);

        sslSocket->setUseClientMode(true);
        sslSocket->startHandshake();




        if (!options->disableSslHostnameVerification &&!options->trustServerCertificate){
          HostnameVerifierImpl hostnameVerifier= new HostnameVerifierImpl();
          SSLSession session= sslSocket->getSession();
          try {
            socketPlugin.verify(host,session,options,serverThreadId);
          }catch (SSLException& ex){
            throw SQLNonTransientConnectionException(
                "SSL hostname verification failed : "
                +ex.getMessage()
                +"\nThis verification can be disabled using the option \"disableSslHostnameVerification\" "
                "but won't prevent man-in-the-middle attacks anymore",
                "08006");
          }
        }

        assignStream(sslSocket,options);
      }
    }

  void AbstractConnectProtocol::authenticationHandler(
      int8_t exchangeCharset,
      int64_t clientCapabilities, const SQLString& authenticationPluginType,
      sql::bytes& seed,
      const Shared::Options& options, const SQLString& database,
      Credential* credential, const SQLString& host)
  {

    // send Client Handshake Response
    SendHandshakeResponsePacket.send(
        writer,
        credential,
        host,
        database,
        clientCapabilities,
        serverCapabilities,
        exchangeCharset,
        (byte) (Boolean.TRUE.equals(options.useSsl) ? 0x02 : 0x01),
        options,
        authenticationPluginType,
        seed);

    writer.permitTrace(false);

    Buffer buffer = reader.getPacket(false);
    AtomicInteger sequence = new AtomicInteger(reader.getLastPacketSeq());

    authentication_loop:
    while (true) {
      switch (buffer.getByteAt(0) & 0xFF) {
        case 0xFE:
          /**
           * ******************************************************************** Authentication
           * Switch Request see
           * https://mariadb.com/kb/en/library/connection/#authentication-switch-request
           * *******************************************************************
           */
          sequence.set(reader.getLastPacketSeq());
          AuthenticationPlugin authenticationPlugin;
          if ((serverCapabilities & MariaDbServerCapabilities.PLUGIN_AUTH) != 0) {
            buffer.readByte();
            String plugin;
            if (buffer.remaining() > 0) {
              // AuthSwitchRequest packet.
              plugin = buffer.readStringNullEnd(StandardCharsets.US_ASCII);
              seed = buffer.readRawBytes(buffer.remaining());
            } else {
              // OldAuthSwitchRequest
              plugin = OldPasswordPlugin.TYPE;
              seed = Utils.copyWithLength(seed, 8);
            }

            // Authentication according to plugin.
            // see AuthenticationProviderHolder for implement other plugin
            authenticationPlugin = AuthenticationPluginLoader.get(plugin);
          } else {
            authenticationPlugin = new OldPasswordPlugin();
            seed = Utils.copyWithLength(seed, 8);
          }

          if (authenticationPlugin.mustUseSsl() && options.useSsl == null) {
            throw new SQLException(
                "Connector use a plugin that require SSL without enabling ssl. "
                    + "For compatibility, this can still be disabled explicitly forcing "
                    + "'useSsl=false' in connection string."
                    + "plugin is = "
                    + authenticationPlugin.type(),
                "08004",
                1251);
          }

          authenticationPlugin.initialize(credential.getPassword(), seed, options);
          buffer = authenticationPlugin.process(writer, reader, sequence);
          break;

        case 0xFF:
          /**
           * ******************************************************************** ERR_Packet see
           * https://mariadb.com/kb/en/library/err_packet/
           * *******************************************************************
           */
          ErrorPacket errorPacket = new ErrorPacket(buffer);
          if (credential.getPassword() != null
              && !credential.getPassword().isEmpty()
              && options.passwordCharacterEncoding == null
              && errorPacket.getErrorNumber() == 1045
              && "28000".equals(errorPacket.getSqlState())) {
            // Access denied
            throw new SQLException(
                errorPacket.getMessage()
                    + "\nCurrent charset is "
                    + Charset.defaultCharset().displayName()
                    + ". If password has been set using other charset, consider "
                    + "using option 'passwordCharacterEncoding'",
                errorPacket.getSqlState(),
                errorPacket.getErrorNumber());
          }
          throw new SQLException(
              errorPacket.getMessage(), errorPacket.getSqlState(), errorPacket.getErrorNumber());

        case 0x00:
          /**
           * ******************************************************************** Authenticated !
           * OK_Packet see https://mariadb.com/kb/en/library/ok_packet/
           * *******************************************************************
           */
          OkPacket okPacket = new OkPacket(buffer);
          serverStatus = okPacket.getServerStatus();
          break authentication_loop;

        default:
          throw new SQLException(
              "unexpected data during authentication (header=" + (buffer.getByteAt(0) & 0xFF));
      }
    }
    writer.permitTrace(true);
  }

  void AbstractConnectProtocol::compressionHandler(Shared::Options options)
  {
    if (options->useCompression){
      writer= new CompressPacketOutputStream(writer->getOutputStream(),options->maxQuerySizeToLog);
      reader =
        new DecompressPacketInputStream(
            ((StandardPacketInputStream)reader).getInputStream(),options->maxQuerySizeToLog);
      if (options->enablePacketDebug){
        writer->setTraceCache(traceCache);
        reader.setTraceCache(traceCache);
      }
    }
  }

  void AbstractConnectProtocol::assignStream(Socket* socket, const Shared::Options& options)
  {
    try {
      this->writer= new StandardPacketOutputStream(socket->getOutputStream(),options);
      this->reader= new StandardPacketInputStream(socket->getInputStream(),options);

      if (options->enablePacketDebug){
        writer->setTraceCache(traceCache);
        reader->setTraceCache(traceCache);
      }

    }catch (IOException& ioe){
      destroySocket();
      throw ExceptionMapper::connException("Socket error: "+ioe.getMessage(),ioe);
    }
  }

  void AbstractConnectProtocol::postConnectionQueries()
  {
    try {

      bool mustLoadAdditionalInfo= true;
      if (globalInfo/*.empty() != true*/){
        if (globalInfo.isAutocommit()==options->autocommit){
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
            if ("08".equals(sqle.getSQLState())){
              throw sqle;
            }


            additionalData(serverData);
          }
        }else {
          additionalData(serverData);
        }

        writer->setMaxAllowedPacket(int32_t.parseInt(serverData.get("max_allowed_packet")));
        autoIncrementIncrement= int32_t.parseInt(serverData.get("auto_increment_increment"));
        loadCalendar(serverData.get("time_zone"),serverData.get("system_time_zone"));

      }else {

        writer->setMaxAllowedPacket(static_cast<int32_t>(globalInfo.getMaxAllowedPacket());)
          autoIncrementIncrement= globalInfo.getAutoIncrementIncrement();
        loadCalendar(globalInfo.getTimeZone(),globalInfo.getSystemTimeZone());
      }

      reader.setServerThreadId(this->serverThreadId,isMasterConnection());
      writer->setServerThreadId(this->serverThreadId,isMasterConnection());

      activeStreamingResult= NULL;
      hostFailed= false;
    }catch (SQLException& sqle){
      destroySocket();
      throw ExceptionMapper::connException(
          "Socket error during post connection queries: "+ioException.getMessage(),ioException);
    }catch (SQLException& sqle){
      destroySocket();
      throw sqlException;
    }
  }

  /**
   * Send all additional needed values. Command are send one after the other, assuming that command
   * are less than 65k (minimum hosts TCP/IP buffer size)
   *
   * @throws IOException if socket exception occur
   */
  void AbstractConnectProtocol::sendPipelineAdditionalData()
  {
    sendSessionInfos();
    sendRequestSessionVariables();

    sendPipelineCheckMaster();
  }

  void AbstractConnectProtocol::sendSessionInfos()
  {






    SQLString sessionOption("autocommit=").append(options->autocommit ?"1":"0");
    if ((serverCapabilities &MariaDbServerCapabilities::CLIENT_SESSION_TRACK)!=0){
      sessionOption.append(", session_track_schema=1");
      if (options->rewriteBatchedStatements){
        sessionOption.append(", session_track_system_variables='auto_increment_increment' ");
      }
    }

    if (options->jdbcCompliantTruncation){
      sessionOption.append(", sql_mode = concat(@@sql_mode,',STRICT_TRANS_TABLES')");
    }

    if (options->sessionVariables/*.empty() != true*/ &&!options->sessionVariables.empty()){
      sessionOption.append(",").append(Utils::parseSessionVariables(options->sessionVariables));
    }

    writer->startPacket(0);
    writer->write(COM_QUERY);
    writer->write("set "+sessionOption);
    writer->flush();
  }

  void AbstractConnectProtocol::sendRequestSessionVariables()
  {
    writer->startPacket(0);
    writer->write(COM_QUERY);
    writer->write(SESSION_QUERY);
    writer->flush();
  }

  void AbstractConnectProtocol::readRequestSessionVariables(Map<SQLString, const SQLString&>serverData)
  {
    Results results= new Results();
    getResult(results);

    results.commandEnd();
    ResultSet* resultSet= results.getResultSet();
    if (resultSet/*.empty() != true*/){
      resultSet->next();

      serverData.put("max_allowed_packet",resultSet->getString(1));
      serverData.put("system_time_zone",resultSet->getString(2));
      serverData.put("time_zone",resultSet->getString(3));
      serverData.put("auto_increment_increment",resultSet->getString(4));

    }else {
      throw SQLException(
          "Error reading SessionVariables results. Socket is connected ? "+socket->isConnected());
    }
  }

  void AbstractConnectProtocol::sendCreateDatabaseIfNotExist(const SQLString& quotedDb)
  {
    writer->startPacket(0);
    writer->write(COM_QUERY);
    writer->write("CREATE DATABASE IF NOT EXISTS "+quotedDb);
    writer->flush();
  }

  void AbstractConnectProtocol::sendUseDatabaseIfNotExist(const SQLString& quotedDb)
  {
    writer->startPacket(0);
    writer->write(COM_QUERY);
    writer->write("USE "+quotedDb);
    writer->flush();
  }

  void AbstractConnectProtocol::readPipelineAdditionalData(Map<SQLString, const SQLString&>serverData)
  {

    SQLException resultingException= NULL;

    try {
      getResult(new Results());
    }catch (SQLException& sqlException){

      resultingException= sqlException;
    }

    bool canTrySessionWithShow= false;
    try {
      readRequestSessionVariables(serverData);
    }catch (SQLException& sqlException){
      if (resultingException/*.empty() == true*/){
        resultingException =
          ExceptionMapper::connException("could not load system variables",sqlException);
        canTrySessionWithShow= true;
      }
    }

    try {
      readPipelineCheckMaster();
    }catch (SQLException& sqlException){
      canTrySessionWithShow= false;
      if (resultingException/*.empty() == true*/){
        throw ExceptionMapper::connException(
            "could not identified if server is master",sqlException);
      }
    }

    if (canTrySessionWithShow){


      requestSessionDataWithShow(serverData);
      connected= true;
      return;
    }

    if (resultingException/*.empty() == true*/){
      throw resultingException;
    }
    connected= true;
  }

  void AbstractConnectProtocol::requestSessionDataWithShow(Map<SQLString, const SQLString&>serverData)
  {
    try {
      Results results= new Results();
      executeQuery(
          true,
          results,
          "SHOW VARIABLES WHERE Variable_name in ("
          "'max_allowed_packet',"
          "'system_time_zone',"
          "'time_zone',"
          "'auto_increment_increment')");
      results.commandEnd();
      ResultSet* resultSet= results.getResultSet();
      if (resultSet/*.empty() != true*/){
        while (resultSet->next()){
          if (logger.isDebugEnabled()){
            logger.debug("server data {} = {}",resultSet->getString(1),resultSet->getString(2));
          }
          serverData.put(resultSet->getString(1),resultSet->getString(2));
        }
        if (serverData.size()<4){
          throw ExceptionMapper::connException(
              "could not load system variables. socket connected: "+socket->isConnected());
        }
      }

    }catch (SQLException& sqlException){
      throw ExceptionMapper::connException("could not load system variables",sqlException);
    }
  }

  void AbstractConnectProtocol::additionalData(Map<SQLString, const SQLString&>serverData),SQLException
  {

    sendSessionInfos();
    getResult(new Results());

    try {
      sendRequestSessionVariables();
      readRequestSessionVariables(serverData);
    }catch (SQLException& sqlException){
      requestSessionDataWithShow(serverData);
    }


    sendPipelineCheckMaster();
    readPipelineCheckMaster();

    if (options->createDatabaseIfNotExist &&!database.empty()){

      SQLString quotedDb= MariaDbConnection*.quoteIdentifier(this->database);
      sendCreateDatabaseIfNotExist(quotedDb);
      getResult(new Results());

      sendUseDatabaseIfNotExist(quotedDb);
      getResult(new Results());
    }
  }

  /**
   * Is the connection closed.
   *
   * @return true if the connection is closed
   */
  bool AbstractConnectProtocol::isClosed()
  {
    return !this->connected;
  }

  void AbstractConnectProtocol::loadCalendar(const SQLString& srvTimeZone,const SQLString& srvSystemTimeZone)
  {
    if (options->useLegacyDatetimeCode){

      timeZone= Calendar&.getInstance().getTimeZone();
    }else {

      SQLString tz= options->serverTimezone;
      if (tz/*.empty() == true*/){
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
  }

  /**
   * Check that current connection is a master connection (not read-only).
   *
   * @return true if master
   * @throws SQLException if requesting infos for server fail.
   */
  bool AbstractConnectProtocol::checkIfMaster()
  {
    return isMasterConnection();
  }

  /**
   * Default collation used for string exchanges with server.
   *
   * @param serverLanguage server default collation
   * @return collation byte
   */
  int8_t AbstractConnectProtocol::decideLanguage(int32_t serverLanguage)
  {

    if (serverLanguage == 45
        ||serverLanguage == 46
        ||(serverLanguage >=224 &&serverLanguage <=247)){
      return (int8_t)serverLanguage;
    }
    if (getMajorServerVersion()==5 &&getMinorServerVersion()<=1){

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
  void AbstractConnectProtocol::readEofPacket(),IOException
  {
    Buffer buffer= reader.getPacket(true);
    switch (buffer.getByteAt(0)){
      case EOF:
        buffer.skipByte();
        this->hasWarningsFlag= buffer.readShort()>0;
        this->serverStatus= buffer.readShort();
        break;

      case ERROR:
        ErrorPacket ep= new ErrorPacket(buffer);
        throw SQLException(
            "Could not connect: "+ep.getMessage(),ep.getSqlState(),ep.getErrorNumber());

      default:
        throw SQLException("Unexpected packet type "+buffer.getByteAt(0)+" instead of EOF");
    }
  }

  /**
   * Check that next read packet is a End-of-file packet.
   *
   * @throws SQLException if not a End-of-file packet
   * @throws IOException if connection error occur
   */
  void AbstractConnectProtocol::skipEofPacket(),IOException
  {
    Buffer buffer= reader.getPacket(true);
    switch (buffer.getByteAt(0)){
      case EOF:
        break;

      case ERROR:
        ErrorPacket ep= new ErrorPacket(buffer);
        throw SQLException(
            "Could not connect: "+ep.getMessage(),ep.getSqlState(),ep.getErrorNumber());

      default:
        throw SQLException("Unexpected packet type "+buffer.getByteAt(0)+" instead of EOF");
    }
  }

  void AbstractConnectProtocol::setHostFailedWithoutProxy()
  {
    hostFailed= true;
    close();
  }

  UrlParser AbstractConnectProtocol::getUrlParser()
  {
    return urlParser;
  }

  /**
   * Indicate if current protocol is a master protocol.
   *
   * @return is master flag
   */
  bool AbstractConnectProtocol::isMasterConnection()
  {
    return currentHost/*.empty() == true*/ ||(ParameterConstant::TYPE_MASTER.compare(currentHost.type) == 0);
  }

  /**
   * Send query to identify if server is master.
   *
   * @throws IOException in case of socket error.
   */
  void AbstractConnectProtocol::sendPipelineCheckMaster()
  {
    if (urlParser->getHaMode()==enum HaMode::AURORA){
      writer->startPacket(0);
      writer->write(COM_QUERY);
      writer->write(IS_MASTER_QUERY);
      writer->flush();
    }
  }

  void AbstractConnectProtocol::readPipelineCheckMaster()
  {

  }

  bool AbstractConnectProtocol::mustBeMasterConnection()
  {
    return true;
  }

  bool AbstractConnectProtocol::noBackslashEscapes()
  {
    return ((serverStatus &ServerStatus::NO_BACKSLASH_ESCAPES)!=0);
  }

  /**
   * Connect without proxy. (use basic failover implementation)
   *
   * @throws SQLException exception
   */
  void AbstractConnectProtocol::connectWithoutProxy()
  {
    if (!isClosed()){
      close();
    }

    std::vector<HostAddress>addrs= urlParser->getHostAddresses();
    LinkedList<HostAddress>hosts= new LinkedList<>(addrs);

    if ((urlParser->getHaMode().compare(enum HaMode::LOADBALANCE) == 0)){
      Collections.shuffle(hosts);
    }


    if (hosts.empty() && options->pipe){
      try {
        createConnection(NULL,username);
        return;
      }catch (SQLException& exception){
        throw ExceptionMapper::connException(
            "Could not connect to named pipe '"
            +options->pipe
            +"' : "
            +exception.getMessage()
            +getTraces(),
            exception);
      }
    }




    while (!hosts.empty()){
      currentHost= hosts.pop_front();
      try {
        createConnection(currentHost,username);
        return;
      }catch (SQLException& exception){
        if (hosts.empty()){
          if (e.getSQLState()/*.empty() != true*/){
            throw ExceptionMapper::get(
                "Could not connect to "
                +HostAddress::toString(addrs)
                +" : "
                +e.getMessage()
                +getTraces(),
                e.getSQLState(),
                e.getErrorCode(),
                e,
                false);
          }
          throw ExceptionMapper::connException(
              "Could not connect to "+currentHost +". "+e.getMessage()+getTraces(),e);
        }
      }
    }
  }

  /**
   * Indicate for Old reconnection if can reconnect without throwing exception.
   *
   * @return true if can reconnect without issue
   */
  bool AbstractConnectProtocol::shouldReconnectWithoutProxy()
  {
    return (((serverStatus & ServerStatus::IN_TRANSACTION)==0)
        && hostFailed
        && urlParser->getOptions()->autoReconnect);
  }

  SQLString AbstractConnectProtocol::getServerVersion()
  {
    return serverVersion;
  }

  bool AbstractConnectProtocol::getReadonly()
  {
    return readOnly;
  }

  void AbstractConnectProtocol::setReadonly(bool readOnly)
  {
    this->readOnly= readOnly;
  }

  HostAddress AbstractConnectProtocol::getHostAddress()
  {
    return currentHost;
  }

  void AbstractConnectProtocol::setHostAddress(HostAddress host)
  {
    this->currentHost= host;
    this->readOnly= (ParameterConstant::TYPE_SLAVE.compare(this->currentHost.type) == 0);
  }

  SQLString AbstractConnectProtocol::getHost()
  {
    return (currentHost/*.empty() == true*/)?NULL :currentHost.host;
  }

  FailoverProxy AbstractConnectProtocol::getProxy()
  {
    return proxy;
  }

  void AbstractConnectProtocol::setProxy(FailoverProxy proxy)
  {
    this->proxy= proxy;
  }

  int32_t AbstractConnectProtocol::getPort()
  {
    return (currentHost/*.empty() == true*/)?3306 :currentHost.port;
  }

  SQLString AbstractConnectProtocol::getDatabase()
  {
    return database;
  }

  SQLString AbstractConnectProtocol::getUsername()
  {
    return username;
  }

  void AbstractConnectProtocol::parseVersion(const SQLString& serverVersion)
  {
    int32_t length= serverVersion::size();
    char car;
    int32_t offset= 0;
    int32_t type= 0;
    int32_t val= 0;
    for (;offset <length;offset++){
      car= serverVersion::at(offset);
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

  int32_t AbstractConnectProtocol::getMajorServerVersion()
  {
    return majorVersion;
  }

  int32_t AbstractConnectProtocol::getMinorServerVersion()
  {
    return minorVersion;
  }

  /**
   * Utility method to check if database version is greater than parameters.
   *
   * @param major major version
   * @param minor minor version
   * @param patch patch version
   * @return true if version is greater than parameters
   */
  bool AbstractConnectProtocol::versionGreaterOrEqual(int32_t major,int32_t minor,int32_t patch)
    bool AbstractConnectProtocol::getPinGlobalTxToPhysicalConnection()
    {
      return this->options->pinGlobalTxToPhysicalConnection;
    }

  /**
   * Has warnings.
   *
   * @return true if as warnings.
   */
  bool AbstractConnectProtocol::hasWarnings()
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {
      return hasWarningsFlag;
    }/* TODO: something with the finally was once here */ {
      lock.unlock();
    }
  }

  /**
   * Is connected.
   *
   * @return true if connected
   */
  bool AbstractConnectProtocol::isConnected()
  {
    std::lock_guard<std::mutex> localScopeLock(*lock);
    try {
      return connected;
    }/* TODO: something with the finally was once here */ {
      lock.unlock();
    }
  }

  int64_t AbstractConnectProtocol::getServerThreadId()
  {
    return serverThreadId;
  }

  Socket AbstractConnectProtocol::getSocket()
  {
    return socket;
  }

  bool AbstractConnectProtocol::isExplicitClosed()
  {
    return explicitClosed;
  }

  TimeZone AbstractConnectProtocol::getTimeZone()
  {
    return timeZone;
  }

  Shared::Options AbstractConnectProtocol::getOptions()
  {
    return options;
  }

  void AbstractConnectProtocol::setHasWarnings(bool _hasWarnings)
  {
    this->hasWarningsFlag= _hasWarnings;
  }

  Results AbstractConnectProtocol::getActiveStreamingResult()
  {
    return activeStreamingResult;
  }

  void AbstractConnectProtocol::setActiveStreamingResult(Results activeStreamingResult)
  {
    this->activeStreamingResult= activeStreamingResult;
  }

  /** Remove exception result and since totally fetched, set fetch size to 0. */  void AbstractConnectProtocol::removeActiveStreamingResult()
  {
    if (this->activeStreamingResult/*.empty() != true*/){
      this->activeStreamingResult.removeFetchSize();
      this->activeStreamingResult= NULL;
    }
  }

  std::mutex AbstractConnectProtocol::getLock()
  {
    return lock;
  }

  bool AbstractConnectProtocol::hasMoreResults()
  {
    return (serverStatus & ServerStatus::MORE_RESULTS_EXISTS)!=0;
  }

  ServerPrepareStatementCache* AbstractConnectProtocol::prepareStatementCache()
  {
    return serverPrepareStatementCache;
  }

  abstract void executeQuery(const SQLString& sql);
  /**
   * Change Socket TcpNoDelay option.
   *
   * @param setTcpNoDelay value to set.
   */
  void AbstractConnectProtocol::changeSocketTcpNoDelay(bool setTcpNoDelay)
  {
    try {
      socket->setTcpNoDelay(setTcpNoDelay);
    }catch (SocketException& socketException){

    }
  }

  void AbstractConnectProtocol::changeSocketSoTimeout(int32_t setSoTimeout)
  {
    this->socketTimeout= setSoTimeout;
    socket->setSoTimeout(this->socketTimeout);
  }

  bool AbstractConnectProtocol::isServerMariaDb()
  {
    return serverMariaDb;
  }

  PacketInputStream AbstractConnectProtocol::getReader()
  {
    return reader;
  }

  PacketOutputStream AbstractConnectProtocol::getWriter()
  {
    return writer;
  }

  bool AbstractConnectProtocol::isEofDeprecated()
  {
    return eofDeprecated;
  }

  bool AbstractConnectProtocol::sessionStateAware()
  {
    return (serverCapabilities & MariaDbServerCapabilities::CLIENT_SESSION_TRACK)!=0;
  }

  /**
   * Get a String containing readable information about last 10 send/received packets.
   *
   * @return String value
   */
  SQLString AbstractConnectProtocol::getTraces()
  {
    if (options->enablePacketDebug){
      //return traceCache.printStack();
    }
    return "";
  }
}
}
