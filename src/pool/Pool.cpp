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


#include "Pool.h"

namespace sql
{
namespace mariadb
{
  const Shared::Logger logger= LoggerFactory::getLogger(typeid(Pool));
  int32_t Pool::POOL_STATE_OK= 0;
  int32_t Pool::POOL_STATE_CLOSING= 1;

  /**
    * Create pool from configuration.
    *
    * @param urlParser configuration parser
    * @param poolIndex pool index to permit distinction of thread name
    * @param poolExecutor pools common executor
    */
  Pool::Pool(UrlParser &urlParser, int32_t poolIndex, ScheduledThreadPoolExecutor& poolExecutor) :
    urlParser(urlParser),
    options(urlParser.getOptions()),
    maxIdleTime(options->maxIdleTime)
  {
    poolTag= generatePoolTag(poolIndex);

    connectionAppenderQueue= new ArrayBlockingQueue<>(options->maxPoolSize);
    connectionAppender =
      new ThreadPoolExecutor(
        1,
        1,
        10,
        TimeUnit::SECONDS,
        connectionAppenderQueue,
        new MariaDbThreadFactory(poolTag +"-appender"));
    connectionAppender.allowCoreThreadTimeOut(true);

    connectionAppender.prestartCoreThread();

    idleConnections= new LinkedBlockingDeque<>();

    int32_t scheduleDelay= Math.min(30, maxIdleTime 2);
    this->poolExecutor= poolExecutor;
    scheduledFuture =
      poolExecutor.scheduleAtFixedRate(
        this::removeIdleTimeoutConnection, scheduleDelay, scheduleDelay, TimeUnit.SECONDS);

    try
    {
      for (int32_t i= 0; i <options->minPoolSize; i++) {
        addConnection();
      }
    }
    catch (Exception& ex) {
      logger.error("error initializing pool connection", sqle);
    }
  }

  /**
    * Add new connection if needed. Only one thread create new connection, so new connection request
    * will wait to newly created connection or for a released connection.
    */
  void Pool::addConnectionRequest()
  {
    if (totalConnection.get() < options->maxPoolSize && poolState.get() == POOL_STATE_OK)
    {
      connectionAppender.prestartCoreThread();
      connectionAppenderQueue.offer(
        ()->{
        if ((totalConnection.get()<options->minPoolSize ||pendingRequestNumber.get()>0)
          &&totalConnection.get()<options->maxPoolSize) {
          try {
            addConnection();
          }
          catch (SQLException&)
          {
          }
        }
      });
    }
  }

  /**
    * Removing idle connection. Close them and recreate connection to reach minimal number of
    * connection.
    */
  void Pool::removeIdleTimeoutConnection()
  {
    Iterator<MariaDbPooledConnection>iterator= idleConnections.descendingIterator();
    MariaDbPooledConnection item;

    while (iterator.hasNext())
    {
      item= iterator.next();

      int64_t idleTime= System.nanoTime()-item.getLastUsed().get();
      bool timedOut= idleTime >TimeUnit.SECONDS.toNanos(maxIdleTime);

      bool shouldBeReleased= false;

      if (globalInfo) {


        if (idleTime >TimeUnit.SECONDS.toNanos(globalInfo.getWaitTimeout()-45)) {
          shouldBeReleased= true;
        }


        if (timedOut &&totalConnection.get()>options->minPoolSize) {
          shouldBeReleased= true;
        }

      }
      else if (timedOut) {
        shouldBeReleased= true;
      }

      if (shouldBeReleased &&idleConnections.remove(item)) {

        totalConnection.decrementAndGet();
        silentCloseConnection(item);
        addConnectionRequest();
        if (logger.isDebugEnabled()) {
          logger.debug(
            "pool {} connection removed due to inactivity (total:{}, active:{}, pending:{})",
            poolTag,
            totalConnection.get(),
            getActiveConnections(),
            pendingRequestNumber.get());
        }


      }
    }
  }

