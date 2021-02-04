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

#include "Consts.h"
#include "Connection.hpp"
#include "GlobalStateInfo.h"

namespace sql
{
namespace mariadb
{
  /** Default value. ! To be used for Connection that will only Kill query/connection ! */
  GlobalStateInfo::GlobalStateInfo() :
    maxAllowedPacket(1000000),
    waitTimeout(28800),
    autocommit(true),
    autoIncrementIncrement(1),
    timeZone("+00:00"),
    systemTimeZone("+00:00"),
    defaultTransactionIsolation(sql::TRANSACTION_REPEATABLE_READ)
  {
  }
  /**
    * Storing global server state to avoid asking server each new connection. Using this Object
    * meaning having set the option "staticGlobal". Application must not change any of the following
    * options.
    *
    * @param maxAllowedPacket max_allowed_packet global variable value
    * @param waitTimeout wait_timeout global variable value
    * @param autocommit auto_commit global variable value
    * @param autoIncrementIncrement auto_increment_increment global variable value
    * @param timeZone time_zone global variable value
    * @param systemTimeZone System global variable value
    * @param defaultTransactionIsolation tx_isolation variable value
    */
  GlobalStateInfo::GlobalStateInfo(
      int64_t maxAllowedPacket,
      int32_t waitTimeout,
      bool autocommit,
      int32_t autoIncrementIncrement, SQLString& timeZone, SQLString& systemTimeZone,
      int32_t defaultTransactionIsolation)
    : maxAllowedPacket(maxAllowedPacket),
      waitTimeout(waitTimeout),
      autocommit(autocommit),
      autoIncrementIncrement(autoIncrementIncrement),
      timeZone(timeZone),
      systemTimeZone(systemTimeZone),
      defaultTransactionIsolation(defaultTransactionIsolation)
  {
  }
  int64_t GlobalStateInfo::getMaxAllowedPacket() const{
    return maxAllowedPacket;
  }
  int32_t GlobalStateInfo::getWaitTimeout() const{
    return waitTimeout;
  }
  bool GlobalStateInfo::isAutocommit() const{
    return autocommit;
  }
  int32_t GlobalStateInfo::getAutoIncrementIncrement() const {
    return autoIncrementIncrement;
  }
  SQLString GlobalStateInfo::getTimeZone() const{
    return timeZone;
  }
  SQLString GlobalStateInfo::getSystemTimeZone() const {
    return systemTimeZone;
  }
  int32_t GlobalStateInfo::getDefaultTransactionIsolation() const{
    return defaultTransactionIsolation;
  }
}
}
