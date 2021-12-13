// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2021 MariaDB Corporation Ab

#include "MariaDbInnerPoolConnection.h"
#include "ConnectionEventListener.h"

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
    lastUsed(std::chrono::steady_clock::now())
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
  std::chrono::time_point<std::chrono::steady_clock> MariaDbInnerPoolConnection::getLastUsed()
  {
    return lastUsed.load();
  }

  /** Set last poolConnection use to now. */
  void MariaDbInnerPoolConnection::lastUsedToNow()
  {
    // Probably not the best place, but does the job
    connection->markClosed(false);
    lastUsed.store(std::chrono::steady_clock::now());
  }

  void MariaDbInnerPoolConnection::ensureValidation()
  {
    lastUsed.store(std::chrono::steady_clock::time_point::min());
  }
}
}
