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


#ifndef _DEFAULTOPTIONS_H_
#define _DEFAULTOPTIONS_H_

#include <memory>

#include "jdbccompat.hpp"
#include "util/Value.h"
#include "StringImp.h"
#include "Options.h"
#include "credential/CredentialPlugin.h"

namespace sql
{
namespace mariadb
{
class DefaultOptions
{
  const SQLString optionName;
  const SQLString description;
  bool required;

  const Value minValue;
  const Value maxValue;

public:
  const Value defaultValue;
  static std::map<std::string, DefaultOptions*> OPTIONS_MAP;

  /* These constructor makes use of [] operator on the OptionsMap possible */
  DefaultOptions() : required(false) {}

  DefaultOptions(const char * optionName, const char * implementationVersion, const char * description, bool required);

  DefaultOptions(const char * optionName, const char * implementationVersion, const char * description,
                 bool required, const char * defaultValue);
  DefaultOptions(const char * optionName, const char * implementationVersion, const char * description,
                 bool required, bool defaultValue);

  DefaultOptions(const char* optionName, const char* implementationVersion, const char* description,
                 bool required, int32_t defaultValue, int32_t minValue);
  DefaultOptions(const char* optionName, const char* implementationVersion, const char* description,
                 int64_t defaultValue, bool required, int64_t minValue);
  /*DefaultOptions(const SQLString& optionName, const int32_t* defaultValue, int32_t minValue, const SQLString& implementationVersion,
    SQLString& description, bool required);*/

  static Shared::Options defaultValues(HaMode haMode);
  static Shared::Options defaultValues(HaMode haMode, bool pool);

  public:
    static void parse(enum HaMode haMode, const SQLString& urlParameters, Shared::Options options);
  private:
    static Shared::Options parse(enum HaMode haMode, const SQLString& urlParameters, Properties& properties);

  public:
    static Shared::Options parse(enum HaMode haMode, const SQLString& urlParameters, Properties& properties, Shared::Options options);
  private:
    static Shared::Options parse(enum HaMode haMode, const Properties& properties, Shared::Options paramOptions);

  public:
    static void postOptionProcess(const Shared::Options options, CredentialPlugin* credentialPlugin);
    static void propertyString(const Shared::Options options, enum HaMode haMode, SQLString& sb);  /*SQLString? stringstream?*/

    SQLString getOptionName();
    SQLString getDescription();
    bool isRequired();
    enum Value::valueType objType() const;
};

extern std::map<std::string, DefaultOptions> OptionsMap;

}
}
#endif