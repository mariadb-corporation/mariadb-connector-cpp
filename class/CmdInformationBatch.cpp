/************************************************************************************
   Copyright (C) 2023 MariaDB Corporation AB

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


#include "CmdInformationBatch.h"
#include "ResultSet.h"
#include "PreparedStatement.h"
#include "ResultSetMetaData.h"
#include "Results.h"

namespace mariadb
{

  /**
    * CmdInformationBatch is similar to CmdInformationMultiple, but knowing it's for batch, doesn't
    * take take of moreResult. That permit to use ConcurrentLinkedQueue, and then when option
    * "useBatchMultiSend" is set and batch is interrupted, will permit to reading thread to keep
    * connection in a correct state without any ConcurrentModificationException.
    *
    * @param expectedSize expected batch size.
    * @param autoIncrement connection auto increment value.
    */
  CmdInformationBatch::CmdInformationBatch(std::size_t _expectedSize)
    : expectedSize(_expectedSize)
    , rewritten(false)
    , insertIdNumber(0)
    , hasException(false)
  {
  }


  CmdInformationBatch::~CmdInformationBatch()
  {
  }

  void CmdInformationBatch::addErrorStat()
  {
    hasException= true;
    updateCounts.push_back(static_cast<int64_t>(PreparedStatement::EXECUTE_FAILED));
  }


  void CmdInformationBatch::reset()
  {
    updateCounts.clear();
    insertIdNumber= 0;
    hasException= false;
    rewritten= false;
  }


  void CmdInformationBatch::addResultSetStat()
  {
    this->updateCounts.push_back(static_cast<int64_t>(RESULT_SET_VALUE));
  }


  void CmdInformationBatch::addSuccessStat(int64_t updateCount)
  {
    insertIdNumber+= updateCount;
    updateCounts.push_back(updateCount);
  }


  std::vector<int64_t>& CmdInformationBatch::getUpdateCounts()
  {
    batchRes.clear();
    if (rewritten) {
      
      int32_t resultValue;

      if (hasException) {
        resultValue= PreparedStatement::EXECUTE_FAILED;
      }
      else if (expectedSize == 1) {
        resultValue= static_cast<int32_t>(updateCounts.front());
      }
      else {
        resultValue= 0;
        for (auto updCnt : updateCounts) {
          if (updCnt != 0) {
            resultValue= PreparedStatement::SUCCESS_NO_INFO;
          }
        }
      }
      batchRes.resize(expectedSize, resultValue);
      return batchRes;
    }

    batchRes.reserve(std::max(updateCounts.size(), expectedSize));

    size_t pos= updateCounts.size();

    for (auto& updCnt : updateCounts) {
      batchRes.push_back(static_cast<int32_t>(updCnt));
    }

    while (pos < expectedSize) {
      batchRes.push_back(PreparedStatement::EXECUTE_FAILED);
      ++pos;
    }

    return batchRes;
  }


  std::vector<int64_t>& CmdInformationBatch::getServerUpdateCounts()
  {
    batchRes.clear();
    batchRes.reserve(updateCounts.size());

    for (auto& updCnt : updateCounts) {
      batchRes.push_back(static_cast<int32_t>(updCnt));
    }
    return batchRes;
  }


  int64_t CmdInformationBatch::getUpdateCount()
  {
    return (updateCounts.size() == 0 ? -1 : static_cast<int32_t>(updateCounts.front()));
  }


  int32_t CmdInformationBatch::getCurrentStatNumber()
  {
    return static_cast<int32_t>(updateCounts.size());
  }


  bool CmdInformationBatch::moreResults()
  {
    return false;
  }


  bool CmdInformationBatch::isCurrentUpdateCount()
  {
    return false;
  }


  void CmdInformationBatch::setRewrite(bool rewritten)
  {
    this->rewritten= rewritten;
  }
} // namespace mariadb
