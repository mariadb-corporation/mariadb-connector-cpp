/*
 * Copyright (c) 2022 MariaDB Corporation AB
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0, as
 * published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an
 * additional permission to link the program and your derivative works
 * with the separately licensed software that they have included with
 * MySQL.
 *
 * Without limiting anything contained in the foregoing, this file,
 * which is part of MySQL Connector/C++, is also subject to the
 * Universal FOSS Exception, version 1.0, a copy of which can be found at
 * http://oss.oracle.com/licenses/universal-foss-exception.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */


#include "../unit_fixture.h"

namespace testsuite
{
namespace classes
{

class pool : public unit_fixture
{
private:
  typedef unit_fixture super;

protected:
public:

  EXAMPLE_TEST_FIXTURE(pool)
  {
    TEST_CASE(pool_simple);
    TEST_CASE(pool_datasource);
    TEST_CASE(pool_idle);
    TEST_CASE(pool_pscache);
  }

  /* Simple test of the connection pool */
  void pool_simple();
  /* Simple test of the connection pool with DataSource class */
  void pool_datasource();
  /* Test of idle connection removal function */
  void pool_idle();
  /* Test of pool with ps cache enabled */
  void pool_pscache();
};


REGISTER_FIXTURE(pool);
} /* namespace classes */
} /* namespace testsuite */
