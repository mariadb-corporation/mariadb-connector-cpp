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
  CmdInformationBatch::CmdInformationBatch(int32_t _expectedSize, int32_t _autoIncrement)
    : expectedSize(_expectedSize)
    , autoIncrement(_autoIncrement)
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

  sql::Ints* CmdInformationBatch::getUpdateCounts()
  {
    if (rewritten) {
      sql::Ints* ret=  nullptr;
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
      ret= new sql::Ints(expectedSize, resultValue);
      return ret;
    }

    sql::Ints* ret= new sql::Ints(std::max(updateCounts.size(), static_cast<size_t>(expectedSize)));

    size_t pos= 0;

    for (auto& updCnt : updateCounts) {
      ret[pos++]= updCnt;
    }


    while (pos < ret->length) {
      ret[pos++]= Statement::EXECUTE_FAILED;
    }

    return ret;
  }

  sql::Ints* CmdInformationBatch::getServerUpdateCounts()
  {
    sql::Ints* ret= new sql::Ints(updateCounts.size());
    size_t pos= 0;

    for (auto& updCnt : updateCounts) {
      ret[pos++]= static_cast<int32_t>(updCnt);
    }
    return ret;
  }

  sql::Longs* CmdInformationBatch::getLargeUpdateCounts()
  {
    if (rewritten) {
      sql::Longs* ret;

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

      ret= new sql::Longs(expectedSize, resultValue);

      return ret;
    }

    sql::Longs* ret= new sql::Longs(std::max(updateCounts.size(), static_cast<size_t>(expectedSize)));

    ret->assign(updateCounts.data(), updateCounts.size());

    if (updateCounts.size() < static_cast<size_t>(expectedSize))
    {
      size_t pos= updateCounts.size();
      while (pos < ret->size()) {
        ret[pos++]= Statement::EXECUTE_FAILED;
      }
    }

    return ret;
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
    return updateCounts.size();
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
