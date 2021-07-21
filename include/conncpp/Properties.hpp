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
#ifndef _SQLPROPERTIES_H_
#define _SQLPROPERTIES_H_

#include <memory>
#include <map>
#include "SQLString.hpp"
#include "buildconf.hpp"

namespace sql
{
class PropertiesImp;

#pragma warning(push)
#pragma warning(disable:4251)

class PropertiesImp;
class iteratorImp;
class const_iteratorImp;

class Properties final {

  friend class PropertiesImp;
  
  std::unique_ptr<PropertiesImp> theMap;

public:
  using value_type= std::pair<const SQLString, SQLString>;
  class iterator final {
    friend class PropertiesImp;
    friend class iteratorImp;
    std::unique_ptr<iteratorImp> it;

    iterator(const std::map<SQLString, SQLString>::iterator& _it);
  public:
    MARIADB_EXPORTED iterator();
    MARIADB_EXPORTED iterator(const iterator& other);
    MARIADB_EXPORTED ~iterator();

    MARIADB_EXPORTED iterator& operator ++();
    MARIADB_EXPORTED iterator& operator --();
    // Postfix
    MARIADB_EXPORTED iterator operator ++(int);
    MARIADB_EXPORTED iterator operator --(int);

    MARIADB_EXPORTED value_type& operator*();
    MARIADB_EXPORTED const value_type& operator*() const;
    MARIADB_EXPORTED value_type* operator->();
    MARIADB_EXPORTED const value_type* operator->() const;
    MARIADB_EXPORTED bool operator==(const Properties::iterator& right) const;
    MARIADB_EXPORTED bool operator!=(const Properties::iterator& right) const;

  };
  using key_type= SQLString;
  using mapped_type= SQLString ;

  class const_iterator final {
    friend class PropertiesImp;
    friend class const_iteratorImp;
    std::unique_ptr<const_iteratorImp> cit;

    const_iterator(const std::map<SQLString, SQLString>::const_iterator& _it);
  public:
    MARIADB_EXPORTED const_iterator();
    MARIADB_EXPORTED const_iterator(const const_iterator& other);
    MARIADB_EXPORTED ~const_iterator();

    MARIADB_EXPORTED const_iterator& operator ++();
    MARIADB_EXPORTED const_iterator& operator --();
    // Postfix
    MARIADB_EXPORTED const_iterator operator ++(int);
    MARIADB_EXPORTED const_iterator operator --(int);

    MARIADB_EXPORTED const value_type& operator*() const;
    MARIADB_EXPORTED const value_type* operator->() const;

    MARIADB_EXPORTED bool operator==(const Properties::const_iterator& right) const;
    MARIADB_EXPORTED bool operator!=(const Properties::const_iterator& right) const;
  };

  MARIADB_EXPORTED Properties(const Properties& other);
  MARIADB_EXPORTED Properties(Properties&&); //Move constructor
  MARIADB_EXPORTED Properties();
  MARIADB_EXPORTED Properties(std::initializer_list<value_type> init);
  MARIADB_EXPORTED ~Properties();
  // Should not be exported
  Properties(std::map<SQLString, SQLString> other) : Properties() {
    for (auto it : other) this->emplace(it.first, it.second);
  }
  MARIADB_EXPORTED Properties& operator=(const Properties& other);
  MARIADB_EXPORTED Properties& operator=(std::initializer_list<std::pair<const char*, const char*>> init);

  MARIADB_EXPORTED mapped_type& operator[](const key_type& key);

  MARIADB_EXPORTED bool empty() const;
  MARIADB_EXPORTED SQLString& at(const SQLString& key);
  /* Not quite what std::map has - just to keep things simple */
  MARIADB_EXPORTED bool insert(const key_type& key, const mapped_type& value);
  MARIADB_EXPORTED bool insert(const value_type& value);
  MARIADB_EXPORTED bool emplace(const key_type& key, const mapped_type& value);

  MARIADB_EXPORTED std::size_t size() const;
  MARIADB_EXPORTED std::size_t erase(const key_type& key);
  MARIADB_EXPORTED Properties::iterator erase(Properties::const_iterator pos);

  MARIADB_EXPORTED Properties::iterator find(const key_type& key);
  MARIADB_EXPORTED Properties::const_iterator find(const key_type& key) const;
  MARIADB_EXPORTED Properties::iterator begin();
  MARIADB_EXPORTED Properties::iterator end();
  MARIADB_EXPORTED Properties::const_iterator begin() const;
  MARIADB_EXPORTED Properties::const_iterator end() const;
  MARIADB_EXPORTED Properties::const_iterator cbegin() const;
  MARIADB_EXPORTED Properties::const_iterator cend() const;
  MARIADB_EXPORTED void clear();
};

}
#pragma warning(pop)
#endif
