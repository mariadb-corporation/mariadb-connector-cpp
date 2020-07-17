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

#ifndef _GLOBALSTATEINFO_H_
#define _GLOBALSTATEINFO_H_
#include <stdint.h>
#include "StringImp.h"
namespace sql
{
namespace mariadb
{
class GlobalStateInfo
{
  private:  int64_t maxAllowedPacket;
  private:  int32_t waitTimeout;
  private:  bool autocommit;
  private:  int32_t autoIncrementIncrement;
  private:  SQLString timeZone;
  private:  SQLString systemTimeZone;
  private:  int32_t defaultTransactionIsolation;
  public:
    GlobalStateInfo();
    GlobalStateInfo(int64_t maxAllowedPacket, int32_t waitTimeout, bool autocommit,
      int32_t autoIncrementIncrement, SQLString& timeZone, SQLString& systemTimeZone,
      int32_t defaultTransactionIsolation);
  public:  int64_t getMaxAllowedPacket() const;
  public:  int32_t getWaitTimeout() const;
  public:  bool isAutocommit() const;
  public:  int32_t getAutoIncrementIncrement() const;
  public:  SQLString getTimeZone() const;
  public:  SQLString getSystemTimeZone() const;
  public:  int32_t getDefaultTransactionIsolation() const;
};
}
}
#endif