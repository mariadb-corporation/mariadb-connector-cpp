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

#ifndef _PROPERTIESIMP_H_
#define _PROPERTIESIMP_H_

#include <map>
#include "SQLString.hpp"
#include "Properties.hpp"

namespace sql
{
  class PropertiesImp
  {
  public:
    using ImpType= std::map<SQLString, SQLString>;

    ImpType realMap;

  public:
    static ImpType& get(Properties& props);
    static const ImpType& get(const Properties& props);

    PropertiesImp(const ImpType& other);
    PropertiesImp()= default;
    ~PropertiesImp()= default;

    ImpType* operator ->() { return &realMap; }
    ImpType& get() { return realMap; }

    Properties::iterator erase(Properties::const_iterator pos);
    Properties::iterator find(const Properties::key_type& key);
    Properties::const_iterator cfind(const Properties::key_type& key) const;
    Properties::iterator begin();
    Properties::iterator end();
    Properties::const_iterator cbegin();
    Properties::const_iterator cend();
  };

  class iteratorImp
  {
  public:
    using ImpType= PropertiesImp::ImpType::iterator;

    ImpType real;

  public:
    static ImpType& get(Properties::iterator& it);
    static const ImpType& get(const Properties::iterator& it);

    iteratorImp(const ImpType& other) : real(other) {}
    iteratorImp() = default;
    ~iteratorImp() = default;

    ImpType* operator ->() { return &real; }

    ImpType& get() { return real; }
  };

  class const_iteratorImp
  {
  public:
    using ImpType = PropertiesImp::ImpType::const_iterator;

    ImpType real;

  public:
    static ImpType& get(Properties::const_iterator& it);
    static const ImpType& get(const Properties::const_iterator& it);

    const_iteratorImp(const ImpType& other) : real(other) {}
    const_iteratorImp() = default;
    ~const_iteratorImp() = default;

    ImpType* operator ->() { return &real; }

    ImpType& get() { return real; }
  };

}

#endif
