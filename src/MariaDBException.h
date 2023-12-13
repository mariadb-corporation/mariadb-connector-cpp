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

#pragma once

#ifndef _MARIADB_EXCEPTION_H_
#define _MARIADB_EXCEPTION_H_

#include "Exception.hpp"

namespace sql
{
///////////////
class Thrower
{
protected:
  Thrower() = default;
public:
  virtual ~Thrower() {}
  virtual void Throw() = 0;
  virtual SQLException* get() = 0;
};

/////////////////////
template <class T> class RealThrower : public Thrower
{
  T exception;
public:
  RealThrower(T& e) : exception(e)
  {}
  void Throw() {
    throw exception;
  }

  SQLException* get() { return dynamic_cast<SQLException*>(&exception); }
};

////////////////////
class MariaDBExceptionThrower
{
  std::unique_ptr<Thrower> exceptionThrower;

  Thrower* release() { return exceptionThrower.release(); }

public:
  MariaDBExceptionThrower() : exceptionThrower(nullptr)
  {}

  template <class T> MariaDBExceptionThrower(T& exc) : exceptionThrower(new RealThrower<T>(exc))
  {
  }
  template <class T> void take(T& exc)
  {
    exceptionThrower.reset(new RealThrower<T>(exc));
  }
  
  MariaDBExceptionThrower(MariaDBExceptionThrower&& moved) noexcept;
  void assign(MariaDBExceptionThrower other);
  template <class T> T* get() { return dynamic_cast<T*>(exceptionThrower->get()); }
  SQLException* getException() { return exceptionThrower->get(); }
  void Throw() { exceptionThrower->Throw(); }

  operator bool() { return (exceptionThrower.get() != nullptr); }
};

}
#endif // _MARIADB_EXCEPTION_H_
