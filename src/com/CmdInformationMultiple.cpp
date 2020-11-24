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


#include "CmdInformationMultiple.h"

#include "SelectResultSet.h"

namespace sql
{
namespace mariadb
{

  /**
    * Object containing update / insert ids, optimized for only multiple result.
    *
    * @param expectedSize expected batch size.
    * @param autoIncrement connection auto increment value.
    */
  CmdInformationMultiple::CmdInformationMultiple(std::size_t _expectedSize, int32_t _autoIncrement)
    : expectedSize(_expectedSize)
    , autoIncrement(_autoIncrement)
    , moreResultsIdx(0)
    , insertIdNumber(0)
    , hasException(false)
    , rewritten(false)
  {
    insertIds.reserve(expectedSize);
    updateCounts.reserve(expectedSize);
  }

  void CmdInformationMultiple::addErrorStat()
  {
    hasException= true;
    updateCounts.push_back(static_cast<int64_t>(Statement::EXECUTE_FAILED));
  }

  void CmdInformationMultiple::reset()
  {
    insertIds.clear();
    updateCounts.clear();
    insertIdNumber= 0;
    moreResultsIdx= 0;
    hasException= false;
    rewritten= false;
  }

  void CmdInformationMultiple::addResultSetStat()
  {
    updateCounts.push_back(static_cast<int64_t>(RESULT_SET_VALUE));
  }

  void CmdInformationMultiple::addSuccessStat(int64_t updateCount, int64_t insertId)
  {
    insertIds.push_back(insertId);
    insertIdNumber+= updateCount;
    updateCounts.push_back(updateCount);
  }

  std::vector<int32_t>& CmdInformationMultiple::getServerUpdateCounts()
  {
    batchRes.clear();
    batchRes.reserve(updateCounts.size());

    auto iterator= updateCounts.begin();
    int32_t pos= 0;
    while (iterator!= updateCounts.end()) {
      batchRes[pos]= static_cast<int32_t>(*iterator);
      ++pos;
      ++iterator;
    }

    return batchRes;
  }

  std::vector<int32_t>& CmdInformationMultiple::getUpdateCounts()
  {
    batchRes.clear();
    if (rewritten) {
      batchRes.resize(expectedSize, hasException ? Statement::EXECUTE_FAILED : Statement::SUCCESS_NO_INFO);
      return batchRes;
    }
    batchRes.reserve(std::max(updateCounts.size(), expectedSize));

    auto iterator= updateCounts.begin();
    int32_t pos= 0;
    while (iterator!= updateCounts.end()) {
      batchRes[pos]= static_cast<int32_t>(*iterator);
      ++pos;
      ++iterator;
    }


    while (pos < expectedSize) {
      batchRes[pos++]= Statement::EXECUTE_FAILED;
    }

    return batchRes;
  }


  std::vector<int64_t>& CmdInformationMultiple::getLargeUpdateCounts()
  {
    largeBatchRes.clear();
    if (rewritten) {
      largeBatchRes.resize(expectedSize, hasException ? Statement::EXECUTE_FAILED : Statement::SUCCESS_NO_INFO);
      return largeBatchRes;
    }

    largeBatchRes.reserve(std::max(updateCounts.size(), expectedSize));

    auto iterator= updateCounts.begin();
    int32_t pos= 0;
    while (iterator != updateCounts.end()) {
      largeBatchRes[pos]= *iterator;
      ++pos;
      ++iterator;
    }

    while (pos < expectedSize) {
      largeBatchRes[pos++]= Statement::EXECUTE_FAILED;
    }

    return largeBatchRes;
  }


  int32_t CmdInformationMultiple::getUpdateCount()
  {
    if (static_cast<size_t>(moreResultsIdx) >= updateCounts.size()) {
      return -1;
    }
    return static_cast<int32_t>(updateCounts[moreResultsIdx]);
  }

  int64_t CmdInformationMultiple::getLargeUpdateCount()
  {
    if (static_cast<size_t>(moreResultsIdx) >= updateCounts.size()) {
      return -1;
    }
    return updateCounts[moreResultsIdx];
  }

  ResultSet* CmdInformationMultiple::getBatchGeneratedKeys(Protocol* protocol)
  {
    std::vector<int64_t> ret;
    int32_t position= 0;
    int64_t insertId;
    auto idIterator= insertIds.begin();

    ret.reserve(static_cast<std::size_t>(insertIdNumber));

    for (int64_t updateCount : updateCounts) {
      if (updateCount != Statement::EXECUTE_FAILED
        && updateCount != RESULT_SET_VALUE
        && (insertId= *idIterator)>0) {
        for (int32_t i= 0; i < updateCount; i++) {
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
  ResultSet* CmdInformationMultiple::getGeneratedKeys(Protocol* protocol, const SQLString& sql)
  {
    std::vector<int64_t> ret;
    int32_t position= 0;
    int64_t insertId;
    auto idIterator= insertIds.begin();
    auto updateIterator= updateCounts.begin();

    ret.reserve(static_cast<size_t>(insertIdNumber));

    for (int32_t element= 0; element <= moreResultsIdx; element++) {
      int64_t updateCount= *updateIterator;
      if (updateCount != Statement::EXECUTE_FAILED
        && updateCount != RESULT_SET_VALUE
        && (insertId= *idIterator)>0
        && element == moreResultsIdx) {
        for (int32_t i= 0; i <updateCount; i++) {
          ret[position++]= insertId + i*autoIncrement;
        }
      }
      ++idIterator;
    }
    return SelectResultSet::createGeneratedData(ret, protocol, true);
  }

  int32_t CmdInformationMultiple::getCurrentStatNumber()
  {
    return static_cast<int32_t>(updateCounts.size());
  }

  bool CmdInformationMultiple::moreResults()
  {
    return static_cast<size_t>(moreResultsIdx++) < (updateCounts.size() - 1)
      && updateCounts[moreResultsIdx] == RESULT_SET_VALUE;
  }

  bool CmdInformationMultiple::isCurrentUpdateCount()
  {
    return updateCounts[moreResultsIdx] != RESULT_SET_VALUE;
  }

  void CmdInformationMultiple::setRewrite(bool rewritten)
  {
    this->rewritten= rewritten;
  }
}
}
