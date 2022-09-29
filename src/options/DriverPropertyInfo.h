/************************************************************************************
   Copyright (C) 2020, 2022 MariaDB Corporation AB

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

#ifndef _DRIVERPROPERTYINFO_H_
#define _DRIVERPROPERTYINFO_H_
namespace sql
{
namespace mariadb
{
struct DriverPropertyInfo {
  std::vector<SQLString> choices;
  SQLString              description;
  SQLString              name;
  bool                   required;
  SQLString              value;
  DriverPropertyInfo(const SQLString& Name, const SQLString& Value) : name(Name), value(Value), required(false) {}
};
}
}
#endif