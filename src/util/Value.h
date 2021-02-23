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


#ifndef _VALUE_H_
#define _VALUE_H_

#include <memory>

#include "StringImp.h"

namespace sql
{
namespace mariadb
{

/* If class takes pointer values, it does not take their ownership. i.e. they are not free-ed. Btw, maybe shared_ptr... or weak... */
class Value final {

public:
  enum valueType : unsigned char
  {
    VNONE=0,
    VINT32,
    VINT64,
    VBOOL,
    VSTRING,
  };
private:
  union Variant
  {
    int32_t iv;
    int64_t lv;
    bool    bv;
    SQLString sv;
    void*   pv;

    Variant(): pv(0) {}
    ~Variant(){}
  };

  Variant         value;
  enum valueType  type;
  bool            isPtr;

public:
  Value() : value(), type(VNONE), isPtr(false) {}
  Value(const Value& other);

  Value(int32_t v);
  Value(int64_t v);
  Value(bool v);
  Value(const SQLString &v);
  Value(const char* v);
  Value(int32_t* v);
  Value(int64_t* v);
  Value(bool* v);
  Value(SQLString *v);

  SQLString& operator=(const SQLString&);
  int32_t&   operator=(int32_t);
  int64_t&   operator=(int64_t);
  bool&      operator=(bool);
  SQLString* operator=(SQLString*);
  int32_t*   operator=(int32_t*);
  int64_t*   operator=(int64_t*);
  bool*      operator=(bool*);

  operator const int32_t() const;
  operator int32_t&();

  operator const int64_t() const;
  operator int64_t&();

  operator const bool() const;
  operator bool&();
  operator const SQLString() const;
  operator SQLString&();
  /* Do we also need this? */
  operator std::string&();

  operator    int32_t*();
  operator    int64_t*();
  operator       bool*();
  operator  SQLString*();
  operator const char*() const;

  bool        empty() const;
  void        reset();
  bool        equals(const Value& other) const;

  enum valueType objType() const
  {
    return type;
  }

  ~Value();

  SQLString toString() const;
};

}
}

#endif