  /**
    * Create new connection.
    *
    * @throws SQLException if connection creation failed
    */
  void Pool::addConnection() {

    Protocol& protocol= Utils::retrieveProxy(urlParser, globalInfo);
    MariaDbConnection connection= new MariaDbConnection(protocol);
    MariaDbPooledConnection pooledConnection= createPoolConnection(connection);

    if (options->staticGlobal) {

      if (globalInfo) {
        initializePoolGlobalState(connection);
      }

      connection.setDefaultTransactionIsolation(globalInfo.getDefaultTransactionIsolation());
    }
    else {

      connection.setDefaultTransactionIsolation(connection.getTransactionIsolation());
    }

    if (poolState.get()==POOL_STATE_OK
      &&totalConnection.incrementAndGet()<=options->maxPoolSize) {
      idleConnections.addFirst(pooledConnection);

      if (logger.isDebugEnabled()) {
        logger.debug(
          "pool {} new physical connection created (total:{}, active:{}, pending:{})",
          poolTag,
          totalConnection.get(),
          getActiveConnections(),
          pendingRequestNumber.get());
      }
      return;
    }

    silentCloseConnection(pooledConnection);
  }

  MariaDbPooledConnection Pool::getIdleConnection() {
    return getIdleConnection(0, TimeUnit.NANOSECONDS);
  }

  /**
    * Get an existing idle connection in pool.
    *
    * @return an IDLE connection.
    */
  MariaDbPooledConnection Pool::getIdleConnection(int64_t timeout, TimeUnit timeUnit) {


    while (true) {
      MariaDbPooledConnection item =
        (timeout ==0)
        ? idleConnections.pollFirst()
        : idleConnections.pollFirst(timeout, timeUnit);

      if (item) {
        MariaDbConnection connection= item.getConnection();
        try {
          if (TimeUnit.NANOSECONDS.toMillis(System.nanoTime()-item.getLastUsed().get())
>options->poolValidMinDelay) {


            if (connection.isValid(10)) {
              item.lastUsedToNow();
              return item;
            }

          }
          else {


            item.lastUsedToNow();
            return item;
          }

        }
        catch (SQLException& sqle) {

        }

        totalConnection.decrementAndGet();


        silentAbortConnection(item);
        addConnectionRequest();
        if (logger.isDebugEnabled()) {
          logger.debug(
            "pool {} connection removed from pool due to failed validation (total:{}, active:{}, pending:{})",
            poolTag,
            totalConnection.get(),
            getActiveConnections(),
            pendingRequestNumber.get());
        }
        continue;
      }

      return NULL;
    }
  }

  void Pool::silentCloseConnection(MariaDbPooledConnection item) {
    try {
      item.close();
    }
    catch (SQLException& ex) {

    }
  }

  void Pool::silentAbortConnection(MariaDbPooledConnection item) {
    try {
      item.abort(poolExecutor);
    }
    catch (SQLException& ex) {

    }
  }

  MariaDbPooledConnection Pool::createPoolConnection(MariaDbConnection connection) {
    MariaDbPooledConnection pooledConnection= new MariaDbPooledConnection(connection);
    pooledConnection.addConnectionEventListener(
      new ConnectionEventListener(){

      @Override
      void Pool::connectionClosed(ConnectionEvent event) {
      MariaDbPooledConnection item =(MariaDbPooledConnection)event.getSource();
      if (poolState.get()==POOL_STATE_OK) {
      try {
      if (!(idleConnections.find_first_of(item) >= 0)) {
      item.getConnection().reset();
      idleConnections.addFirst(item);
      }
      }
      catch (SQLException& sqle) {


      totalConnection.decrementAndGet();
      silentCloseConnection(item);
      logger.debug("connection removed from pool {} due to error during reset",poolTag);
      }
      }
      else {

    try {
    item.close();
    }
    catch (SQLException& sqle) {

    }
    totalConnection.decrementAndGet();
    }
    }

    @Override
    void Pool::connectionClosed(ConnectionEvent event) {

    MariaDbPooledConnection item =((MariaDbPooledConnection)event.getSource());
    if (idleConnections.remove(item)) {
    totalConnection.decrementAndGet();
    }
    silentCloseConnection(item);
    addConnectionRequest();
    logger.debug(
    "connection {} removed from pool {} due to having throw a Connection* exception (total:{}, active:{}, pending:{})",
    item.getConnection().getServerThreadId(),
    poolTag,
    totalConnection.get(),
    getActiveConnections(),
    pendingRequestNumber.get());
    }
      });
    return pooledConnection;
  }

