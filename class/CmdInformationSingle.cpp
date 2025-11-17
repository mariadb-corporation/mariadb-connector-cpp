/************************************************************************************
   Copyright (C) 2022, 2024 MariaDB Corporation plc

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

#include "interface/ResultSet.h"


namespace mariadb
{
  /**
    * Object containing update / insert ids, optimized for only one result.
    *
    * @param insertId auto generated id.
    * @param updateCount update count
    * @param autoIncrement connection auto increment value.
    */
  CmdInformationSingle::CmdInformationSingle(int64_t _updateCount)
    : updateCount(_updateCount)
  {
  }


  CmdInformationSingle::~CmdInformationSingle()
  {
  }

  std::vector<int64_t>& CmdInformationSingle::getUpdateCounts()
  {
    batchRes[0]= static_cast<int32_t>(updateCount);
    return batchRes;
  }

  std::vector<int64_t>& CmdInformationSingle::getServerUpdateCounts()
  {
    batchRes[0]= static_cast<int32_t>(updateCount);
    return batchRes;
  }

  int64_t CmdInformationSingle::getUpdateCount()
  {
    return static_cast<int64_t>(updateCount);
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


  int32_t CmdInformationSingle::getCurrentStatNumber()
  {
    return 1;
  }

  bool CmdInformationSingle::moreResults()
  {
    updateCount= RESULT_SET_VALUE;// -1?
    return false;
  }

  bool CmdInformationSingle::isCurrentUpdateCount()
  {
    return updateCount != RESULT_SET_VALUE;
  }

  void CmdInformationSingle::addSuccessStat(int64_t updateCount)
  {

  }

  void CmdInformationSingle::setRewrite(bool rewritten)
  {

  }
} // namespace mariadb
