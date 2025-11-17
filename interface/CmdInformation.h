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


#ifndef _CMDINFORMATION_H_
#define _CMDINFORMATION_H_

#include <vector>
#include <memory>
#include <cstdint>

namespace mariadb
{

class CmdInformation
{
  CmdInformation(const CmdInformation &);
  void operator=(CmdInformation &);
protected:
  std::vector<int64_t> batchRes;
public:
  CmdInformation() {}
  virtual ~CmdInformation(){}

  enum {
    RESULT_SET_VALUE= -2
  };
  virtual std::vector<int64_t>& getUpdateCounts()=0;
  virtual std::vector<int64_t>& getServerUpdateCounts()=0;
  virtual int64_t getUpdateCount()=0;
  virtual void addSuccessStat(int64_t updateCount)=0;
  virtual void addErrorStat()=0;
  virtual void reset()=0;
  virtual void addResultSetStat()=0;
  virtual int32_t getCurrentStatNumber()=0;
  virtual bool moreResults()=0;
  virtual uint32_t hasMoreResults()=0;
  virtual bool isCurrentUpdateCount()=0;
  virtual void setRewrite(bool rewritten)=0;
};

namespace Unique
{
  typedef std::unique_ptr<mariadb::CmdInformation> CmdInformation;
}
}
#endif
