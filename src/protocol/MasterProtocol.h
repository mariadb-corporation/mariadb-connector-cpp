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


#ifndef _MASTERPROTOCOL_H_
#define _MASTERPROTOCOL_H_

#include <vector>
#include <list>

#include "Consts.h"

#include "protocol/capi/QueryProtocol.h"
#include "HostAddress.h"

namespace sql
{
namespace mariadb
{
class Listener;
class SearchFilter;

class MasterProtocol  : public capi::QueryProtocol
{
  typedef capi::QueryProtocol super;

  static void resetHostList(Listener* listener, std::list<HostAddress>& loopAddresses);
  static MasterProtocol* getNewProtocol(FailoverProxy* proxy, GlobalStateInfo* globalInfo, std::shared_ptr<UrlParser>& urlParser);

public:
  MasterProtocol(std::shared_ptr<UrlParser>& urlParser, GlobalStateInfo* globalInfo, Shared::mutex& lock);
  static void loop(Listener* listener, GlobalStateInfo& globalInfo, const std::vector<HostAddress>& addresses, SearchFilter* searchFilter);
  ~MasterProtocol() {}
};
}
}
#endif
