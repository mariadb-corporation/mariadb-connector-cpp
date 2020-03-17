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


#include "MariaDbSavepoint.h"

namespace sql
{
  namespace mariadb
  {

    MariaDbSavepoint::MariaDbSavepoint(const SQLString& _name,int32_t _savepointId) :
      savepointId(_savepointId), name(_name)
    {
    }

    /**
     * Retrieves the generated ID for the savepoint that this <code>Savepoint</code> object
     * represents.
     *
     * @return the numeric ID of this savepoint
     * @since 1.4
     */
    int32_t MariaDbSavepoint::getSavepointId() const {
      return savepointId;
    }

    /**
     * Retrieves the name of the savepoint that this <code>Savepoint</code> object represents.
     *
     * @return the name of this savepoint
     * @since 1.4
     */
    const SQLString& MariaDbSavepoint::getSavepointName() const {
      return name;
    }

    SQLString MariaDbSavepoint::toString() const
    {
      SQLString res(name);
      return res.append(std::to_string(savepointId));
    }


  }
}
