// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2021 MariaDB Corporation Ab


#include "ConnectionEventListener.h"
#include "MariaDbInnerPoolConnection.h"

namespace sql
{
namespace mariadb
{
  void MariaDbConnectionEventListener::connectionClosed(ConnectionEvent& event)
  {
    connClosed(event);
  }


  void MariaDbConnectionEventListener::connectionErrorOccurred(ConnectionEvent& event)
  {
    connErrorOccurred(event);
  }

}
}

