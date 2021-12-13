// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2021 MariaDB Corporation Ab

#ifndef _MARIADBINNERPOOLCONNECTION_H_
#define _MARIADBINNERPOOLCONNECTION_H_

#include "MariaDbPoolConnection.h"

namespace sql
{
namespace mariadb
{
class MariaDbInnerPoolConnection  : public MariaDbPoolConnection {
  std::atomic<std::chrono::time_point<std::chrono::steady_clock>> lastUsed;

public:
  MariaDbInnerPoolConnection(MariaDbConnection* connection);
  void close();
  std::chrono::time_point<std::chrono::steady_clock> getLastUsed();
  void lastUsedToNow();
  void ensureValidation();
  };
}
}
#endif
