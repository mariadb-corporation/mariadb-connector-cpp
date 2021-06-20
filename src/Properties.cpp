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

#include "Properties.hpp"
#include "PropertiesImp.h"

namespace sql
{
  /*********** PropertiesImp - 2 static methods and constructor ******/
  PropertiesImp::ImpType& PropertiesImp::get(Properties& props)
  {
    return props.theMap->realMap;
  }


  const PropertiesImp::ImpType& PropertiesImp::get(const Properties& props)
  {
    return props.theMap->realMap;
  }

  PropertiesImp::PropertiesImp(const PropertiesImp::ImpType& other) :
    realMap(other)
  {

  }
  ///////////// PropertiesImp - End ////////////////
  /**************** Properties ********************/
  Properties::Properties(const Properties& other)
  {
    theMap.reset(new PropertiesImp(*other.theMap));
  }


  Properties::Properties(Properties&& other)
  {
    theMap= std::move(other.theMap);
  }


  Properties::Properties()
  {
    theMap.reset(new PropertiesImp());
  }

  Properties::Properties(std::initializer_list<value_type> init) : Properties()
  {
    theMap->realMap.insert(init);
  }


  Properties::~Properties()
  {
  }


  Properties& Properties::operator=(const Properties& other)
  {
    theMap.reset(new PropertiesImp(*other.theMap));
    return *this;
  }


  Properties& Properties::operator=(std::initializer_list<std::pair<const char*, const char*>> init)
  {
    (*theMap)->clear();

    for (auto it : init)
    {
      (*theMap)->emplace(it.first, it.second);
    }
    return *this;
  }


  Properties::mapped_type& Properties::operator[](const key_type& key)
  {
    return theMap->get()[key];
  }


  bool Properties::empty() const
  {
    return (*theMap)->empty();
  }


  SQLString& Properties::at(const SQLString& key)
  {
    return (*theMap)->at(key);
  }


  bool Properties::insert(const key_type& key, const mapped_type& value)
  {
    auto res= (*theMap)->emplace(key, value);
    return res.second;
  }

  bool Properties::insert(const Properties::value_type& value)
  {
    auto res= (*theMap)->insert(value);
    return res.second;
  }


  bool Properties::emplace(const key_type& key, const mapped_type& value)
  {
    auto res = (*theMap)->emplace(key, value);
    return res.second;
  }


  std::size_t Properties::size() const
  {
    return (*theMap)->size();
  }

  std::size_t Properties::erase(const key_type& key)
  {
    return (*theMap)->erase(key);
  }

  Properties::iterator Properties::erase(Properties::const_iterator pos)
  {
    return (*theMap)->erase(pos);
  }


  Properties::iterator Properties::find(const key_type& key)
  {
    return (*theMap)->find(key);
  }

  Properties::const_iterator Properties::find(const key_type& key) const
  {
    return (*theMap)->find(key);
  }


  Properties::iterator Properties::begin() noexcept
  {
    return (*theMap)->begin();
  }

  Properties::iterator Properties::end()
  {
    return (*theMap)->end();
  }

  Properties::const_iterator Properties::begin() const noexcept
  {
    return (*theMap)->begin();
  }

  Properties::const_iterator Properties::end() const
  {
    return (*theMap)->end();
  }


  void Properties::clear()
  {
    (*theMap)->clear();
  }



};

