/************************************************************************************
   Copyright (C) 2026 MariaDB Corporation plc

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


#ifndef _PARAMETERSET_H_
#define _PARAMETERSET_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#include "ParameterHolder.h"

namespace sql
{
namespace mariadb
{

void bindParameter(capi::MYSQL_BIND& bind, ParameterHolder& param);

template <class T> inline void bindParameterOf(capi::MYSQL_BIND& bind, T& param)
{
  bind.buffer_type= static_cast<capi::enum_field_types>(param.T::getColumnType().getType());
  bind.is_unsigned= param.T::isUnsigned() ? '\1' : '\0';
  bind.is_null_value= '\0';
  bind.long_data_used= '\0';
  bind.buffer= nullptr;
  bind.buffer_length= 0;

  if (param.T::isNullData()) {
    bind.is_null_value= '\1';
  }
  else if (param.T::isLongData()) {
    bind.long_data_used= '\1';
  }
  else {
    bind.buffer= param.T::getValuePtr();
    bind.buffer_length= param.T::getValueBinLen();
  }
}

/* Place for one parameter value. Parameter object is created right in the slot, if it fits there,
   so setting of a parameter does not allocate memory for the object */
class ParamSlot
{
  static const std::size_t InPlaceSize= 32;

  union {
    char place[InPlaceSize];
    void* alignment;
  };
  ParameterHolder* holder= nullptr;
  uint16_t plainValue= 0;

public:
  ParamSlot() {}
  ParamSlot(const ParamSlot&)= delete;
  ParamSlot& operator=(const ParamSlot&)= delete;
  ~ParamSlot() { clear(); }

  void clear()
  {
    if (holder != nullptr) {
      ParameterHolder* current= holder;
      holder= nullptr;
      if (plainValue != 0) {
        plainValue= 0;
      }
      else if (static_cast<const void*>(current) == static_cast<const void*>(place)) {
        current->~ParameterHolder();
      }
      else {
        delete current;
      }
    }
  }

  template <class T, class... Args> T* emplace(Args&&... args)
  {
    T* created;
    clear();
    if (sizeof(T) <= InPlaceSize && alignof(T) <= alignof(void*)) {
      created= new (static_cast<void*>(place)) T(std::forward<Args>(args)...);
      if (T::PlainValueId != 0) {
        plainValue= static_cast<uint16_t>(T::PlainValueId | (created->T::getColumnType().getType() << 8));
      }
    }
    else {
      created= new T(std::forward<Args>(args)...);
    }
    holder= created;
    return created;
  }

  void reset(ParameterHolder* heapObject= nullptr)
  {
    clear();
    holder= heapObject;
  }

  /* Not 0 only for plain values. If it is the same for 2 values put in the slot, they are bound identically */
  uint16_t getPlainValue() const { return plainValue; }
  ParameterHolder* get() const { return holder; }
  ParameterHolder* operator->() const { return holder; }
  ParameterHolder& operator*() const { return *holder; }
  explicit operator bool() const { return holder != nullptr; }
};


/* Values of all parameters of a statement. If created for the binary protocol, it also keeps array of
   MYSQL_BIND structures up to date, so nothing has to be prepared for the execution */
class ParamSet
{
  std::unique_ptr<ParamSlot[]> slot;
  std::size_t count= 0;
  std::size_t assigned= 0;
  bool longData= false;
  std::vector<capi::MYSQL_BIND> bind;
  uint64_t id= 0;
  uint64_t bindVersion= 0;

public:
  ParamSet()= default;
  ParamSet(ParamSet&&)= default;
  ParamSet& operator=(ParamSet&&)= default;

  void init(std::size_t paramCount, bool binary);
  void clear();
  ParamSet clone() const;

  std::size_t size() const { return count; }
  bool complete() const { return assigned == count; }
  bool hasLongData() const { return longData; }
  capi::MYSQL_BIND* getBind() { return bind.empty() ? nullptr : bind.data(); }
  /* Id is unique for each array of MYSQL_BIND, the version is changed with each change in that array */
  uint64_t getId() const { return id; }
  uint64_t getBindVersion() const { return bindVersion; }

  ParamSlot& operator[](std::size_t i) { return slot[i]; }
  const ParamSlot& operator[](std::size_t i) const { return slot[i]; }
  ParamSlot* begin() { return slot.get(); }
  ParamSlot* end() { return slot.get() + count; }
  const ParamSlot* begin() const { return slot.get(); }
  const ParamSlot* end() const { return slot.get() + count; }

  template <class T, class... Args> void set(std::size_t i, Args&&... args)
  {
    ParamSlot& target= slot[i];

    if (!target) {
      ++assigned;
    }
    uint16_t previous= target.getPlainValue();
    T* param= target.template emplace<T>(std::forward<Args>(args)...);

    if (param->T::isLongData()) {
      longData= true;
    }
    if (!bind.empty() && (previous == 0 || previous != target.getPlainValue())) {
      bindParameterOf<T>(bind[i], *param);
      ++bindVersion;
    }
  }

  void set(std::size_t i, ParameterHolder* heapObject);
};

}
}
#endif
