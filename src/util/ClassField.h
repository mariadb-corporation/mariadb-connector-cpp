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


#ifndef _CLASSFIELD_H_
#define _CLASSFIELD_H_

#include <stdexcept>
#include "StringImp.h"
#include "Value.h"

namespace sql
{
namespace mariadb
{

/* If class takes pointer values, it does not take their ownership. i.e. they are not free-ed. Btw, maybe shared_ptr... or weak... */
template <class T>
class ClassField final {

public:
  enum valueType
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
    int32_t T::*iv;
    int64_t T::*lv;
    bool    T::*bv;

    SQLString T::*sv;
  };

  Variant         value;
  enum valueType  type;

public:
  ClassField() : value(), type(VNONE) {}

  ClassField(const ClassField& other) : type(other.type), value(other.value)
  {
    /*switch (type)
    {
    case VSTRING:
      value.sv= other.value.sv;
      break;
    case VINT32:
      value.iv= other.value.iv;
      break;
    case VINT64:
      value.lv= other.value.lv;
      break;
    case VBOOL:
      value.bv= other.value.bv;
      break;
    default:
      break;
    }*/
  }

  ClassField(int32_t T::*v) : type(VINT32)
  {
    value.iv= v;
  }

  ClassField(int64_t T::*v) : type(VINT64)
  {
    value.lv= v;
  }

  ClassField(bool T::*v) : type(VBOOL)
  {
    value.bv= v;
  }

  ClassField(SQLString T::*v) : type(VSTRING)
  {
    value.sv= v;
  }

  void operator=(SQLString T::*fp)
  {
    type= valueType::TSTRING;
    value.sv= fp;
  }

  void operator=(int32_t T::*fp)
  {
    type= valueType::TINT32;
    value.iv= fp;
  }

  void operator=(int64_t T::* fp)
  {
    type= valueType::TINT64;
    value.lv= fp;
  }

  void operator=(bool T::*fp)
  {
    type= valueType::TBOOL;
    value.bv= fp;
  }

  /*operator const int32_t();
  operator int32_t&();

  operator const int64_t();
  operator int64_t&();

  operator const bool();
  operator bool&();
  operator SQLString&();*/

  //operator *()
  //{
  //}

  operator int32_t T::*()
  {
    if (type == valueType::VINT32)
    {
      return value.iv;
    }

    throw std::runtime_error("Wrong lvalue type requested - the field type is not int32");
  }

  operator int64_t T::*()
  {
    if (type == valueType::VINT64)
    {
      return value.lv;
    }

    throw std::runtime_error("Wrong lvalue type requested - the field type is not int64");
  }

  operator bool T::*()
  {
    if (type == valueType::VBOOL)
    {
      return value.bv;
    }

    throw std::runtime_error("Wrong lvalue type requested - the field type is not bool");
  }

  operator SQLString T::*()
  {
    if (type == valueType::VSTRING)
    {
      return value.sv;
    }

    throw std::runtime_error("Wrong lvalue type requested - the field type is not string");
  }

  bool empty() const
  {
    return type == valueType::VNONE;
  }

  operator bool() const
  {
    return !empty();
  }

  void reset()
  {
    type= valueType::VNONE;
  }

  enum valueType objType()
  {
    return type;
  }

  void set(T& obj, const SQLString& val2set) const
  {
    if (type != valueType::VSTRING)
    {
      throw std::invalid_argument("Cannot set the value - the field is not string");
    }

    obj.*(value.sv)= val2set;
  }

  void set(T& obj, int32_t val2set) const
  {
    if (type != valueType::VINT32)
    {
      throw std::invalid_argument("Cannot set the value - the field is not int32");
    }

    obj.*(value.iv)= val2set;
  }

  void set(T& obj, int64_t val2set) const
  {
    if (type != valueType::VINT64)
    {
      throw std::invalid_argument("Cannot set the value - the field is not int64");
    }

    obj.*(value.lv)= val2set;
  }

  void set(T& obj, bool val2set) const
  {
    if (type != valueType::VBOOL)
    {
      throw std::invalid_argument("Cannot set the value - the field is not bool");
    }

    obj.*(value.bv)= val2set;
  }

  const Value get(const T& obj) const
  {
    /* We actually could make the Value out of pointer and give full access to the field,
       but do we need it? we return const */
    switch (type)
    {
    case valueType::VSTRING:
      return obj.*(value.sv);
    case valueType::VINT32:
      return obj.*(value.iv);
    case valueType::VINT64:
      return obj.*(value.lv);
    case valueType::VBOOL:
      return obj.*(value.bv);
    default:
      break;
    }

    return Value();
  }

  ~ClassField()
  {
  }
};

}
}

#endif
