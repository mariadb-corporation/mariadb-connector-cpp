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


#include <stdexcept>

#include "Value.h"

namespace sql
{
namespace mariadb
{
  extern const SQLString emptyStr;

  Value::Value(const Value& other) 
  {
    type= other.type;
    isPtr= other.isPtr;

    if (isPtr)
    {
      value.pv= other.value.pv;
    }
    else
    {
      switch (type)
      {
      case VSTRING:
        new (&value.sv) SQLString(other.value.sv);
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
      }
    }
  }

  Value::Value(int32_t v) : type(VINT32), isPtr(false)
  {
    value.iv= v;
  }


  Value::Value(int64_t v) : type(VINT64), isPtr(false)
  {
    value.lv= v;
  }


  Value::Value(bool v) : type(VBOOL), isPtr(false)
  {
    value.bv= v;
  }


  Value::Value(const SQLString &v) : type(VSTRING), isPtr(false)
  {
    new (&value.sv) SQLString(v);
  }


  Value::Value(const char* v) : type(VSTRING), isPtr(false)
  {
    new (&value.sv) SQLString(v);
  }


  Value::Value(int32_t* v) : type(VINT32), isPtr(true)
  {
    value.pv= v;
  }


  Value::Value(int64_t* v) : type(VINT64), isPtr(true)
  {
    value.pv= static_cast<void*>(v);
  }


  Value::Value(bool* v) : type(VBOOL), isPtr(true)
  {
    value.pv= static_cast<void*>(v);
  }


  Value::Value(SQLString *v) : type(VSTRING), isPtr(true)
  {
    value.pv= static_cast<void*>(v);
  }

  SQLString & Value::operator=(const SQLString &str)
  {

    if (type != VSTRING || isPtr)
    {
      type=  VSTRING;
      isPtr= false;
      new (&value.sv) SQLString(str);
    }
    else
    {
      value.sv= str;
    }

    return value.sv;
  }

  int32_t & Value::operator=(int32_t num)
  {
    if (type == VSTRING && !isPtr)
    {
      value.sv.~SQLString();
    }
    isPtr= false;
    type= VINT32;
    value.iv= num;

    return value.iv;
  }

  int64_t & Value::operator=(int64_t num)
  {
    if (type == VSTRING && !isPtr)
    {
      value.sv.~SQLString();
    }
    isPtr= false;
    type= VINT64;
    value.lv= num;

    return value.lv;
  }

  bool & Value::operator=(bool v)
  {
    if (type == VSTRING && !isPtr)
    {
      value.sv.~SQLString();
    }
    isPtr= false;
    type= VBOOL;
    value.bv= v;

    return value.bv;
  }

  SQLString * Value::operator=(SQLString *str)
  {
    if (type == VSTRING && !isPtr)
    {
      value.sv.~SQLString();
    }
    isPtr= true;
    type= VSTRING;
    value.pv= str;

    return str;
  }


  Value::operator const int32_t() const
  {
    switch (type)
    {
    case sql::mariadb::Value::VINT32:
      return isPtr ? *static_cast<int32_t*>(value.pv) : value.iv;
    case sql::mariadb::Value::VINT64:
      return static_cast<int32_t>(isPtr ? *static_cast<int64_t*>(value.pv) : value.lv);
    case sql::mariadb::Value::VBOOL:
      return (isPtr ? *static_cast<bool*>(value.pv) : value.bv) ? 1 : 0;
    case sql::mariadb::Value::VSTRING:
      return std::stoi(StringImp::get(isPtr ? *static_cast<SQLString*>(value.pv) : value.sv));
    case sql::mariadb::Value::VNONE:
      // or exception if empty?
      return 0;
    }
    return 0;
  }

  Value::operator int32_t&()
  {
    if (type == VINT32)
    {
      return isPtr ? *static_cast<int32_t*>(value.pv) : value.iv;
    }

    throw std::runtime_error("Wrong lvalue type requested - the type is not int32");
  }

  Value::operator const int64_t() const
  {
    switch (type)
    {
    case sql::mariadb::Value::VINT32:
      return static_cast<int64_t>(isPtr ? *static_cast<int32_t*>(value.pv) : value.iv);
    case sql::mariadb::Value::VINT64:
      return (isPtr ? *static_cast<int64_t*>(value.pv) : value.lv);
    case sql::mariadb::Value::VBOOL:
      return (isPtr ? *static_cast<bool*>(value.pv) : value.bv) ? 1 : 0;
    case sql::mariadb::Value::VSTRING:
      return std::stoll(StringImp::get(isPtr ? *static_cast<SQLString*>(value.pv) : value.sv));
    case sql::mariadb::Value::VNONE:
      return 0;
    }
    return 0;
  }


  Value::operator int64_t&()
  {
    if (type == VINT64)
    {
      return isPtr ? *static_cast<int64_t*>(value.pv) : value.lv;
    }

    throw std::runtime_error("Wrong lvalue type requested - the type is not int64");
  }


