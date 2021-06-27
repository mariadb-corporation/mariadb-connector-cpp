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
  /**************** iteratorImp *******************/
  iteratorImp::ImpType& iteratorImp::get(Properties::iterator& it)
  {
    return it.it->real;
  }

  const iteratorImp::ImpType& iteratorImp::get(const Properties::iterator& it)
  {
    return it.it->real;
  }
  ////////////// iteratorImp - End /////////////////
  /**************** const_iteratorImp *******************/
  const_iteratorImp::ImpType& const_iteratorImp::get(Properties::const_iterator& cit)
  {
    return cit.cit->real;
  }

  const const_iteratorImp::ImpType& const_iteratorImp::get(const Properties::const_iterator& cit)
  {
    return cit.cit->real;
  }
  ////////////// iteratorImp - End ///////////////// 
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


  Properties::iterator PropertiesImp::erase(Properties::const_iterator pos)
  {
    return realMap.erase(pos.cit->real);
  }

  Properties::iterator PropertiesImp::find(const Properties::key_type& key)
  {
    return realMap.find(key);
  }

  Properties::const_iterator PropertiesImp::cfind(const Properties::key_type& key) const
  {
    return realMap.find(key);
  }

  Properties::iterator PropertiesImp::begin()
  {
    return realMap.begin();
  }

  Properties::iterator PropertiesImp::end()
  {
    return realMap.end();
  }

  Properties::const_iterator PropertiesImp::cbegin()
  {
    return realMap.cbegin();
  }

  Properties::const_iterator PropertiesImp::cend()
  {
    return realMap.cend();
  }
  ///////////// PropertiesImp - End ////////////////

  /************ Properties::iterator **************/
  Properties::iterator::iterator(const std::map<SQLString, SQLString>::iterator& _it) : iterator()
  {
    *it= _it;
  }

  Properties::iterator::iterator() : it(new iteratorImp())
  {}

  Properties::iterator::iterator(const iterator& other): it(new iteratorImp(*other.it))
  {}

  Properties::iterator::~iterator()
  {}

  Properties::iterator& Properties::iterator::operator ++()
  {
    ++(it->real);
    return *this;
  }

  Properties::iterator& Properties::iterator::operator --()
  {
    --(it->real);
    return *this;
  }

  Properties::iterator Properties::iterator::operator ++(int dummy)
  {
    Properties::iterator current(*this);
    ++(*this);
    return current;
  }

  Properties::iterator Properties::iterator::operator --(int dummy)
  {
    Properties::iterator current(*this);
    --(*this);
    return current;
  }

  Properties::value_type& Properties::iterator::operator*()
  {
    return *(it->real);
  }

  const Properties::value_type& Properties::iterator::operator*() const
  {
    return *(it->real);
  }

  Properties::value_type* Properties::iterator::operator->()
  {
    return &*(it->real);
  }

  const Properties::value_type* Properties::iterator::operator->() const
  {
    return &*(it->real);
  }
  /////////// Properties::iterator - End ///////////
  /************ Properties::const_iterator **************/
  Properties::const_iterator::const_iterator(const std::map<SQLString, SQLString>::const_iterator& _it) : const_iterator()
  {
    *cit = _it;
  }

  Properties::const_iterator::const_iterator() : cit(new const_iteratorImp())
  {}

  Properties::const_iterator::const_iterator(const const_iterator& other) : cit(new const_iteratorImp(*other.cit))
  {}

  Properties::const_iterator::~const_iterator()
  {}

  Properties::const_iterator& Properties::const_iterator::operator ++()
  {
    ++(cit->real);
    return *this;
  }

  Properties::const_iterator& Properties::const_iterator::operator --()
  {
    --(cit->real);
    return *this;
  }

  Properties::const_iterator Properties::const_iterator::operator ++(int dummy)
  {
    Properties::const_iterator current(*this);
    ++(*this);
    return current;
  }

  Properties::const_iterator Properties::const_iterator::operator --(int dummy)
  {
    Properties::const_iterator current(*this);
    --(*this);
    return current;
  }


  const Properties::value_type& Properties::const_iterator::operator*() const
  {
    return *(cit->real);
  }


  const Properties::value_type* Properties::const_iterator::operator->() const
  {
    return &*(cit->real);
  }
  /////////// Properties::const_iterator - End ///////////
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
    return theMap->erase(pos);
  }


  Properties::iterator Properties::find(const key_type& key)
  {
    return theMap->find(key);
  }

  Properties::const_iterator Properties::find(const key_type& key) const
  {
    return theMap->cfind(key);
  }


  Properties::iterator Properties::begin()
  {
    return theMap->begin();
  }

  Properties::iterator Properties::end()
  {
    return theMap->end();
  }

  Properties::const_iterator Properties::begin() const
  {
    return theMap->cbegin();
  }

  Properties::const_iterator Properties::end() const
  {
    return theMap->cend();
  }

  Properties::const_iterator Properties::cbegin() const
  {
    return theMap->cbegin();
  }

  Properties::const_iterator Properties::cend() const
  {
    return theMap->cend();
  }

  void Properties::clear()
  {
    (*theMap)->clear();
  }


  /** Standalone operators/functions */
  bool operator==(const Properties::iterator& left, const Properties::iterator& right)
  {
    return (iteratorImp::get(left) == iteratorImp::get(right));
  }

  bool operator!=(Properties::iterator& left, Properties::iterator& right)
  {
    return (iteratorImp::get(left) != iteratorImp::get(right));
  }

  bool operator==(Properties::const_iterator& left, Properties::const_iterator& right)
  {
    return (const_iteratorImp::get(left) == const_iteratorImp::get(right));
  }
  bool operator!=(Properties::const_iterator& left, Properties::const_iterator& right)
  {
    return (const_iteratorImp::get(left) != const_iteratorImp::get(right));
  }
};

