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


#ifndef _CMDINFORMATION_H_
#define _CMDINFORMATION_H_

#include <vector>
#include "MariaDbConnCpp.h"

namespace sql
{
namespace mariadb
{

class CmdInformation  {
  CmdInformation(const CmdInformation &);
  void operator=(CmdInformation &);
protected:
  // It's probably better to eventually move them to implementation classes
  std::vector<int32_t> batchRes;
  std::vector<int64_t> largeBatchRes;
public:
  CmdInformation() {}
  virtual ~CmdInformation(){}

  enum {
    RESULT_SET_VALUE= -1
  };
  virtual std::vector<int32_t>& getUpdateCounts()=0;
  virtual std::vector<int32_t>& getServerUpdateCounts()=0;
  virtual std::vector<int64_t>& getLargeUpdateCounts()=0;
  virtual int32_t getUpdateCount()=0;
  virtual int64_t getLargeUpdateCount()=0;
  virtual void addSuccessStat(int64_t updateCount,int64_t insertId)=0;
  virtual void addErrorStat()=0;
  virtual void reset()=0;
  virtual void addResultSetStat()=0;
  virtual ResultSet* getGeneratedKeys(Protocol* protocol, const SQLString& sql)=0;
  virtual ResultSet* getBatchGeneratedKeys(Protocol* protocol)=0;
  virtual int32_t getCurrentStatNumber()=0;
  virtual bool moreResults()=0;
  virtual bool isCurrentUpdateCount()=0;
  virtual void setRewrite(bool rewritten)=0;
};

}
}
#endif
