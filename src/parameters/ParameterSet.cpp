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


#include <atomic>
#include <cstring>

#include "ParameterSet.h"

namespace sql
{
namespace mariadb
{

  void bindParameter(capi::MYSQL_BIND& bind, ParameterHolder& param)
  {
    bind.buffer_type= static_cast<capi::enum_field_types>(param.getColumnType().getType());
    bind.is_unsigned= param.isUnsigned() ? '\1' : '\0';
    bind.is_null_value= '\0';
    bind.long_data_used= '\0';
    bind.buffer= nullptr;
    bind.buffer_length= 0;

    if (param.isNullData()) {
      bind.is_null_value= '\1';
    }
    else if (param.isLongData()) {
      bind.long_data_used= '\1';
    }
    else {
      bind.buffer= param.getValuePtr();
      bind.buffer_length= param.getValueBinLen();
    }
  }


  void ParamSet::init(std::size_t paramCount, bool binary)
  {
    static std::atomic<uint64_t> lastId{0};

    id= ++lastId;
    bindVersion= 0;
    slot.reset(paramCount > 0 ? new ParamSlot[paramCount] : nullptr);
    count= paramCount;
    assigned= 0;
    longDataCount= 0;
    bind.clear();

    if (binary && paramCount > 0) {
      bind.resize(paramCount);
      for (auto& it : bind) {
        it.is_null= &it.is_null_value;
      }
    }
  }


  void ParamSet::clear()
  {
    for (std::size_t i= 0; i < count; ++i) {
      slot[i].clear();
    }
    assigned= 0;
    longDataCount= 0;
  }


  ParamSet ParamSet::clone() const
  {
    ParamSet copy;

    copy.init(count, false);
    for (std::size_t i= 0; i < count; ++i) {
      if (slot[i]) {
        copy.set(i, slot[i]->clone());
      }
    }
    return copy;
  }


  void ParamSet::set(std::size_t i, ParameterHolder* heapObject)
  {
    ParamSlot& target= slot[i];

    if (target && longDataCount > 0 && target->isLongData()) {
      --longDataCount;
    }
    if (!target && heapObject != nullptr) {
      ++assigned;
    }
    else if (target && heapObject == nullptr) {
      --assigned;
    }
    target.reset(heapObject);

    if (heapObject != nullptr) {
      if (heapObject->isLongData()) {
        ++longDataCount;
      }
      if (!bind.empty()) {
        bindParameter(bind[i], *heapObject);
        ++bindVersion;
      }
    }
  }

}
}
