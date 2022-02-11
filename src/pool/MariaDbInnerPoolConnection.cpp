// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2021 MariaDB Corporation Ab

#include "MariaDbInnerPoolConnection.h"
#include "ConnectionEventListener.h"

using namespace std::chrono;
namespace sql
{
namespace mariadb
{

  /**
   * Constructor.
   *
   * @param connection connection to retrieve connection options
   */
  MariaDbInnerPoolConnection::MariaDbInnerPoolConnection(MariaDbConnection* connection) :
    MariaDbPoolConnection(connection),
    lastUsed(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())
  {
    
  }

  void MariaDbInnerPoolConnection::close()
  {
    fireConnectionClosed(new ConnectionEvent(*this));
  }

  /**
   * Indicate last time this pool connection has been used.
   *
   * @return current last used time (nano).
   */
  int64_t MariaDbInnerPoolConnection::getLastUsed()
  {
    return lastUsed.load();
  }

  /** Set last poolConnection use to now. */
  void MariaDbInnerPoolConnection::lastUsedToNow()
  {
    // Probably not the best place, but does the job
    connection->markClosed(false);
    lastUsed.store(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
  }

  void MariaDbInnerPoolConnection::ensureValidation()
  {
    lastUsed.store(0);
  }
}
}
