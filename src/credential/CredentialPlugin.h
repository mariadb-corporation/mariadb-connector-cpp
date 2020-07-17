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


#ifndef _CREDENTIALPLUGIN_H_
#define _CREDENTIALPLUGIN_H_

#include "HostAddress.h"
#include "Credential.h"
#include "options/Options.h"

namespace sql
{
namespace mariadb
{
class CredentialPlugin {

  /* Will later change it to be base abstract class. Using this as a stub so far */
  std::string Name;
  std::string Type;

public:
  virtual std::string name() {return Name;}//=0;
  virtual std::string type() {return Type;}//=0;

  virtual bool mustUseSsl()
  {
    return false;
  }

  virtual SQLString defaultAuthenticationPluginType()
  {
    return "";
  }

  virtual CredentialPlugin* initialize(Shared::Options options, const SQLString& userName, const HostAddress& hostAddress)
  {
    Name= StringImp::get(userName);

    return this;
  }

  Credential* get()
  {
    return new Credential(Name, "");
  }
};

}
}
#endif
