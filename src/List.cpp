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

#include "ListImp.h"

namespace sql
{
  /*********** ListImp - 4 static methods and constructor ******/
  ListImp::ImpType& ListImp::get(List& props)
  {
    return props.list->real;
  }


  const ListImp::ImpType& ListImp::get(const List& props)
  {
    return props.list->real;
  }

  ListImp::ListImp(const ListImp::ImpType& other) :
    real(other)
  {
  }
  ///////////// ListImp - End ////////////////

  /**************** List ********************/
  List::List(const List& other)
  {
    list.reset(new ListImp(*other.list));
  }


  List::List(List&& other)
  {
    list= std::move(other.list);
  }


  List::List()
  {
    list.reset(new ListImp());
  }

  List::List(std::initializer_list<SQLString> init) : List()
  {
    for (auto it : init)
    {
      list->real.emplace_back(it);
    }
  }


  List::~List()
  {
  }


  List& List::operator=(const List& other)
  {
    list.reset(new ListImp(*other.list));
    return *this;
  }


  List& List::operator=(std::initializer_list<SQLString> init)
  {
    (*list)->clear();
    for (auto it : init)
    {
      list->real.emplace_back(it);
    }
    return *this;
  }

  void List::push_back(const SQLString& value)
  {
    return (*list)->push_back(value);
  }

};

