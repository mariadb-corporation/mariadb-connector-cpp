// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2021 MariaDB Corporation Ab

#ifndef _CONNECTIONEVENTLISTENER_H_
#define _CONNECTIONEVENTLISTENER_H_

#include <functional>
//#include "MariaDbInnerPoolConnection.h"

namespace sql
{
namespace mariadb
{
  class MariaDbPoolConnection;
}
class ConnectionEvent {
  mariadb::MariaDbPoolConnection &connection;

public:
  ConnectionEvent(mariadb::MariaDbPoolConnection &source) : connection(source) {}
  mariadb::MariaDbPoolConnection& getSource() {return connection;}
};

class ConnectionEventListener {

public:
  virtual ~ConnectionEventListener() {}
  virtual void connectionClosed(ConnectionEvent& event)= 0;
  virtual void connectionErrorOccurred(ConnectionEvent& event)= 0;
};

namespace mariadb
{
class MariaDbConnectionEventListener : public ConnectionEventListener {
  std::function<void(ConnectionEvent&)> connClosed;
  std::function<void(ConnectionEvent&)> connErrorOccurred;
public:
  MariaDbConnectionEventListener(std::function<void(ConnectionEvent&)> _connClosed, std::function<void(ConnectionEvent&)> _connErrorOccurred) :
    connClosed(_connClosed), connErrorOccurred(_connErrorOccurred) {}
  virtual ~MariaDbConnectionEventListener() {}

  void connectionClosed(ConnectionEvent& event);
  void connectionErrorOccurred(ConnectionEvent& event);
};

}
}
#endif
