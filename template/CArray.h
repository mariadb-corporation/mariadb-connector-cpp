/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

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


#ifndef _CARRAY_H_
#define _CARRAY_H_

#include <initializer_list>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <string>

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
  //CArray(CArray&& moved);

  CArray() : arr(nullptr), length(0)
  {}

  void assign(const T* _arr, std::size_t size= 0);
  CArray<T>& wrap(T* _arr, std::size_t size);
  CArray<T>& wrap(std::vector<T>& _vector);
  void reserve(std::size_t size);
};

#define BYTES_INIT(CSTR) {CSTR, CSTR!=nullptr ? std::strlen(CSTR) : 1}
#define BYTES_STR_INIT(STR) {STR.c_str(), STR.length()}
#define BYTES_ASSIGN_STR(BYTES, STR) BYTES.assign(STR.c_str(), STR.length())
#define BYTES_FROM_CSTR(CSTR) CSTR, std::strlen(CSTR)

//-------------------- <CArrView - Begin> --------------------
template <class T> class CArrView
{
  void make_copy(const T _arr[], std::size_t len)
  {
    T* tmp= new T[len];
    arr= tmp;
    std::memcpy(tmp, _arr, len);
  }

public:
  int64_t length= 0;
  const T* arr= nullptr;

  operator const T*() const
  {
    return arr;
  }

  operator bool()
  {
    return arr != nullptr;
  }


  CArrView(const T _arr[], std::size_t len)
    : length(static_cast<int64_t>(len))
    , arr(_arr)
  {}

  // Switched parameter places just for compiler to have less doubts what constructor to use
  CArrView(int64_t len, const T _arr[] )
    : length(len < 0 ? len : -len)
  {
    make_copy(_arr, static_cast<std::size_t>(-length));
  }

  CArrView(int64_t len)
    : length(len < 0 ? len : -len)
    , arr(new T[-length])
  {
  }

  ~CArrView()
  {
    if (length < 0 && arr)
    {
      delete[] arr;
    }
  }


  std::size_t size() const { return static_cast<std::size_t>(length < 0 ? -length : length); }
  const T* begin() const { return arr; }
  const T* end() const { return arr + size(); }


  CArrView(const CArrView& rhs)
  {
    *this= rhs;
  }

  CArrView& operator =(const CArrView& rhs)
  {
    length= rhs.length;
    if (length >= 0)
    {
      arr= rhs.arr;
    }
    else
    {
      make_copy(rhs.arr, static_cast<std::size_t>(-length));
    }
    return *this;
  }

  CArrView(CArrView&& rhs)
    : length(rhs.length)
    , arr(rhs.arr)
  {
    rhs.length= 0;
    rhs.arr= nullptr;
  }

  CArrView()
  {}


  CArrView(const std::vector<T>& _vector)
    : arr(_vector.data())
    , length(_vector.size())
  {
  }

  /* Technically we need it for char arrays only */
  CArrView(const std::string& _str)
    : arr(_str.data())
    , length(_str.size())
  {
  }


  CArrView<T>& wrap(const T* _arr, std::size_t size)
  {
    arr= _arr;
    length= static_cast<int64_t>(size);
    return *this;
  }


  CArrView<T>& takeover(const T* _arr, int64_t size)
  {
    arr= _arr;
    length= size > 0 ? -size : size;
    return *this;
  }

  CArrView<T>& wrap(const std::vector<T>& _vector)
  {
    arr= _vector.data();
    length= static_cast<int64_t>(_vector.size());
    return *this;
  }

  // Makes sense only for view, i.e. wrapped array - it's basically the same as wrap(arr, length + units). 
  CArrView<T>& append(int64_t units=1)
  {
    //assert(length > 0);
    length+= units;
  }
};
//--------------------- <CArrView - End> ---------------------

template <class T>
CArray<T>::CArray(int64_t len) : arr(nullptr), length(len)
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


template <class T>
CArray<T>::CArray(int64_t len, const T& fillValue) : CArray<T>(len)
{
  std::fill(this->begin(), this->end(), fillValue);
}

#ifndef _WIN32
# define ZEROI64 0LL
#else
# define ZEROI64 0I64
#endif
/* This constructor takes existin(stack?) array for "storing". Won't delete */
template <class T>
CArray<T>::CArray(T _arr[], size_t len) : arr(_arr), length(ZEROI64 - len)
{
}


template <class T>
CArray<T>::CArray(const T _arr[], size_t len)
: CArray(len)
{
  std::memcpy(arr, _arr, len*sizeof(T));
}


template <class T>
CArray<T>::~CArray()
{
  if (arr != nullptr && length > 0)
  {
    delete[] arr;
  }
}


template <class T>
CArray<T>::CArray(std::initializer_list<T> const& initList) : CArray(initList.end() - initList.begin())
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
  length= 0;
}
}*/

template <class T>
CArray<T>::CArray(const CArray& rhs)
: arr(rhs.arr)
, length(rhs.length)
{
  if (length > 0)
  {
    arr= new T[static_cast<size_t>(length)];
    std::memcpy(arr, rhs.arr, static_cast<std::size_t>(length));
  }
}


template <class T>
T* CArray<T>::end()
{
  return arr + (length > 0 ? length : -length);
}


template <class T> const T* CArray<T>::end() const
{
return arr + (length > 0 ? length : -length);
}


template <class T>
void CArray<T>::assign(const T* _arr, std::size_t size)
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

template <class T>
CArray<T>& CArray<T>::wrap(T* _arr, std::size_t size)
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


template <class T>
CArray<T>& CArray<T>::wrap(std::vector<T>& _vector)
{
  return this->wrap(_vector.data(), _vector.size());
}

template <class T>
void CArray<T>::reserve(std::size_t size)
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


namespace mariadb
{
  typedef CArray<char> bytes;
  typedef CArray<int32_t> Ints;
  typedef CArray<int64_t> Longs;
  typedef CArrView<char> bytes_view;
}

template <template<typename, typename...> class C, typename T>
class UniqueContainer {
public:
  UniqueContainer()= default;
  ~UniqueContainer() {
    for (auto ptr : container) {
      delete ptr;
    }
  }

  void reset() {
    for (auto ptr : container) {
      delete ptr;
    }
  }

  void emplace_back(T* ptr) {
    container.emplace_back(ptr);
  }

  void reset(T* array, std::size_t count) {
    reset();
    container.reserve(count);
    for (std::size_t i= 0; i < count; ++i) {
      emplace_back(array[i]);
    }
  }

  C<T*> get() {
    return container;
  }
private:
  C<T*> container;
};

#endif
