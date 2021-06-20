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
#include <SQLString.hpp>
#include "buildconf.hpp"

namespace sql
{
class PropertiesImp;

#pragma warning(push)
#pragma warning(disable:4251)

class MARIADB_EXPORTED Properties final {

  friend class PropertiesImp;
  
  std::unique_ptr<PropertiesImp> theMap;

public:
  using key_type= typename SQLString;
  using mapped_type= typename SQLString ;
  using iterator= std::map<SQLString, SQLString>::iterator;
  using const_iterator= std::map<SQLString, SQLString>::const_iterator;
  using value_type= std::pair<const SQLString, SQLString>;

  Properties(const Properties& other);
  Properties(Properties&&); //Move constructor
  Properties();
  Properties(std::initializer_list<value_type> init);
  ~Properties();

  Properties& operator=(const Properties& other);
  Properties& operator=(std::initializer_list<std::pair<const char*, const char*>> init);

  mapped_type& operator[](const key_type& key);

  bool empty() const;
  SQLString& at(const SQLString& key);
  /* Not quite what std::map has - just to keep things simple */
  bool insert(const key_type& key, const mapped_type& value);
  bool insert(const value_type& value);
  bool emplace(const key_type& key, const mapped_type& value);

  std::size_t size() const;
  std::size_t erase(const key_type& key);
  Properties::iterator erase(Properties::const_iterator pos);

  Properties::iterator find(const key_type& key);
  Properties::const_iterator find(const key_type& key) const;
  Properties::iterator begin() noexcept;
  Properties::iterator end();
  Properties::const_iterator begin() const noexcept;
  Properties::const_iterator end() const;
  Properties::const_iterator cbegin() const noexcept { return begin(); }
  Properties::const_iterator cend() const { return end(); }
  void clear();
};

}
#pragma warning(pop)
#endif
