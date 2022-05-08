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


#include "CmdInformationBatch.h"

#include "SelectResultSet.h"

namespace sql
{
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
  CmdInformationBatch::CmdInformationBatch(std::size_t _expectedSize, int32_t _autoIncrement)
    : expectedSize(_expectedSize)
    , autoIncrement(_autoIncrement)
    , rewritten(false)
    , insertIdNumber(0)
    , hasException(false)
  {
  }


  void CmdInformationBatch::addErrorStat()
  {
    hasException= true;
    updateCounts.push_back(static_cast<int64_t>(Statement::EXECUTE_FAILED));
  }


  void CmdInformationBatch::reset()
  {
    insertIds.clear();
    updateCounts.clear();
    insertIdNumber= 0;
    hasException= false;
    rewritten= false;
  }


  void CmdInformationBatch::addResultSetStat()
  {
    this->updateCounts.push_back(static_cast<int64_t>(RESULT_SET_VALUE));
  }


  void CmdInformationBatch::addSuccessStat(int64_t updateCount, int64_t insertId)
  {
    insertIds.push_back(insertId);
    insertIdNumber+= updateCount;
    updateCounts.push_back(updateCount);
  }


  std::vector<int32_t>& CmdInformationBatch::getUpdateCounts()
  {
    batchRes.clear();
    if (rewritten) {
      
      int32_t resultValue;

      if (hasException) {
        resultValue= Statement::EXECUTE_FAILED;
      }
      else if (expectedSize == 1) {
        resultValue= static_cast<int32_t>(updateCounts.front());
      }
      else {
        resultValue= 0;
        for (auto updCnt : updateCounts) {
          if (updCnt != 0) {
            resultValue= Statement::SUCCESS_NO_INFO;
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
      batchRes.push_back(Statement::EXECUTE_FAILED);
      ++pos;
    }

    return batchRes;
  }


  std::vector<int32_t>& CmdInformationBatch::getServerUpdateCounts()
  {
    batchRes.clear();
    batchRes.reserve(updateCounts.size());

    for (auto& updCnt : updateCounts) {
      batchRes.push_back(static_cast<int32_t>(updCnt));
    }
    return batchRes;
  }

  std::vector<int64_t>& CmdInformationBatch::getLargeUpdateCounts()
  {
    largeBatchRes.clear();
    if (rewritten) {

      int64_t resultValue;
      if (hasException) {
        resultValue= Statement::EXECUTE_FAILED;
      }
      else if (expectedSize == 1) {
        resultValue= updateCounts.front();
      }
      else {
        resultValue= 0;

        for (auto& updCnt : updateCounts) {
          if (updCnt != 0) {
            resultValue= Statement::SUCCESS_NO_INFO;
          }
        }
      }

      largeBatchRes.resize(expectedSize, resultValue);

      return largeBatchRes;
    }

    largeBatchRes.reserve(std::max(updateCounts.size(), expectedSize));

    size_t pos= updateCounts.size();
    for (auto& updCnt : updateCounts) {
      largeBatchRes.push_back(updCnt);
    }

    while (pos < expectedSize) {
      largeBatchRes.push_back(Statement::EXECUTE_FAILED);
      ++pos;
    }

    return largeBatchRes;
  }


  int32_t CmdInformationBatch::getUpdateCount()
  {
    return (updateCounts.size() == 0 ? -1 : static_cast<int32_t>(updateCounts.front()));
  }


  int64_t CmdInformationBatch::getLargeUpdateCount()
  {
    return (updateCounts.size() == 0 ? -1 : updateCounts.front());
  }


  ResultSet* CmdInformationBatch::getBatchGeneratedKeys(Protocol* protocol)
  {
    std::vector<int64_t> ret;
    int32_t position= 0;
    int64_t insertId;
    auto idIterator= insertIds.begin();

    ret.reserve(static_cast<std::size_t>(insertIdNumber));

    for (int64_t updateCountLong : updateCounts) {
      int32_t updateCount= static_cast<int32_t>(updateCountLong);
      if (updateCount != Statement::EXECUTE_FAILED
        && updateCount != RESULT_SET_VALUE
        && (insertId= *idIterator) > 0) {
        for (int64_t i= 0; i < updateCount; i++) {
          ret[position++]= insertId + i*autoIncrement;
        }
      }
      ++idIterator;
    }
    return SelectResultSet::createGeneratedData(ret, protocol, true);
  }

  /**
    * Return GeneratedKeys containing insert ids. Insert ids are calculated using autoincrement
    * value.
    *
    * @param protocol current protocol
    * @param sql SQL command
    * @return a resultSet with insert ids.
    */
  ResultSet* CmdInformationBatch::getGeneratedKeys(Protocol* protocol, const SQLString& sql)
  {
    std::vector<int64_t> ret;
    int32_t position= 0;
    int64_t insertId;
    auto idIterator= insertIds.begin();

    ret.reserve(static_cast<size_t>(insertIdNumber));

    for (int64_t updateCountLong : updateCounts) {
      int32_t updateCount= static_cast<int32_t>(updateCountLong);
      if (updateCount != Statement::EXECUTE_FAILED
        && updateCount != RESULT_SET_VALUE
        && (insertId= *idIterator) > 0) {
        for (int32_t i= 0; i < updateCount; i++) {
          ret[position++]= insertId + i*autoIncrement;
        }
      }
      ++idIterator;
    }
    return SelectResultSet::createGeneratedData(ret, protocol, true);
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
}
}
