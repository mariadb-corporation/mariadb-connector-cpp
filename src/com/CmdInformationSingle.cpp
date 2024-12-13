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


#include "CmdInformationSingle.h"

#include "SelectResultSet.h"

namespace sql
{
namespace mariadb
{
  /**
    * Object containing update / insert ids, optimized for only one result.
    *
    * @param insertId auto generated id.
    * @param updateCount update count
    * @param autoIncrement connection auto increment value.
    */
  CmdInformationSingle::CmdInformationSingle(int64_t _insertId, int64_t _updateCount, int32_t _autoIncrement)
    : insertId(_insertId)
    , autoIncrement(_autoIncrement)
    , updateCount(_updateCount)
  {
  }

  std::vector<int32_t>& CmdInformationSingle::getUpdateCounts()
  {
    batchRes[0]= static_cast<int32_t>(updateCount);
    return batchRes;
  }

  std::vector<int32_t>& CmdInformationSingle::getServerUpdateCounts()
  {
    batchRes[0]= static_cast<int32_t>(updateCount);
    return batchRes;
  }

  std::vector<int64_t>& CmdInformationSingle::getLargeUpdateCounts()
  {
    largeBatchRes[0]= updateCount;
    return largeBatchRes;
  }

  int32_t CmdInformationSingle::getUpdateCount()
  {
    return static_cast<int32_t>(updateCount);
  }

  int64_t CmdInformationSingle::getLargeUpdateCount()
  {
    return updateCount;
  }

  void CmdInformationSingle::addErrorStat()
  {
  }

  void CmdInformationSingle::reset()
  {
  }

  void CmdInformationSingle::addResultSetStat()
  {
  }

  /**
    * Get generated Keys.
    *
    * @param protocol current protocol
    * @param sql SQL command
    * @return a resultSet containing the single insert ids.
    */
  ResultSet* CmdInformationSingle::getGeneratedKeys(Protocol* protocol, const SQLString& sql)
  {
    if (insertId == 0) {
      return SelectResultSet::createEmptyResultSet();
    }

    std::vector<int64_t> insertIds{ insertId };

    if (updateCount > 1 && !sql.empty() && !isDuplicateKeyUpdate(sql)) {
      insertIds.reserve(static_cast<size_t>(updateCount));
      for (int32_t i= 1; i <updateCount; i++) {
        insertIds.push_back(insertId + i*autoIncrement);
      }
      return SelectResultSet::createGeneratedData(insertIds, protocol, true);
    }
    return SelectResultSet::createGeneratedData(insertIds, protocol, true);
  }


  bool CmdInformationSingle::isDuplicateKeyUpdate(const SQLString& sql)
  {
    //dupKeyUpdate("(?i).*ON\\s+DUPLICATE\\s+KEY\\s+UPDATE.*");
    const std::string &str= StringImp::get(sql);
    std::size_t On= 0, prev= 17/* minimal position of ON DUPLICATE clause in INSERT x VALUES() */;

    while ((On= str.find_first_of("Oo", prev)) != std::string::npos && On < str.size() - 22) {
      if ((str[On + 1] == 'N' || str[On + 1] == 'n') && std::isspace(str[On + 2])) {

        for (prev= On + 3; prev < str.size() && std::isspace(str[prev]); ++prev);
        // We don't have enough space for the rest of clause
        if (prev > str.size() - 20) {
          return false;
        }
        if ((str[prev] == 'D' || str[prev] == 'd') &&
          (str[++prev] == 'U' || str[prev] == 'u') &&
          (str[++prev] == 'P' || str[prev] == 'p') &&
          (str[++prev] == 'L' || str[prev] == 'l') &&
          (str[++prev] == 'I' || str[prev] == 'a') &&
          (str[++prev] == 'C' || str[prev] == 'c') &&
          (str[++prev] == 'A' || str[prev] == 'a') &&
          (str[++prev] == 'T' || str[prev] == 't') &&
          (str[++prev] == 'E' || str[prev] == 'e')) {

          for (++prev; prev < str.size() && std::isspace(str[prev]); ++prev);

          if (prev > str.size() - 10) {
            return false;
          }

          if ((str[prev] == 'K' || str[prev] == 'k') &&
            (str[++prev] == 'E' || str[prev] == 'e') &&
            (str[++prev] == 'Y' || str[prev] == 'y')) {

            for (++prev; prev < str.size() && std::isspace(str[prev]); ++prev);

            if (prev > str.size() - 6) {
              return false;
            }

            if ((str[prev] == 'U' || str[prev] == 'u') &&
              (str[++prev] == 'P' || str[prev] == 'p') &&
              (str[++prev] == 'D' || str[prev] == 'd') &&
              (str[++prev] == 'A' || str[prev] == 'a') &&
              (str[++prev] == 'T' || str[prev] == 't') &&
              (str[++prev] == 'E' || str[prev] == 'e')) {
              return true;
            }
          }
        }
      }
    }
    return false;
  }


  ResultSet* CmdInformationSingle::getBatchGeneratedKeys(Protocol* protocol)
  {
    return getGeneratedKeys(protocol, nullptr);
  }


  int32_t CmdInformationSingle::getCurrentStatNumber()
  {
    return 1;
  }

  bool CmdInformationSingle::moreResults()
  {
    updateCount= RESULT_SET_VALUE;
    return false;
  }

  bool CmdInformationSingle::isCurrentUpdateCount()
  {
    return updateCount != RESULT_SET_VALUE;
  }

  /* Have nothing to do, but must implement */
  void CmdInformationSingle::addSuccessStat(int64_t /*updateCount*/, int64_t /*insertId*/)
  {

  }

  /* Have nothing to do, but must implement */
  void CmdInformationSingle::setRewrite(bool /*rewritten*/)
  {

  }
}
}
