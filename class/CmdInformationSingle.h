/************************************************************************************
   Copyright (C) 2022, 2025 MariaDB Corporation plc

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

#include "CmdInformation.h"

namespace mariadb
{

class CmdInformationSingle  : public CmdInformation
{
  int64_t updateCount;

public:
  CmdInformationSingle(int64_t updateCount);
  ~CmdInformationSingle();

  std::vector<int64_t>& getUpdateCounts() override;
  std::vector<int64_t>& getServerUpdateCounts() override;
  int64_t getUpdateCount() override;
  void addErrorStat() override;
  void reset() override;
  void addResultSetStat() override;
  int32_t getCurrentStatNumber() override;
  bool moreResults() override;
  inline uint32_t hasMoreResults() override { return 0U; }
  bool isCurrentUpdateCount() override;
  void addSuccessStat(int64_t updateCount) override;
  void setRewrite(bool rewritten) override;
};

}
#endif
