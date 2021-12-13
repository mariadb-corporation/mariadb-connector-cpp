/************************************************************************************
  Copyright (C) 2021 MariaDB Corporation AB

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

#include "MariaDbDataSourceInternal.h"
#include "MariaDbPoolConnection.h"
#include "ExceptionFactory.h"
#include "options/DefaultOptions.h"
#include "pool/Pools.h"
#include "pool/Pool.h"
#include "Consts.h"

namespace sql
{
namespace mariadb
{
  /**
   * Constructor.
   *
   * @param hostname hostname (ipv4, ipv6, dns name)
   * @param port server port
   * @param database database name
   */
  MariaDbDataSource::MariaDbDataSource(const SQLString& hostname,int32_t port, const SQLString& database)
  {
    internal.reset(new MariaDbDataSourceInternal(hostname, port, database));
  }

  MariaDbDataSource::MariaDbDataSource(const SQLString& url)
  {
    internal.reset(new MariaDbDataSourceInternal(url));
  }

  /** Default constructor. hostname will be localhost, port 3306. */
  MariaDbDataSource::MariaDbDataSource()
  {
    internal.reset(new MariaDbDataSourceInternal(localhost, 3306, emptyStr));
  }


  MariaDbDataSource::~MariaDbDataSource() {
  }

  /**
    * Gets the name of the database.
    *
    * @return the name of the database for this data source
    */
  const SQLString& MariaDbDataSource::getDatabaseName()
  {
    if (!internal->database.empty()){
      return internal->database;
    }
    return (internal->urlParser && internal->urlParser->getDatabase() ? internal->urlParser->getDatabase() : emptyStr);
  }

  /**
   * Sets the database name.
   *
   * @param database the name of the database
   * @throws SQLException if connection information are erroneous
   */
  void MariaDbDataSource::setDatabaseName(const SQLString& database)
  {
    internal->database= database;
    internal->reInitializeIfNeeded();
  }

  /**
   * Gets the username.
   *
   * @return the username to use when connecting to the database
   */
  const SQLString& MariaDbDataSource::getUser()
  {
    if (!internal->user.empty()){
      return internal->user;
    }
    return internal->urlParser ? internal->urlParser->getUsername() : emptyStr;
  }

  /**
   * Sets the username.
   *
   * @param user the username
   * @throws SQLException if connection information are erroneous
   */
  void MariaDbDataSource::setUser(const SQLString& user)
  {
    internal->user= user;
    internal->reInitializeIfNeeded();
  }

  /**
   * Gets the username.
   *
   * @return the username to use when connecting to the database
   */
  const SQLString& MariaDbDataSource::getUserName()
  {
    return getUser();
  }

  /**
   * Sets the username.
   *
   * @param userName the username
   * @throws SQLException if connection information are erroneous
   */
  void MariaDbDataSource::setUserName(const SQLString& userName)
  {
    setUser(userName);
  }

  /**
   * Sets the password.
   *
   * @param password the password
   * @throws SQLException if connection information are erroneous
   */
  void MariaDbDataSource::setPassword(const SQLString& password)
  {
    internal->password= password;
    internal->reInitializeIfNeeded();
  }

  /**
   * Returns the port number.
   *
   * @return the port number
   */
  int32_t MariaDbDataSource::getPort()
  {
    if (internal->port != 0){
      return internal->port;
    }
    return internal->urlParser ? internal->urlParser->getHostAddresses()[0].port : 3306;
  }

  /**
   * Sets the database port.
   *
   * @param port the port
   * @throws SQLException if connection information are erroneous
   */
  void MariaDbDataSource::setPort(int32_t port)
  {
    internal->port= port;
    internal->reInitializeIfNeeded();
  }

  /**
   * Returns the port number.
   *
   * @return the port number
   */
  int32_t MariaDbDataSource::getPortNumber()
  {
    return getPort();
  }

  /**
   * Sets the port number.
   *
   * @param port the port
   * @throws SQLException if connection information are erroneous
   * @see #setPort
   */
  void MariaDbDataSource::setPortNumber(int32_t port)
  {
    if (port >0){
      setPort(port);
    }
  }

  void MariaDbDataSource::setProperties(const SQLString& properties)
  {
    internal->properties= properties;
    internal->reInitializeIfNeeded();
  }

  /**
   * Sets the connection string URL.
   *
   * @param url the connection string
   * @throws SQLException if error in URL
   */
  void MariaDbDataSource::setUrl(const SQLString& url)
  {
    internal->url= url;
    internal->reInitializeIfNeeded();
  }

  /**
   * Returns the name of the database server.
   *
   * @return the name of the database server
   */
  const SQLString& MariaDbDataSource::getServerName()
  {
    if (!internal->hostname.empty()){
      return internal->hostname;
    }
    bool hasHost= !internal->urlParser && internal->urlParser->getHostAddresses()[0].host.empty();
    return (hasHost) ? internal->urlParser->getHostAddresses()[0].host : localhost;
  }

  /**
   * Sets the server name.
   *
   * @param serverName the server name
   * @throws SQLException if connection information are erroneous
   */
  void MariaDbDataSource::setServerName(const SQLString& serverName)
  {
    internal->hostname= serverName;
    internal->reInitializeIfNeeded();
  }

  /**
   * Attempts to establish a connection with the data source that this <code>DataSource</code>
   * object represents.
   *
   * @return a connection to the data source
   * @throws SQLException if a database access error occurs
   */
  Connection* MariaDbDataSource::getConnection()
  {
    try {
      if (!internal->urlParser){
        internal->initialize();
      }

      return MariaDbConnection::newConnection(internal->urlParser, nullptr);
    }catch (SQLException& e){
      throw ExceptionFactory::INSTANCE.create(e);
    }
  }

  /**
   * Attempts to establish a connection with the data source that this <code>DataSource</code>
   * object represents.
   *
   * @param username the database user on whose behalf the connection is being made
   * @param password the user's password
   * @return a connection to the data source
   * @throws SQLException if a database access error occurs
   */
  Connection* MariaDbDataSource::getConnection(const SQLString& username,const SQLString& passwd)
  {
    try {
      if (!internal->urlParser){
        internal->user= username;
        internal->password= passwd;
        internal->initialize();
      }

      Shared::UrlParser urlParser(this->internal->urlParser->clone());
      internal->urlParser->setUsername(username);
      internal->urlParser->setPassword(passwd);
      return MariaDbConnection::newConnection(urlParser, nullptr);

    }
    catch (SQLException& e){
      throw ExceptionFactory::INSTANCE.create(e);
    }
    /*catch (CloneNotSupportedException& e){
      throw ExceptionFactory::INSTANCE.create("Error in configuration");
    }*/
  }

  /**
   * Retrieves the log writer for this <code>DataSource</code> object.
   *
   * <p>The log writer is a character output stream to which all logging and tracing messages for
   * this data source will be printed. This includes messages printed by the methods of this object,
   * messages printed by methods of other objects manufactured by this object, and so on. Messages
   * printed to a data source specific log writer are not printed to the log writer associated with
   * the <code>java.sql.DriverManager</code> class. When a <code>DataSource</code> object is
   * created, the log writer is initially null; in other words, the default is for logging to be
   * disabled.
   *
   * @return the log writer for this data source or null if logging is disabled
   * @see #setLogWriter
   */
  PrintWriter* MariaDbDataSource::getLogWriter()
  {
    return nullptr;
  }

  /**
   * Sets the log writer for this <code>DataSource</code> object to the given <code>
   * java.io.PrintWriter</code> object.
   *
   * <p>The log writer is a character output stream to which all logging and tracing messages for
   * this data source will be printed. This includes messages printed by the methods of this object,
   * messages printed by methods of other objects manufactured by this object, and so on. Messages
   * printed to a data source- specific log writer are not printed to the log writer associated with
   * the <code>java.sql.DriverManager</code> class. When a <code>DataSource</code> object is created
   * the log writer is initially null; in other words, the default is for logging to be disabled.
   *
   * @param out the new log writer; to disable logging, set to null
   * @see #getLogWriter
   */
  void MariaDbDataSource::setLogWriter(const PrintWriter& out)
  {

  }

  /**
   * Gets the maximum time in seconds that this data source can wait while attempting to connect to
   * a database. A value of zero means that the timeout is the default system timeout if there is
   * one; otherwise, it means that there is no timeout. When a <code>DataSource</code> object is
   * created, the login timeout is initially zero.
   *
   * @return the data source login time limit
   * @see #setLoginTimeout
   */
  int32_t MariaDbDataSource::getLoginTimeout()
  {
    if (internal->connectTimeoutInMs/*.empty() != true*/){
      return internal->connectTimeoutInMs / 1000;
    }
    return (internal->urlParser) ? internal->urlParser->getOptions()->connectTimeout / 1000 : 30;
  }


  void MariaDbDataSource::setLoginTimeout(int32_t seconds)
  {
    internal->connectTimeoutInMs= seconds * 1000;
  }

  /**
   * Attempts to establish a physical database connection that can be used as a pooled connection.
   *
   * @return a <code>PooledConnection</code> object that is a physical connection to the database
   *     that this <code>ConnectionPoolDataSource</code> object represents
   * @throws SQLException if a database access error occurs
   */
  PooledConnection* MariaDbDataSource::getPooledConnection()
  {
    throw SQLFeatureNotSupportedException("getPooledConnection() is not supported");
    /* The line below will even compile. But letting things as simple as possible - getConnection() will
       return a connection from the pool(if pool option is selected) */
    //return new MariaDbPoolConnection(dynamic_cast<MariaDbConnection*>(getConnection()));
  }

  /**
   * Attempts to establish a physical database connection that can be used as a pooled connection.
   *
   * @param user the database user on whose behalf the connection is being made
   * @param password the user's password
   * @return a <code>PooledConnection</code> object that is a physical connection to the database
   *     that this <code>ConnectionPoolDataSource</code> object represents
   * @throws SQLException if a database access error occurs
   */
  PooledConnection* MariaDbDataSource::getPooledConnection(const SQLString& user, const SQLString& password)
  {
    throw SQLFeatureNotSupportedException("getPooledConnection() is not supported");
    /* The line below will even compile. But letting things as simple as possible - getConnection() will
       return a connection from the pool(if pool option is selected) */
    //return new MariaDbPoolConnection(dynamic_cast<MariaDbConnection*>(getConnection(user,password)));
  }

  XAConnection* MariaDbDataSource::getXAConnection()
  {
    throw SQLFeatureNotSupportedException("getXAConnection() is not supported");
    //return new MariaXaConnection(dynamic_cast<MariaDbConnection*>(getConnection()));
  }

  XAConnection* MariaDbDataSource::getXAConnection(const SQLString& user, const SQLString& password)
  {
    throw SQLFeatureNotSupportedException("getXAConnection() is not supported");
    //return new MariaXaConnection((MariaDbConnection*)getConnection(user,password));
  }

  sql::Logger* MariaDbDataSource::getParentLogger()
  {
    return nullptr;
  }

  /** Close datasource. This an extension to JDBC to close corresponding connections pool */
  void MariaDbDataSource::close() {
    Pools::retrievePool(internal->urlParser)->close();
  }

  // ------------------------- MariaDbDataSourceInternal ---------------------------
  /**
   * For testing purpose only.
   *
   * @return current url parser.
   */
  UrlParser* MariaDbDataSourceInternal::getUrlParser()
  {
    return urlParser.get();
  }

  void MariaDbDataSourceInternal::reInitializeIfNeeded()
  {
    if (urlParser){
      initialize();
    }
  }

  
  void MariaDbDataSourceInternal::initialize()
  {
    std::unique_lock<std::mutex> localScopeLock(syncronization);

    if (!url.empty()) {
      // With DS using pool by default. Perhaps would be better to "implement" getPooledConnection, and have it different for getPooledConnection and getConnection
      PropertiesImp::ImpType props{ {"pool", "true"}};
      if (!user.empty()){
        props["user"]= user;
      }
      if (!password.empty()) {
        props["password"]= password;
      }
      if (!database.empty()){
        props["schema"]= database;
      }
      if (connectTimeoutInMs != 0){
        props["connectTimeout"]= std::to_string(connectTimeoutInMs);
      }

      urlParser.reset(UrlParser::parse(url, props));
    }
    else {
      Shared::Options options= DefaultOptions::defaultValues(HaMode::NONE);
      options->user= user;
      options->password= password;

      std::vector<HostAddress> hosts({ HostAddress(
        hostname.empty() ? localhost : hostname,
        port == 0 ? 3306 : port) });
      urlParser.reset(
        new UrlParser(
            database,
            hosts,
            options,
            HaMode::NONE));
      if (!properties.empty()){
        urlParser->setProperties(properties);
      }
      if (connectTimeoutInMs != 0){
        urlParser->getOptions()->connectTimeout= connectTimeoutInMs;
      }
    }
  }
}
}
