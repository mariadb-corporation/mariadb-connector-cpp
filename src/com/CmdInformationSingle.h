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


#ifndef _CMDINFORMATIONSINGLE_H_
#define _CMDINFORMATIONSINGLE_H_

#include "Consts.h"

#include "CmdInformation.h"

namespace sql
{
namespace mariadb
{


class CmdInformationSingle  : public CmdInformation {

  const int64_t insertId;
  int32_t autoIncrement;
  int64_t updateCount;

public:
  CmdInformationSingle(int64_t insertId,int64_t updateCount,int32_t autoIncrement);
  std::vector<int32_t>& getUpdateCounts();
  std::vector<int32_t>& getServerUpdateCounts();
  std::vector<int64_t>& getLargeUpdateCounts();
  int32_t getUpdateCount();
  int64_t getLargeUpdateCount();
  void addErrorStat();
  void reset();
  void addResultSetStat();
  ResultSet* getGeneratedKeys(Protocol* protocol, const SQLString& sql);

private:
  bool isDuplicateKeyUpdate(const SQLString& sql);

public:
  ResultSet* getBatchGeneratedKeys(Protocol* protocol);
  int32_t getCurrentStatNumber();
  bool moreResults();
  bool isCurrentUpdateCount();
  void addSuccessStat(int64_t updateCount,int64_t insertId);
  void setRewrite(bool rewritten);
  };
}
}
#endif