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


#include <mutex>
#include <random>
#include <thread>

#include "MasterProtocol.h"

#include "UrlParser.h"
#include "pool/GlobalStateInfo.h"
#include "failover/FailoverProxy.h"
#include "util/LogQueryTool.h"

namespace sql
{
namespace mariadb
{

  /**
   * Get a protocol instance.
   *
   * @param urlParser connection URL infos
   * @param globalInfo server global variables information
   * @param lock the lock for thread synchronisation
   */
  MasterProtocol::MasterProtocol(std::shared_ptr<UrlParser>& urlParser, GlobalStateInfo* globalInfo, Shared::mutex& lock)
    :super(urlParser, globalInfo, lock)
  {
  }

  /**
   * Get new instance.
   *
   * @param proxy proxy
   * @param urlParser url connection object
   * @return new instance
   */
  MasterProtocol* MasterProtocol::getNewProtocol(FailoverProxy* proxy, GlobalStateInfo* globalInfo, std::shared_ptr<UrlParser>& urlParser)
  {
    MasterProtocol* newProtocol= new MasterProtocol(urlParser, globalInfo, proxy->lock);
    newProtocol->setProxy(proxy);

    return newProtocol;
  }

  /**
   * loop until found the failed connection.
   *
   * @param listener current failover
   * @param globalInfo server global variables information
   * @param addresses list of HostAddress to loop
   * @param searchFilter search parameter
   * @throws SQLException if not found
   */
  void MasterProtocol::loop( Listener* listener, GlobalStateInfo& globalInfo, const std::vector<HostAddress>& addresses, SearchFilter* searchFilter)
  {
    std::shared_ptr<Protocol> protocol;
    std::list<HostAddress> loopAddresses;

    std::copy(addresses.begin(), addresses.end(), loopAddresses.begin());

    if (loopAddresses.empty()){
      resetHostList(listener, loopAddresses);
    }

    int32_t maxConnectionTry= listener->getRetriesAllDown();
    bool firstLoop= true;
    SQLException* lastQueryException= nullptr;

    while (!loopAddresses.empty() || /*((!searchFilter || !searchFilter->isFailoverLoop()) && */maxConnectionTry > 0)
    {
      protocol.reset(getNewProtocol(listener->getProxy(), &globalInfo, listener->getUrlParser()));

      if (listener->isExplicitClosed()){
        return;
      }
      maxConnectionTry--;

      try {
        HostAddress* host;
        auto it= loopAddresses.begin();
        if (it == loopAddresses.end()){
          std::copy(listener->getUrlParser()->getHostAddresses().begin(),
            listener->getUrlParser()->getHostAddresses().end(), loopAddresses.begin());
          it= loopAddresses.begin();
        }
        host= &*it;
        loopAddresses.pop_front();
        protocol->setHostAddress(*host);
        protocol->connect();

        if (listener->isExplicitClosed()){
          protocol->close();
          return;
        }
        listener->removeFromBlacklist(protocol->getHostAddress());
        listener->foundActiveMaster(protocol);
        return;

      }catch (SQLException& e){
        listener->addToBlacklist(protocol->getHostAddress());
        //TODO: unlikely this is correct
        lastQueryException= &e;
      }

      // if server has try to connect to all host, and master still fail
      // add all servers back to continue looping until maxConnectionTry is reached
      if (loopAddresses.empty() && /*(!searchFilter || !searchFilter->isFailoverLoop()) &&*/ maxConnectionTry > 0)
      {
        resetHostList(listener, loopAddresses);
        if (firstLoop){
          firstLoop= false;
        }else {
          try {
            // wait 250ms before looping through all connection another time
            std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(250));
          }catch (SQLException&){

          }
        }
      }
    }
    if (lastQueryException != nullptr){
      throw SQLException(
          "No active connection found for master : " + lastQueryException->getMessage(),
          lastQueryException->getSQLState(),
          lastQueryException->getErrorCode(),
          lastQueryException);
    }
    throw SQLException("No active connection found for master");
  }

  /**
   * Reinitialize loopAddresses with all hosts : all servers in randomize order without connected
   * host.
   *
   * @param listener current listener
   * @param loopAddresses the list to reinitialize
   */
  void MasterProtocol::resetHostList(Listener* listener, std::list<HostAddress>& loopAddresses)
  {
    static auto rnd = std::default_random_engine{};

    std::vector<HostAddress> servers(listener->getUrlParser()->getHostAddresses());

    std::shuffle(servers.begin(), servers.end(), rnd);

    loopAddresses.clear();
    std::copy(servers.begin(), servers.end(), loopAddresses.begin());
  }
}
}