  Value::operator const bool() const
  {
    switch (type)
    {
    case sql::mariadb::Value::VINT32:
      return (isPtr ? *static_cast<int32_t*>(value.pv) : value.iv) != 0;
    case sql::mariadb::Value::VINT64:
      return (isPtr ? *static_cast<int64_t*>(value.pv) : value.lv) != 0;
    case sql::mariadb::Value::VBOOL:
      return (isPtr ? *static_cast<bool*>(value.pv) : value.bv);
    case sql::mariadb::Value::VSTRING:
    {
      const SQLString &str= isPtr ? *static_cast<SQLString*>(value.pv) : value.sv;
      if (str.compare("true") == 0)
      {
        return true;
      }
      else
      {
        return std::stoll(StringImp::get(str)) != 0;
      }
    }
    case sql::mariadb::Value::VNONE:
      return false;
    }
    return false;
  }


  Value::operator bool&()
  {
    if (type == VBOOL)
    {
      return isPtr ? *static_cast<bool*>(value.pv) : value.bv;
    }

    throw std::invalid_argument("Wrong lvalue type requested - the type is not bool");
  }


  SQLString Value::toString() const
  {
    switch (type)
    {
    case sql::mariadb::Value::VINT32:
      return std::to_string(isPtr ? *static_cast<int32_t*>(value.pv) : value.iv);
    case sql::mariadb::Value::VINT64:
      return std::to_string(isPtr ? *static_cast<int64_t*>(value.pv) : value.lv);
    case sql::mariadb::Value::VBOOL:
      return (isPtr ? *static_cast<bool*>(value.pv) : value.bv) ? "true" : "false";
    case sql::mariadb::Value::VSTRING:
      return isPtr ? *static_cast<SQLString*>(value.pv) : value.sv;
    case sql::mariadb::Value::VNONE:
      return emptyStr;
    }
    return emptyStr;
  }

  Value::operator const SQLString() const
  {
    return toString();
  }


  Value::operator SQLString&()
  {
    if (type == VSTRING)
    {
      return isPtr ? *static_cast<SQLString*>(value.pv) : value.sv;
    }

    throw std::runtime_error("Wrong lvalue type requested - the type is not string");
  }

  /* Do we also need this? */
  Value::operator std::string&()
  {
    if (type == VSTRING)
    {
      return StringImp::get(isPtr ? *static_cast<SQLString*>(value.pv) : value.sv);
    }

    throw std::invalid_argument("Wrong lvalue type requested - the type is not string");
  }


  Value::operator int32_t*()
  {
    if (type == VINT32)
    {
      return isPtr ? static_cast<int32_t*>(value.pv) : &value.iv;
    }

    throw std::invalid_argument("Wrong lvalue type requested - the type is not int32");
  }


  Value::operator int64_t*()
  {
    if (type == VINT64)
    {
      return isPtr ? static_cast<int64_t*>(value.pv) : &value.lv;
    }

    throw std::invalid_argument("Wrong lvalue type requested - the type is not int64");
  }


  Value::operator bool*()
  {
    if (type == VBOOL)
    {
      return isPtr ? static_cast<bool*>(value.pv) : &value.bv;
    }

    throw std::invalid_argument("Wrong lvalue type requested - the type is not bool");
  }


  Value::operator SQLString*()
  {
    if (type == VSTRING)
    {
      return isPtr ? static_cast<SQLString*>(value.pv) : &value.sv;
    }

    throw std::invalid_argument("Wrong lvalue type requested - the type is not string");
  }


  Value::operator const char*() const
  {
    if (type == VSTRING)
    {
      return isPtr ? static_cast<SQLString*>(value.pv)->c_str() : value.sv.c_str();
    }

    throw std::invalid_argument("Wrong lvalue type requested - the type is not string");
  }


  bool Value::empty() const
  {
    return type == VNONE;
  }


  void Value::reset()
  {
    if (type == VSTRING && !isPtr)
    {
      value.sv.~SQLString();
    }
    type= VNONE;
  }


  bool Value::equals(const Value& other) const
  {
    if (type == other.type)
    {
      switch (type)
      {
      /* Using cast to deploy operators that will take care if one or both values are pointers.
         Or should bool and bool* be different? */
      case sql::mariadb::Value::VINT32:
        return static_cast<const int32_t>(*this) == static_cast<const int32_t>(other);
      case sql::mariadb::Value::VINT64:
        return static_cast<const int64_t>(*this) == static_cast<const int64_t>(other);
      case sql::mariadb::Value::VBOOL:
        return static_cast<const bool>(*this) == static_cast<const bool>(other);
      case sql::mariadb::Value::VSTRING:
        if (isPtr) {
          if (other.isPtr) {
            return (static_cast<const SQLString*>(this->value.pv)->compare(*static_cast<const SQLString*>(other.value.pv)) == 0);
          }
          else {
            return (static_cast<const SQLString*>(this->value.pv)->compare(other.value.sv) == 0);
          }
        }
        else {
          //it (other.isPtr) {
          //}
          //else {
            return this->value.sv.compare(static_cast<const char*>(other)) == 0;
          //}
        }
      case sql::mariadb::Value::VNONE:
        return true;
      }
    }

    /* If we happen to nned to compare different type - should fairly easy to do. throwing so far */
    throw std::invalid_argument("Compared values are not of the same time");
  }

  Value::~Value()
  {
    if (type == VSTRING && !isPtr)
    {
      value.sv.~SQLString();
    }
  }

}
}

