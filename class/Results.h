/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

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

#ifndef _RESULTS_H_
#define _RESULTS_H_

#include <deque>

#include "SQLString.h"
#include "CmdInformation.h"
#include "pimpls.h"

namespace mariadb
{

class Results  {

  PreparedStatement* statement= nullptr;
  ServerPrepareResult* serverPrepResult= nullptr;
  int32_t fetchSize= 0;
  bool batch= false;
  std::size_t expectedSize= 1;
  Unique::CmdInformation cmdInformation;
  std::deque<Unique::ResultSet> executionResults;
  Unique::ResultSet currentRs;
  ResultSet* resultSet= nullptr;
  Unique::ResultSet callableResultSet;
  bool binaryFormat= false;
  int32_t resultSetScrollType;
  bool rewritten= false;
  SQLString sql;
  MYSQL_BIND* parameters;
  bool cachingLocally= false;

public:
  enum {
    EXECUTE_FAILED= -3,
    SUCCESS_NO_INFO= -2
  };

  Results();
  Results(
    ClientSidePreparedStatement* statement,
    int32_t fetchSize,
    bool batch,
    std::size_t expectedSize,
    bool binaryFormat,
    int32_t resultSetScrollType,
    const SQLString& sql,
    MYSQL_BIND* parameters);
  Results(
    ServerSidePreparedStatement* statement,
    int32_t fetchSize,
    bool batch,
    std::size_t expectedSize,
    bool binaryFormat,
    int32_t resultSetScrollType,
    const SQLString& sql,
    MYSQL_BIND* parameters);
  Results(
    PreparedStatement* statement,
    int32_t fetchSize,
    bool batch,
    std::size_t expectedSize,
    bool binaryFormat,
    int32_t resultSetScrollType,
    const SQLString& sql,
    MYSQL_BIND* parameters);
  ~Results();

  void addStats(int64_t updateCount, bool moreResultAvailable);
  void addStatsError(bool moreResultAvailable);
  int32_t getCurrentStatNumber();
  void addResultSet(ResultSet* resultSet,bool moreResultAvailable);
  CmdInformation* getCmdInformation();

protected:
  void setCmdInformation(CmdInformation* cmdInformation);

public:
  bool commandEnd();
  ResultSet* getResultSet();
  ResultSet* releaseResultSet();
  ResultSet* getCallableResultSet();
  void loadFully(bool skip, Protocol* guard);
  void abort();
  bool isFullyLoaded();
  bool getMoreResults(bool closeCurrent, Protocol *guard);
  bool hasMoreResults(Protocol *guard);
  int32_t getFetchSize();
  PreparedStatement* getStatement();
  bool isBatch();
  std::size_t getExpectedSize();
  bool isBinaryFormat();
  void removeFetchSize();
  int32_t getResultSetScrollType() const;
  const SQLString& getSql() const;
  MYSQL_BIND* getParameters();
  void close();
  bool isRewritten();
  void setRewritten(bool rewritten);
  void checkOut(ResultSet* iamleaving);
  bool nextIsLast(Protocol* protocol);
};

} // namespace mariadb
#endif
