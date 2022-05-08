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


#ifndef _HOSTADDRESS_H_
#define _HOSTADDRESS_H_

#include <vector>
#include <memory>

#include "StringImp.h"
#include "logger/Logger.h"
#include "Consts.h"

namespace sql
{
namespace mariadb
{
  constexpr int32_t DefaultPort = 3306;

  class HostAddress
  {
    static Shared::Logger logger; /* const? */

  public:
    SQLString host;
    int32_t port;
    SQLString type;
  private:
    HostAddress();
  public:
    HostAddress(const SQLString& host, int32_t port= DefaultPort);
    HostAddress(const SQLString& host, int32_t port, const SQLString& type);
    static std::vector<HostAddress> parse(const SQLString& spec, enum HaMode haMode);
  private:
    static std::unique_ptr<HostAddress> parseSimpleHostAddress(const SQLString& str);
    static int32_t getPort(const SQLString& portString);
    static std::unique_ptr<HostAddress> parseParameterHostAddress(SQLString& str);
  public:
    static SQLString toString(std::vector<HostAddress> addrs);
#ifdef WEVE_FIGURED_OUT_WE_NEED_IT
    static SQLString toString(HostAddress* addrs);
#endif
    SQLString toString() const;
    bool equals(HostAddress* obj);
    int64_t hashCode();
  };
}
}
#endif
