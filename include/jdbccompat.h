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


#ifndef _JDBCCOMPAT_H_
#define _JDBCCOMPAT_H_

#include <map>
#include <algorithm>
#include <initializer_list>
#include <istream>

/* Missing JDBC classes/types/enums or their stubs or tmporary definitions(or some of them become permanent) */
//#include "compat/Array.h"
#include "compat/Struct.h"
#include "compat/Object.h"
#include "compat/Executor.h"

namespace sql
{
  class SQLString;
  typedef enum enRowIdLifetime {
    ROWID_UNSUPPORTED= 0,
    ROWID_VALID_OTHER,
    ROWID_VALID_TRANSACTION,
    ROWID_VALID_SESSION,
    ROWID_VALID_FOREVER
  } RowIdLifetime;

  enum ClientInfoStatus
  {
    REASON_UNKNOWN= 0,
    REASON_UNKNOWN_PROPERTY,
    REASON_VALUE_INVALID,
    REASON_VALUE_TRUNCATED
  };

  typedef SQLString Time;
  typedef SQLString Date;
  typedef SQLString BigDecimal;
  typedef SQLString Timestamp;

  typedef SQLString SQLXML;
  typedef SQLString Clob;
  typedef std::istream Blob;
  typedef SQLString NClob;
  typedef SQLString URL;

  /* Simple C array wrapper template for use to pass arrays from connector */
  template <class T> struct CArray
  {
    T* arr;
    int64_t length;

    operator T*()
    {
      return arr;
    }
    operator const T*() const
    {
      return arr;
    }

    operator bool()
    {
      return arr != nullptr;
    }

    CArray(int64_t len);
    CArray(int64_t len, const T& fillValue);

    /* This constructor takes existing(stack?) array for "storing". Won't delete */
    CArray(T _arr[], size_t len);
    CArray(const T _arr[], size_t len);

    ~CArray();

    T* begin() { return arr; }
    T* end();

    std::size_t size() const { return end() - begin(); }
    const T* begin() const { return arr; }
    const T* end() const;

    CArray(std::initializer_list<T> const& initList);
    CArray(const CArray& rhs);

    CArray() : arr(nullptr), length(0)
    {}

    void assign(const T* _arr, std::size_t size= 0);
    void wrap(T* _arr, std::size_t size);
    void reserve(std::size_t size);
  };


  typedef CArray<char> bytes;
  typedef CArray<int32_t> Ints;
  typedef CArray<int64_t> Longs;


  class RowId
  {
    RowId(const RowId&);
    void operator=(RowId &)=delete;
  public:
    RowId() {}
    virtual ~RowId() {}
    virtual bytes*	getBytes() const=0;
    virtual bool equals(Object *obj) const=0;
    virtual int64_t hashCode() const=0;
    virtual SQLString toString() const=0;
  };

  class Ref
  {
    Ref(const Ref&);
    void operator=(Ref &)=delete;
  public:
    Ref() {}
    virtual ~Ref() {}
    virtual const SQLString& getBaseTypeName() const=0;
    virtual Object* getObject()=0;
    virtual void setObject()=0;
  };

  typedef std::map<SQLString, SQLString> Properties;
}
#endif
