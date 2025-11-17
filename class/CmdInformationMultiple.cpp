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


#include "CmdInformationMultiple.h"
#include "ResultSet.h"
#include "interface/PreparedStatement.h"
#include "ResultSetMetaData.h"
#include "Results.h"
#include <algorithm>

namespace mariadb
{
  /**
    * Object containing update / insert ids, optimized for only multiple result.
    *
    * @param expectedSize expected batch size.
    * @param autoIncrement connection auto increment value.
    */
  CmdInformationMultiple::CmdInformationMultiple(std::size_t _expectedSize)
    : expectedSize(_expectedSize)
    , moreResultsIdx(0)
    , hasException(false)
    , rewritten(false)
  {
    updateCounts.reserve(expectedSize > 4U ? expectedSize : 4U); // let's assume SP is RS, outparams and result code + 1 reserve is kinda normal
  }


  CmdInformationMultiple::~CmdInformationMultiple()
  {}


  void CmdInformationMultiple::addErrorStat()
  {
    hasException= true;
    updateCounts.push_back(static_cast<int64_t>(PreparedStatement::EXECUTE_FAILED));
  }

  void CmdInformationMultiple::reset()
  {
    updateCounts.clear();
    moreResultsIdx= 0;
    hasException= false;
    rewritten= false;
  }

  void CmdInformationMultiple::addResultSetStat()
  {
    updateCounts.push_back(static_cast<int64_t>(RESULT_SET_VALUE));
  }

  void CmdInformationMultiple::addSuccessStat(int64_t updateCount)
  {
    updateCounts.push_back(updateCount);
  }

  std::vector<int64_t>& CmdInformationMultiple::getServerUpdateCounts()
  {
    batchRes.clear();
    batchRes.reserve(updateCounts.size());

    auto iterator= updateCounts.begin();
    int32_t pos= 0;
    while (iterator != updateCounts.end()) {
      batchRes[pos]= static_cast<int64_t>(*iterator);
      ++pos;
      ++iterator;
    }

    return batchRes;
  }

  std::vector<int64_t>& CmdInformationMultiple::getUpdateCounts()
  {
    batchRes.clear();
    if (rewritten) {
      batchRes.resize(expectedSize, hasException ? PreparedStatement::EXECUTE_FAILED : PreparedStatement::SUCCESS_NO_INFO);
      return batchRes;
    }
    batchRes.reserve(std::max(updateCounts.size(), expectedSize));

    auto iterator= updateCounts.begin();
    std::size_t pos= 0;
    while (iterator!= updateCounts.end()) {
      batchRes[pos]= static_cast<int64_t>(*iterator);
      ++pos;
      ++iterator;
    }

    while (pos < expectedSize) {
      batchRes[pos++]= PreparedStatement::EXECUTE_FAILED;
    }

    return batchRes;
  }


  int64_t CmdInformationMultiple::getUpdateCount()
  {
    if (static_cast<std::size_t>(moreResultsIdx) >= updateCounts.size()) {
      return -1;
    }
    return static_cast<int64_t>(updateCounts[moreResultsIdx]);
  }


  int32_t CmdInformationMultiple::getCurrentStatNumber()
  {
    return static_cast<int32_t>(updateCounts.size());
  }

  bool CmdInformationMultiple::moreResults()
  {
    return static_cast<std::size_t>(++moreResultsIdx) < updateCounts.size();
  //    && updateCounts[moreResultsIdx] == RESULT_SET_VALUE;
  }

  uint32_t CmdInformationMultiple::hasMoreResults()
  {
    uint32_t size= static_cast<uint32_t>(updateCounts.size());
    return size > moreResultsIdx ? size - moreResultsIdx - 1 : 0U;
  }
  bool CmdInformationMultiple::isCurrentUpdateCount()
  {
    return updateCounts[moreResultsIdx] != RESULT_SET_VALUE;
  }

  void CmdInformationMultiple::setRewrite(bool rewritten)
  {
    this->rewritten= rewritten;
  }
} // namespace mariadb
