/************************************************************************************
   Copyright (C) 2021 MariaDB Corporation AB

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

/* Simplified std::map wrapper used for connection properties*/
#ifndef _SQLLIST_H_
#define _SQLLIST_H_

#include <memory>
#include <list>
#include "buildconf.hpp"
#include "SQLString.hpp"

namespace sql
{

#pragma warning(push)
#pragma warning(disable:4251)

class ListImp;

class List final {

  friend class ListImp;
  
  std::unique_ptr<ListImp> list;

public:

  MARIADB_EXPORTED List(const List& other);
  MARIADB_EXPORTED List(List&&); //Move constructor
  MARIADB_EXPORTED List();
  MARIADB_EXPORTED List(std::initializer_list<SQLString> init);
  MARIADB_EXPORTED ~List();
  // Should not be exported
  List(const std::list<SQLString> other) : List() {
    for (auto it : other) this->push_back(it);
  }
  MARIADB_EXPORTED List& operator=(const List& other);
  MARIADB_EXPORTED List& operator=(std::initializer_list<SQLString> init);
  MARIADB_EXPORTED void push_back(const SQLString& value);
};

}
#pragma warning(pop)
#endif