  /**
    * Retrieve new connection. If possible return idle connection, if not, stack connection query,
    * ask for a connection creation, and loop until a connection become idle / a new connection is
    * created.
    *
    * @return a connection object
    * @throws SQLException if no connection is created when reaching timeout (connectTimeout option)
    */
  MariaDbConnection* Pool::getConnection()
  {
    pendingRequestNumber.incrementAndGet();

    MariaDbPooledConnection pooledConnection;

    try {


      if ((pooledConnection =
        getIdleConnection(totalConnection.get()>4 ? 0 : 50, TimeUnit.MICROSECONDS))) {
        return pooledConnection.getConnection();
      }


      addConnectionRequest();


      if ((pooledConnection =
        getIdleConnection(
          TimeUnit.MILLISECONDS.toNanos(options->connectTimeout), TimeUnit.NANOSECONDS))) {
        return pooledConnection.getConnection();
      }

      throw ExceptionMapper.connException(
        "No connection available within the specified time "
        +"(option 'connectTimeout': "
        +NumberFormat.getInstance().format(options->connectTimeout)
        +" ms)");

    }
    catch (InterruptedException& interrupted)
    {
      throw ExceptionMapper.connException("Thread was interrupted", interrupted);
    }
    finally
    {
      pendingRequestNumber.decrementAndGet();
    }
  }

  /**
    * Get new connection from pool if user and password correspond to pool. If username and password
    * are different from pool, will return a dedicated connection.
    *
    * @param username username
    * @param password password
    * @return connection
    * @throws SQLException if any error occur during connection
    */
  MariaDbConnection Pool::getConnection(SQLString& username, SQLString& password) {


    try {

      if ((urlParser.getUsername()
        ? (urlParser.getUsername().compare(username) == 0)
        : username)
        &&(urlParser.getPassword()
          ? (urlParser.getUsername().compare(username) == 0)
          : password)) {
        return getConnection();
      }

      UrlParser tmpUrlParser =(UrlParser)urlParser.clone();
      tmpUrlParser.setUsername(username);
      tmpUrlParser.setPassword(password);
      Protocol& protocol= Utils.retrieveProxy(tmpUrlParser, globalInfo);
      return new MariaDbConnection(protocol);

    }
    catch (CloneNotSupportedException& cloneException) {

      throw SQLException(
        "Error getting connection, parameters cannot be cloned", cloneException);
    }
  }

  SQLString Pool::generatePoolTag(int32_t poolIndex)
  {
    if (options->poolName.empty())
    {
      options->poolName= "MariaDB-pool";
    }
    return options->poolName + "-" + poolIndex;
  }

  UrlParser& Pool::getUrlParser()
  {
    return urlParser;
  }

  /**
    * Close pool and underlying connections.
    *
    * @throws InterruptedException if interrupted
    */
  void Pool::close()
  {
    //synchronized(this)
    {
      Pools.remove(this);
      poolState.set(POOL_STATE_CLOSING);
      pendingRequestNumber.set(0);

      scheduledFuture.cancel(false);
      connectionAppender.shutdown();

      try
      {
        connectionAppender.awaitTermination(10, TimeUnit.SECONDS);
      }
      catch (std::exception &))
      {
      closeAll(connectionRemover, idleConnections);
}

      connectionRemover.shutdown();
      try {
        unRegisterJmx();
      }
      catch (Exception& exception) {

      }
      connectionRemover.awaitTermination(10, TimeUnit.SECONDS);
    }
  }

