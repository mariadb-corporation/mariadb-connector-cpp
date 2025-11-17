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


#ifndef _CMDINFORMATIONMULTIPLE_H_
#define _CMDINFORMATIONMULTIPLE_H_

#include "CmdInformation.h"


namespace mariadb
{

class CmdInformationMultiple  : public CmdInformation
{
  std::vector<int64_t>updateCounts;
  std::size_t expectedSize;
  uint32_t moreResultsIdx;
  bool hasException;
  bool rewritten;

public:
  CmdInformationMultiple(std::size_t expectedSize);
  ~CmdInformationMultiple();

  void addErrorStat();
  void reset();
  void addResultSetStat();
  void addSuccessStat(int64_t updateCount);
  std::vector<int64_t>& getServerUpdateCounts();
  std::vector<int64_t>& getUpdateCounts();
  int64_t getUpdateCount();
  int32_t getCurrentStatNumber();
  bool moreResults();
  uint32_t hasMoreResults();
  bool isCurrentUpdateCount();
  void setRewrite(bool rewritten);
};

}
#endif
