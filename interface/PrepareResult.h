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


#ifndef _PREPARERESULT_H_
#define _PREPARERESULT_H_

#include <vector>
#include <memory>

#include "SQLString.h"
#include "ColumnDefinition.h"
#include "pimpls.h"

namespace mariadb
{

class PrepareResult
{
  void operator=(PrepareResult &)= delete;
  
protected:
  std::vector<ColumnDefinition> column;
  // Independet from C/C copy. Is needed to be able to reurn array of FIELD structures. ColumnDefinition also is referring these structures
  // It's better to have FIELDS array cuz in text protocol we don't have pure metadata MYSQL_RES object
  std::vector<const MYSQL_FIELD*> field;

  void init(MYSQL_FIELD* fields, std::size_t fieldCount);

public:
  PrepareResult() {}

  virtual ~PrepareResult(){}

  virtual const SQLString& getSql() const=0;
  virtual std::size_t getParamCount() const=0;
  virtual ResultSetMetaData* getEarlyMetaData()=0;
};

}
#endif
