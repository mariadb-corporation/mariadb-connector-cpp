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


#include <chrono>

#include "MariaDbPooledConnection.h"

namespace sql
{
namespace mariadb
{

  /**
    * Constructor.
    *
    * @param connection connection to retrieve connection options
    */
  MariaDbPooledConnection::MariaDbPooledConnection(MariaDbConnection* connection)
    : connection(connection)
  {
    connection->pooledConnection.reset(this);
    lastUsedToNow();
  }

  /**
    * Creates and returns a <code>Connection</code> object that is a handle for the physical
    * connection that this <code>PooledConnection</code> object represents. The connection pool
    * manager calls this method when an application has called the method <code>
    * DataSource.getConnection</code> and there are no <code>PooledConnection</code> objects
    * available. See the {@link PooledConnection interface description} for more information.
    *
    * @return a <code>Connection</code> object that is a handle to this <code>PooledConnection</code>
    *     object
    */
  MariaDbConnection* MariaDbPooledConnection::getConnection()
  {
    return connection;
  }

  /**
    * Closes the physical connection that this <code>PooledConnection</code> object represents. An
    * application never calls this method directly; it is called by the connection pool module, or
    * manager. <br>
    * See the {@link PooledConnection interface description} for more information.
    *
    * @throws SQLException if a database access error occurs
    */
  void MariaDbPooledConnection::close()
  {
    connection->pooledConnection.reset();
    connection->close();
  }

  /**
    * Abort connection.
    *
    * @param executor executor
    * @throws SQLException if a database access error occurs
    */
  void MariaDbPooledConnection::abort(sql::Executor* executor)
  {
    connection->pooledConnection.reset();
    connection->abort(executor);
  }

  /**
    * Registers the given event failover so that it will be notified when an event occurs on this
    * <code>PooledConnection</code> object.
    *
    * @param listener a component, usually the connection pool manager, that has implemented the
    *     <code>ConnectionEventListener</code> interface and wants to be notified when the connection
    *     is closed or has an error
    * @see #removeConnectionEventListener
    */
  void MariaDbPooledConnection::addConnectionEventListener(ConnectionEventListener& listener)
  {
    connectionEventListeners.push_back(&listener);
  }

  /**
    * Removes the given event failover from the list of components that will be notified when an
    * event occurs on this <code>PooledConnection</code> object.
    *
    * @param listener a component, usually the connection pool manager, that has implemented the
    *     <code>ConnectionEventListener</code> interface and been registered with this <code>
    *     PooledConnection</code> object as a failover
    * @see #addConnectionEventListener
    */
  void MariaDbPooledConnection::removeConnectionEventListener(ConnectionEventListener& listener)
  {
    //connectionEventListeners.erase(&listener);
  }

  /**
    * Registers a <code>StatementEventListener</code> with this <code>PooledConnection</code> object.
    * Components that wish to be notified when <code>PreparedStatement</code>s created by the
    * connection are closed or are detected to be invalid may use this method to register a <code>
    * StatementEventListener</code> with this <code>PooledConnection</code> object. <br>
    *
    * @param listener an component which implements the <code>StatementEventListener</code> interface
    *     that is to be registered with this <code>PooledConnection</code> object <br>
    */
  void MariaDbPooledConnection::addStatementEventListener(StatementEventListener& listener)
  {
    statementEventListeners.push_back(&listener);
  }

  /**
    * Removes the specified <code>StatementEventListener</code> from the list of components that will
    * be notified when the driver detects that a <code>PreparedStatement</code> has been closed or is
    * invalid. <br>
    *
    * @param listener the component which implements the <code>StatementEventListener</code>
    *     interface that was previously registered with this <code>PooledConnection</code> object
    *     <br>
    */
  void MariaDbPooledConnection::removeStatementEventListener(StatementEventListener& listener)
  {
    //statementEventListeners.erase(listener);
  }

  /**
    * Fire statement close event to listeners.
    *
    * @param st statement
    */
  void MariaDbPooledConnection::fireStatementClosed(Statement* st)
  {
    if (INSTANCEOF(st, PreparedStatement*)) {
      /*StatementEvent* event= new StatementEvent(this, st);
      for (StatementEventListener* listener : statementEventListeners) {
        listener->statementClosed(event);
      }*/
    }
  }

  /**
    * Fire statement error to listeners.
    *
    * @param st statement
    * @param ex exception
    */
  void MariaDbPooledConnection::fireStatementErrorOccured(Statement* st, MariaDBExceptionThrower& ex)
  {
    if (INSTANCEOF(st, PreparedStatement*)) {
      /*StatementEvent* event= new StatementEvent(this, st, ex);
      for (StatementEventListener* listener : statementEventListeners) {
        listener->statementErrorOccurred(event);
      }*/
    }
  }

  /** Fire Connection close to listening listeners. */
  void MariaDbPooledConnection::fireConnectionClosed()
  {
    /*ConnectionEvent* event= new ConnectionEvent(this);
    for (ConnectionEventListener* listener : connectionEventListeners) {
      listener->connectionClosed(event);
    }*/
  }

  /**
    * Fire connection error to listening listeners.
    *
    * @param ex exception
    */
  void MariaDbPooledConnection::fireConnectionErrorOccured(SQLException ex)
  {
    /*ConnectionEvent* event= new ConnectionEvent(this, ex);
    for (ConnectionEventListener* listener : connectionEventListeners) {
      listener->connectionErrorOccurred(event);
    }*/
  }

  /**
    * Indicate if there are any registered listener.
    *
    * @return true if no listener.
    */
  bool MariaDbPooledConnection::noStmtEventListeners()
  {
    return statementEventListeners.empty();
  }

  /**
    * Indicate last time this pool connection has been used.
    *
    * @return current last used time (nano).
    */
  int64_t MariaDbPooledConnection::getLastUsed()
  {
    return lastUsed.load();
  }

  /** Set last poolConnection use to now. */
  void MariaDbPooledConnection::lastUsedToNow()
  {
    auto now= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
    lastUsed.store(now.count());
  }
}
}
