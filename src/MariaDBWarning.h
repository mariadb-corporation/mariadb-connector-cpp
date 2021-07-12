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

#ifndef _MARIADBWARNING_H_
#define _MARIADBWARNING_H_

#include <memory>

#include "Warning.hpp"

namespace sql
{
  namespace mariadb
  {
    class MariaDBWarning : public SQLWarning
    {
      SQLString msg;
      SQLString sqlState;
      int32_t errorCode;
      std::unique_ptr<SQLWarning> next;

      void setNextWarning(const SQLWarning* nextWarning) {}
    public:
      MariaDBWarning() : errorCode(0) {}
      virtual ~MariaDBWarning();
      MariaDBWarning(const MariaDBWarning&);
      MariaDBWarning(const SQLString& msg);
      //MariaDBWarning(const SQLString& msg, const SQLString& state, int32_t error= 0, const std::exception* e= nullptr);
      MariaDBWarning(const char* msg, const char* state= "", int32_t error= 0, const std::exception* e= nullptr);

      SQLWarning* getNextWarning() const;
      void setNextWarning(SQLWarning* nextWarning);
      const SQLString& getSQLState() const;
      int32_t getErrorCode() const;
      const SQLString& getMessage() const;
    };
  }
}
#endif
