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

#include "Consts.h"
#include "CredentialPluginLoader.h"
#include "Exception.hpp"

namespace sql
{
namespace mariadb
{
  static std::shared_ptr<CredentialPlugin> nullPlugin(nullptr);
  std::map<std::string, std::shared_ptr<CredentialPlugin>> CredentialPluginLoader::plugin;

  void CredentialPluginLoader::RegisterPlugin(CredentialPlugin* aPlugin)
  {
    plugin.insert(std::pair<std::string, std::shared_ptr<CredentialPlugin>>(aPlugin->type(), std::shared_ptr<CredentialPlugin>(aPlugin)));
  }
  /**
   * Get current Identity plugin according to option `identityType`.
   *
   * @param type identity plugin type
   * @return identity plugin
   * @throws SQLException if no identity plugin found with this type is in classpath
   */
  std::shared_ptr<CredentialPlugin> CredentialPluginLoader::get(const std::string& type)
  {
    if (type.empty()){
      return nullptr;
    }
    std::map<std::string, std::shared_ptr<CredentialPlugin>>::iterator PluginTypeHandler= plugin.find(type);
    if (PluginTypeHandler != plugin.end()){
      return PluginTypeHandler->second;
    }
    /* As auth plugins are currently cared by C/C we cannot check if named plugin is available. We will later let C/C to do that.
       So far just returning "null" plugin as we do not need to throw exception now.
    */
    return nullPlugin;
    //throw sql::SQLException(SQLString("No identity plugin registered with the type \"") + type + "\".", "08004", 1251);
  }
}
}