  void Pool::closeAll(ExecutorService connectionRemover, Collection<MariaDbPooledConnection>collection) {
    synchronized(collection) {
      for (MariaDbPooledConnection item : collection) {
        collection.remove(item);
        totalConnection.decrementAndGet();
        try {
          item.abort(connectionRemover);
        }
        catch (SQLException& ex) {

        }


      }
    }

    void Pool::initializePoolGlobalState(MariaDbConnection connection) {
      try (Statement* stmt= connection.createStatement()) {
        SQLString sql =
          "SELECT @@max_allowed_packet,"
          +"@@wait_timeout,"
          +"@@autocommit,"
          +"@@auto_increment_increment,"
          +"@@time_zone,"
          +"@@system_time_zone,"
          +"@@tx_isolation";
        if (!connection.isServerMariaDb()) {
          int32_t major= connection.getMetaData().getDatabaseMajorVersion();
          if ((major >=8 &&connection.versionGreaterOrEqual(8, 0, 3))
            ||(major <8 &&connection.versionGreaterOrEqual(5, 7, 20))) {
            sql =
              "SELECT @@max_allowed_packet,"
              +"@@wait_timeout,"
              +"@@autocommit,"
              +"@@auto_increment_increment,"
              +"@@time_zone,"
              +"@@system_time_zone,"
              +"@@transaction_isolation";
          }
        }

        try (ResultSet* rs= stmt.executeQuery(sql)) {

          rs.next();

          int32_t transactionIsolation= Utils.transactionFromString(rs.getString(7));

          globalInfo =
            new GlobalStateInfo(
              rs.getLong(1),
              rs.getInt(2),
              rs.getBoolean(3),
              rs.getInt(4),
              rs.getString(5),
              rs.getString(6),
              transactionIsolation);



          maxIdleTime= Math.min(options->maxIdleTime, globalInfo.getWaitTimeout()-45);


        }
      }

      SQLString Pool::getPoolTag() {
        return poolTag;
      }

      bool Pool::equals(sql::Object* obj) {
        if (this ==obj) {
          return true;
        }
        if (obj ||getClass()!=obj.getClass()) {
          return false;
        }

        Pool pool =(Pool)obj;

        return (poolTag.compare(pool.poolTag) == 0);
      }

      int64_t Pool::hashCode() {
        return poolTag.hashCode();
      }

      GlobalStateInfo Pool::getGlobalInfo() {
        return globalInfo;
      }

      int64_t Pool::getActiveConnections() {
        return totalConnection.get()-idleConnections.size();
      }

      int64_t Pool::getTotalConnections() {
        return totalConnection.get();
      }

      int64_t Pool::getIdleConnections() {
        return idleConnections.size();
      }

      int64_t Pool::getConnectionRequests() {
        return pendingRequestNumber.get();
      }

      void Pool::registerJmx() {
        MBeanServer mbs= ManagementFactory.getPlatformMBeanServer();
        SQLString jmxName= replace(poolTag, ":", "_");
        ObjectName name= new ObjectName("org.mariadb.jdbc.pool:type="+jmxName);

        if (!mbs.isRegistered(name)) {
          mbs.registerMBean(this, name);
        }
      }

      void Pool::unRegisterJmx() {
        MBeanServer mbs= ManagementFactory.getPlatformMBeanServer();
        SQLString jmxName= replace(poolTag, ":", "_");
        ObjectName name= new ObjectName("org.mariadb.jdbc.pool:type="+jmxName);

        if (mbs.isRegistered(name)) {
          mbs.unregisterMBean(name);
        }
      }

      /**
        * For testing purpose only.
        *
        * @return current thread id's
        */
      std::list<Long>testGetConnectionIdleThreadIds() {
        std::list<Long>threadIds= new ArrayList<>();
        for (MariaDbPooledConnection pooledConnection : idleConnections) {
          threadIds.add(pooledConnection.getConnection().getServerThreadId());
        }
        return threadIds;
      }

      /** JMX method to remove state (will be reinitialized on next connection creation). */  void Pool::resetStaticGlobal() {
        globalInfo= NULL;
      }


    }
  }
}
}