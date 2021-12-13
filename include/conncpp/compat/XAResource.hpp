/*
 *
 * MariaDB  C++ Connector
 *
 * Copyright (c) 2021 MariaDB Ab.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not see <http://www.gnu.org/licenses>
 * or write to the Free Software Foundation, Inc., 
 * 51 Franklin St., Fifth Floor, Boston, MA 02110, USA
 *
 */

#ifndef _XARESOURCE_H_
#define _XARESOURCE_H_
/* Stub class for the interface, that is used in one of methods for the non-implemented functionality. */
namespace sql
{
class XAResource{
  XAResource(const XAResource&);
  void operator=(XAResource &);
public:
  XAResource() {}
  virtual ~XAResource(){}
};
}
#endif
