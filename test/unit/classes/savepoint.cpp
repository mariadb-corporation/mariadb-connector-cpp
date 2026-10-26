/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *               2026 MariaDB Corporation plc
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



#include "Connection.hpp"


#include "Warning.hpp"

#include "savepointtest.h"
#include <stdlib.h>

#include <memory>

namespace testsuite
{
namespace classes
{

void savepoint::getSavepointId()
{
  logMsg("savepoint::getSavepointId() - MySQL_Savepoint::getSavepointId()");
  SKIP("Currently in this version savepoint can be set "
    "in autoCommit mode. This will be fixed in next version series.");
  try
  {
    con->setAutoCommit(true);
    std::unique_ptr< sql::Savepoint > sp(con->setSavepoint("mysavepoint"));
    FAIL("You should not be able to set a savepoint in autoCommit mode");
  }
  catch (sql::SQLException &)
  {
  }
  try
  {
    con->setAutoCommit(false);
    std::unique_ptr< sql::Savepoint > sp(con->setSavepoint("mysavepoint"));
    try
    {
      sp->getSavepointId();
      FAIL("Savepoint is not anonymous - getSavepointId() should throw");
    }
    catch (sql::InvalidArgumentException &)
    {
    }
    con->releaseSavepoint(sp.get());
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

void savepoint::getSavepointName()
{
  logMsg("savepoint::getSavepointName() - MySQL_Savepoint::getSavepointName()");
  try
  {
    con->setAutoCommit(false);
    std::unique_ptr< sql::Savepoint > sp(con->setSavepoint("mysavepoint"));
    ASSERT_EQUALS("mysavepoint", sp->getSavepointName());
    con->releaseSavepoint(sp.get());
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

/**/
void savepoint::concpp164()
{
  logMsg("savepoint::concpp164() - MariaDbSavepoint::getSavepointName()");
  try
  {
    const sql::SQLString savepointName("evil`savepoint");

    createTable("concpp164", "(id INT)");

    con->setAutoCommit(false);
    std::unique_ptr< sql::Savepoint > sp(con->setSavepoint(savepointName));
    ASSERT_EQUALS(savepointName, sp->getSavepointName());
    // Verifying that the created savepoint has the requested name.
    // Execution should not fail if the connector created savepoint with the correct name.
    sql::SQLString releaseQuery("RELEASE SAVEPOINT `evil``savepoint`");
    stmt->execute(releaseQuery);
    // Creating it again to test releaseSavepoint() method of the connection.
    sp.reset(con->setSavepoint(savepointName));
    con->releaseSavepoint(sp.get());

    // Now testing that the savepoint is not only created and released, but is also usable
    stmt->executeUpdate("INSERT INTO concpp164 VALUES(1)");
    sp.reset(con->setSavepoint(savepointName));
    stmt->executeUpdate("INSERT INTO concpp164 VALUES(2)");
    con->rollback(sp.get());
    con->commit();

    // Only the row inserted before the savepoint has to survive the rollback to it
    res.reset(stmt->executeQuery("SELECT id FROM concpp164"));
    ASSERT(res->next());
    ASSERT_EQUALS(1, res->getInt(1));
    ASSERT(!res->next());
    res.reset();

    con->setAutoCommit(true);
  }
  catch (sql::SQLException& e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}
} /* namespace savepoint */
} /* namespace testsuite */
