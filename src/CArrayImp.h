/************************************************************************************
   Copyright (C) 2020, 2021 MariaDB Corporation AB

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


/* MariaDB Connector/C++ Generally used consts, types definitions, enums,  macros, small classes, templates */

#ifndef __CARRAYINT_H_
#define __CARRAYINT_H_

#include <stdexcept>
#include <cstring>
#include "CArray.hpp"

#define BYTES_INIT(STR) {STR, STR!=nullptr ? strlen(STR) + 1 : 1}
#define BYTES_STR_INIT(STR) {STR.c_str(), STR.length()}
#define BYTES_ASSIGN_STR(BYTES, STR) BYTES.assign(STR.c_str(), STR.length())

namespace sql
{

template <class T> CArray<T>::CArray(int64_t len) : arr(nullptr), length(len)
{
  if (length < 0)
  {
    throw std::invalid_argument("Invalid length");
  }
  if (length > 0)
  {
    arr= new T[static_cast<size_t>(length)];
    if (arr == nullptr)
    {
      throw std::runtime_error("Could not allocate memory");
    }
  }
}


template <class T> CArray<T>::CArray(int64_t len, const T& fillValue) : CArray<T>(len)
{
  std::fill(this->begin(), this->end(), fillValue);
}

#ifndef _WIN32
# define ZEROI64 0LL
#else
# define ZEROI64 0I64
#endif
/* This constructor takes existin(stack?) array for "storing". Won't delete */
template <class T> CArray<T>::CArray(T _arr[], size_t len) : arr(_arr), length(ZEROI64 - len)
{
}


template <class T> CArray<T>::CArray(const T _arr[], size_t len)
  : CArray(len)
{
  std::memcpy(arr, _arr, len*sizeof(T));
}


template <class T> CArray<T>::~CArray()
{
  if (arr != nullptr && length > 0)
  {
    delete[] arr;
  }
}


template <class T> CArray<T>::CArray(std::initializer_list<T> const& initList) : CArray(initList.end() - initList.begin())
{
  std::copy(initList.begin(), initList.end(), arr);
}


/*template <class T> CArray<T>::CArray(CArray&& rhs)
  : arr(rhs.arr)
  , length(rhs.length)
{
  if (rhs.length > 0)
  {
    rhs.arr= nullptr;
    length = 0;
  }
}*/

template <class T> CArray<T>::CArray(const CArray& rhs)
  : arr(rhs.arr)
  , length(rhs.length)
{
  if (length > 0)
  {
    arr= new T[static_cast<size_t>(length)];
    std::memcpy(arr, rhs.arr, static_cast<std::size_t>(length));
  }
}


template <class T> T* CArray<T>::end()
{
  return arr + (length > 0 ? length : -length);
}


template <class T> const T* CArray<T>::end() const
{
  return arr + (length > 0 ? length : -length);
}


template <class T> void CArray<T>::assign(const T* _arr, std::size_t size)
{
  if (size == 0)
  {
    if (length == 0)
    {
      throw std::invalid_argument("Size is not given, and the array is not yet allocated");
    }
    else
    {
      size= this->size();
    }
  }
  else if (size > this->size())
  {
    if (arr == nullptr/* && length == 0*/)
    {
      length= size;
      arr= new T[size];
    }
    else
    {
      throw std::invalid_argument("Size is greater, then array's capacity");
    }
  }

  std::memcpy(arr, _arr, size*sizeof(T));
}

template <class T> CArray<T>& CArray<T>::wrap(T* _arr, std::size_t size)
{
  if (length > 0/* && arr != nullptr*/)
  {
    delete[] arr;
  }

  arr= _arr;
  if (arr == nullptr)
  {
    length= 0;
  }
  else
  {
    length= ZEROI64 - size;
  }
  return *this;
}


template <class T> CArray<T>& CArray<T>::wrap(std::vector<T>& _vector)
{
  return this->wrap(_vector.data(), _vector.size());
}

template <class T> void CArray<T>::reserve(std::size_t size)
{
  if (size > 0)
  {
    if (length > 0)
    {
      if (static_cast<std::size_t>(length) < size)
      {
        delete[] arr;

      }
      else
      {
        return;
      }
    }
    arr= new T[size];
    length= size;
  }
  // else deallocate?
}
/*template <class T> T* operator+(CArray<T>& arr, size_t offset)
{
  // Should check the range? and return no further than the end of the array?
  return static_cast<T*>(arr) + offset;
}*/

} //---- namespave sql
#endif
