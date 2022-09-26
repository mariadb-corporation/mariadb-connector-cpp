/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *               2020, 2022 MariaDB Corporation AB
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



#include "PreparedStatement.hpp"
#include "Connection.hpp"
#include "Warning.hpp"
#include "preparedstatementtest.h"
#include <stdlib.h>

#include <memory>

namespace testsuite
{
namespace classes
{
void preparedstatement::setUp()
{
  sql::SQLString sspsOptionValue;
  sql::Properties::iterator useServerPrepStmts= commonProperties.find("useServerPrepStmts");
  bool hadOption= useServerPrepStmts != commonProperties.end();

  if (hadOption) {
    sspsOptionValue= useServerPrepStmts->second;
  }

  commonProperties["useServerPrepStmts"]= "false";
  super::setUp();

  commonProperties["useServerPrepStmts"] = "true";
  try
  {
    sspsCon.reset(this->getConnection(&commonProperties));
  }
  catch (sql::SQLException& sqle)
  {
    logErr(String("Couldn't get connection") + sqle.what());
    throw sqle;
  }

  sspsCon->setSchema(db);
}


void preparedstatement::InsertSelectAllTypes()
{
  logMsg("preparedstatement::InsertSelectAllTypes() - MySQL_PreparedStatement::*");

  //TODO: Enable it after fixing

  std::stringstream sql;
  std::vector<columndefinition>::iterator it;
  stmt.reset(con->createStatement());
  bool got_warning=false;
  size_t len;

  try
  {
    bool blobTestMakesSense= true;
    for (it = columns.end(), it--; it != columns.begin(); it--)
    {
      stmt->execute("DROP TABLE IF EXISTS test");
      sql.str("");
      sql << "CREATE TABLE test(dummy TIMESTAMP, id " << it->sqldef << ")";
      try
      {
        stmt->execute(sql.str());
        sql.str("");
        sql << "... testing '" << it->sqldef << "'";
        logMsg(sql.str());
      }
      catch (sql::SQLException&)
      {
        sql.str("");
        sql << "... skipping '" << it->sqldef << "'";
        logMsg(sql.str());
        continue;
      }

      pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));
      if (it->name == "BIT")
      {
        pstmt->setLong(1, std::stoull(it->value));
      }
      else
      {
        pstmt->setString(1, it->value);
      }
      ASSERT_EQUALS(1, pstmt->executeUpdate());

      pstmt.reset(con->prepareStatement("SELECT id, NULL FROM test"));
      res.reset(pstmt->executeQuery());
      checkResultSetScrolling(res);
      ASSERT(res->next());

      res.reset();
      res.reset(pstmt->executeQuery());
      checkResultSetScrolling(res);
      ASSERT(res->next());

      if (it->check_as_string && (res->getString(1) != it->as_string))
      {
        sql.str("");
        sql << "... \t\tWARNING - SQL: '" << it->sqldef << "' - expecting '" << it->as_string << "'";
        sql << " got '" << res->getString(1) << "'";
        logMsg(sql.str());
        got_warning = true;
      }

      sql::SQLString value = res->getString(1);
      ASSERT_EQUALS(res->getString("id"), String(value.c_str(), value.length()));
      try
      {
        res->getString(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      try
      {
        res->getString(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      res->beforeFirst();
      try
      {
        res->getString(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getString(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();

      try
      {
        ASSERT_EQUALS(res->getDouble("id"), res->getDouble(1));
        try
        {
          res->getDouble(0);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        try
        {
          res->getDouble(3);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        res->beforeFirst();
        try
        {
          res->getDouble(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->afterLast();
        try
        {
          res->getDouble(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->first();

        ASSERT_EQUALS(res->getInt(1), res->getInt("id"));
        try
        {
          res->getInt(0);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        try
        {
          res->getInt(3);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        res->beforeFirst();
        try
        {
          res->getInt(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->afterLast();
        try
        {
          res->getInt(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->first();

        ASSERT_EQUALS(res->getUInt(1), res->getUInt("id"));
        try
        {
          res->getUInt(0);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        try
        {
          res->getUInt(3);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        res->beforeFirst();
        try
        {
          res->getUInt(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->afterLast();
        try
        {
          res->getUInt(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->first();

        ASSERT_EQUALS(res->getInt64("id"), res->getInt64(1));
        try
        {
          res->getInt64(0);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        try
        {
          res->getInt64(3);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        res->beforeFirst();
        try
        {
          res->getInt64(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->afterLast();
        try
        {
          res->getInt64(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->first();

        ASSERT_EQUALS(res->getUInt64("id"), res->getUInt64(1));
        try
        {
          res->getUInt64(0);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        try
        {
          res->getUInt64(3);
          FAIL("Invalid argument not detected");
        }
        catch (sql::InvalidArgumentException&)
        {
        }

        res->beforeFirst();
        try
        {
          res->getUInt64(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
        res->afterLast();
        try
        {
          res->getUInt64(1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLDataException&)
        {
        }
      }
      catch (sql::SQLException & e)
      {
        //All is good
        if ((e.getErrorCode() != 1264 || e.getSQLState().compare("22003") != 0) && !e.getMessage().startsWith("getDouble not available for data field type"))
        {
          throw e;
        }
      }
      res->first();

      ASSERT_EQUALS(res->getBoolean("id"), res->getBoolean(1));
      try
      {
        res->getBoolean(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      try
      {
        res->getBoolean(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      res->beforeFirst();
      try
      {
        res->getBoolean(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getBoolean(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();

      /* YEAR is first type after char/bin types(traversing backwards). The way getBlob test is written now,
         it will work correctly only for those types(and maybe some others), and YEAR is firdt type it will not */
      if (it->name == "YEAR")
      {
        blobTestMakesSense= false;
      }
      // TODO - make BLOB
      if (it->check_as_string && blobTestMakesSense)
      {
        {
          std::unique_ptr<std::istream> blob_output_stream(res->getBlob(1));
          len=it->as_string.length();
          std::unique_ptr<char[]> blob_out(new char[len]);
          blob_output_stream->read(blob_out.get(), len);
          if (it->as_string.compare(0, static_cast<std::size_t>(blob_output_stream->gcount())
                                    , blob_out.get(), static_cast<std::size_t>(blob_output_stream->gcount())))
          {
            sql.str("");
            sql << "... \t\tWARNING - SQL: '" << it->sqldef << "' - expecting '" << it->as_string << "'";
            sql << " got '" << res->getString(1) << "'";
            logMsg(sql.str());
            got_warning=false;
          }
        }

        {
          std::unique_ptr<std::istream> blob_output_stream(res->getBlob("id"));
          len=it->as_string.length();
          std::unique_ptr<char[]> blob_out(new char[len]);
          blob_output_stream->read(blob_out.get(), len);
          if (it->as_string.compare(0, static_cast<std::size_t>(blob_output_stream->gcount())
                                    , blob_out.get(), static_cast<std::size_t>(blob_output_stream->gcount())))
          {
            sql.str("");
            sql << "... \t\tWARNING - SQL: '" << it->sqldef << "' - expecting '" << it->as_string << "'";
            sql << " got '" << res->getString(1) << "'";
            logMsg(sql.str());
            got_warning=false;
          }
        }
      }
      try
      {
        res->getBlob(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      try
      {
        res->getBlob(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      res->beforeFirst();
      try
      {
        res->getBlob(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getBlob(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();

    }
    stmt->execute("DROP TABLE IF EXISTS test");
    if (got_warning)
      FAIL("See warnings");

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

void preparedstatement::assortedSetType()
{
  logMsg("preparedstatement::assortedSetType() - MySQL_PreparedStatement::set*");

  //TODO: Enable it after fixing
  //SKIP("Removed until fixed");

  std::stringstream sql;
  std::vector<columndefinition>::iterator it;
  stmt.reset(con->createStatement());
  bool got_warning=false;

  try
  {

    for (it=columns.end(), it--; it != columns.begin(); it--)
    {
      stmt->execute("DROP TABLE IF EXISTS test");
      sql.str("");
      sql << "CREATE TABLE test(dummy TIMESTAMP, id " << it->sqldef << ")";
      try
      {
        stmt->execute(sql.str());
        sql.str("");
        sql << "... testing '" << it->sqldef << "'";
        logMsg(sql.str());
      }
      catch (sql::SQLException &)
      {
        sql.str("");
        sql << "... skipping '" << it->sqldef << "'";
        logMsg(sql.str());
        continue;
      }

      pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));

      if (it->name == "BIT")
      {
        pstmt->setLong(1, std::stoull(it->value));
        ASSERT_EQUALS(1, pstmt->executeUpdate());
      }
      else
      {
        pstmt->setString(1, it->value);
        ASSERT_EQUALS(1, pstmt->executeUpdate());

        pstmt->clearParameters();
        pstmt->setDateTime(1, it->value);
        ASSERT_EQUALS(1, pstmt->executeUpdate());

        pstmt->clearParameters();
        pstmt->setBigInt(1, it->value);
        ASSERT_EQUALS(1, pstmt->executeUpdate());
      }
      

      pstmt->clearParameters();
      try
      {
        pstmt->setString(0, "overflow");
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setString(2, "invalid");
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setBigInt(0, it->value);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setBigInt(2, it->value);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      // enum won't accept 0 as a value
      pstmt->setBoolean(1, it->name.compare("ENUM") == 0);
      ASSERT_EQUALS(1, pstmt->executeUpdate());

      pstmt->clearParameters();
      try
      {
        pstmt->setBoolean(0, false);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setBoolean(2, false);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setDateTime(0, it->value);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setDateTime(2, it->value);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      pstmt->setDouble(1, 1.23);

      if (it->name != "TIMESTAMP" && it->name != "DATETIME" && it->name != "DATE")
      {
        try
        {
          ASSERT_EQUALS(1, pstmt->executeUpdate());
        }
        catch(sql::SQLException& e)
        {
          if (!((it->name == "VARBINARY" || it->name == "BINARY" || it->name == "CHAR" || it->name == "VARCHAR") &&
            it->precision < 30 && e.getSQLState() == "22001" && e.getErrorCode() == 1406))
          {
            TEST_THROW(sql::SQLException, e);
          }
        }

        pstmt->clearParameters();
        try
        {
          pstmt->setDouble(0, 1.23);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        try
        {
          pstmt->setDouble(2, 1.23);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        pstmt->setInt(1, 1);
        ASSERT_EQUALS(1, pstmt->executeUpdate());

        pstmt->clearParameters();
        try
        {
          pstmt->setInt(0, (int32_t)-1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        try
        {
          pstmt->setInt(2, (int32_t)-1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        pstmt->setUInt(1, (uint32_t)1);
        ASSERT_EQUALS(1, pstmt->executeUpdate());

        pstmt->clearParameters();
        try
        {
          pstmt->setUInt(0, (uint32_t)1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        try
        {
          pstmt->setUInt(2, (uint32_t)1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        pstmt->setInt64(1, 1);
        ASSERT_EQUALS(1, pstmt->executeUpdate());

        pstmt->clearParameters();
        pstmt->setUInt64(1, (uint64_t)1);
        ASSERT_EQUALS(1, pstmt->executeUpdate());

        pstmt->clearParameters();
        try
        {
          pstmt->setUInt64(0, (uint64_t)1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        try
        {
          pstmt->setUInt64(2, (uint64_t)-1);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }
      }

      if (it->is_nullable)
      {
        pstmt->clearParameters();
        pstmt->setNull(1, it->ctype);
        ASSERT_EQUALS(1, pstmt->executeUpdate());

        pstmt->clearParameters();
        try
        {
          pstmt->setNull(0, it->ctype);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }

        pstmt->clearParameters();
        try
        {
          pstmt->setNull(2, it->ctype);
          FAIL("Invalid argument not detected");
        }
        catch (sql::SQLException&)
        {
        }
      }

      pstmt->clearParameters();
      pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));
      std::stringstream blob_input_stream;
      blob_input_stream.str(it->value);
      pstmt->setBlob(1, &blob_input_stream);
      try
      {
        ASSERT_EQUALS(1, pstmt->executeUpdate());
      }
      catch (sql::SQLException & e)
      {
        if (!(e.getErrorCode() == 1265 || e.getErrorCode() == 1406) || (it->name.compare("SET") != 0 && it->name.compare("ENUM") != 0 && it->name.compare("BIT") != 0))
        {
          throw e;
        }
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setBlob(0, &blob_input_stream);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt->clearParameters();
      try
      {
        pstmt->setBlob(2, &blob_input_stream);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLException&)
      {
      }

      pstmt.reset(con->prepareStatement("SELECT COUNT(IFNULL(id, 1)) AS _num FROM test"));
      res.reset(pstmt->executeQuery());
      checkResultSetScrolling(res);
      ASSERT(res->next());
      if (res->getInt("_num") != (11 + (int) it->is_nullable))
      {
        sql.str("");
        sql << "....\t\tWARNING, SQL: " << it->sqldef << ", nullable " << std::boolalpha;
        sql << it->is_nullable << ", found " << res->getInt(1) << "columns but";
        sql << " expecting " << (11 + (int) it->is_nullable);
        logMsg(sql.str());
        got_warning=true;
      }

    }
    stmt->execute("DROP TABLE IF EXISTS test");
    if (got_warning)
    {
      SKIP("There were warnings, but that is due to changes in the test, that made detecting of warnings obsolete");
    }

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    std::string message("The error occured on type:");
    message.append(it->sqldef).append(".").append(e.what());
    fail(message.c_str(), __FILE__, __LINE__);
  }
}

void preparedstatement::setNull()
{
  logMsg("preparedstatement::setNull() - MySQL_PreparedStatement::*");

  std::stringstream sql;
  stmt.reset(con->createStatement());

  try
  {
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id INT)");
    pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));
    pstmt->setNull(1, sql::DataType::INTEGER);
    ASSERT_EQUALS(1, pstmt->executeUpdate());

    pstmt.reset(con->prepareStatement("SELECT id FROM test"));
    res.reset(pstmt->executeQuery());
    checkResultSetScrolling(res);
    ASSERT(res->next());
    ASSERT(res->isNull(1));
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }

  try
  {
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id INT NOT NULL)");
    pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));
    pstmt->setNull(1, sql::DataType::INTEGER);
    pstmt->executeUpdate();
    FAIL("Should fail");
  }
  catch (sql::SQLException &)
  {
  }

}

void preparedstatement::checkClosed()
{
  logMsg("preparedstatement::checkClosed() - MySQL_PreparedStatement::close()");

  try
  {
    pstmt.reset(con->prepareStatement("SELECT 1"));
    pstmt->close();
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }

}

void preparedstatement::getMetaData()
{
  logMsg("preparedstatement::getMetaData() - MySQL_PreparedStatement::getMetaData()");

  std::stringstream sql;
  std::vector<columndefinition>::iterator it;
  stmt.reset(con->createStatement());
  ResultSetMetaData meta_ps;
  ResultSetMetaData meta_st;
  ResultSet res_st;
  bool got_warning=false;
  unsigned int i;

  try
  {

    for (it=columns.end(), it--; it != columns.begin(); it--)
    {
      stmt->execute("DROP TABLE IF EXISTS test");
      sql.str("");
      sql << "CREATE TABLE test(dummy TIMESTAMP, id " << it->sqldef << ")";
      try
      {
        stmt->execute(sql.str());
        sql.str("");
        sql << "... testing '" << it->sqldef << "'";
        logMsg(sql.str());
      }
      catch (sql::SQLException &)
      {
        sql.str("");
        sql << "... skipping '" << it->sqldef << "'";
        logMsg(sql.str());
        continue;
      }

      pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));
      if (it->name == "BIT")
      {
        pstmt->setLong(1, std::stoull(it->value));
      }
      else
      {
        pstmt->setString(1, it->value);
      }
      ASSERT_EQUALS(1, pstmt->executeUpdate());

      pstmt.reset(con->prepareStatement("SELECT id, dummy, NULL, -1.1234, 'Warum nicht...' FROM test"));
      res.reset(pstmt->executeQuery());
      meta_ps.reset(res->getMetaData());

      res_st.reset(stmt->executeQuery("SELECT id, dummy, NULL, -1.1234, 'Warum nicht...' FROM test"));
      meta_st.reset(res->getMetaData());

      ASSERT_EQUALS(meta_ps->getColumnCount(), meta_st->getColumnCount());

      for (i=1; i <= meta_ps->getColumnCount(); i++)
      {
        ASSERT_EQUALS(meta_ps->getCatalogName(i), meta_st->getCatalogName(i));
        ASSERT_EQUALS(meta_ps->getColumnDisplaySize(i), meta_st->getColumnDisplaySize(i));
        ASSERT_EQUALS(meta_ps->getColumnLabel(i), meta_st->getColumnLabel(i));
        ASSERT_EQUALS(meta_ps->getColumnName(i), meta_st->getColumnName(i));
        ASSERT_EQUALS(meta_ps->getColumnType(i), meta_st->getColumnType(i));
        ASSERT_EQUALS(meta_ps->getColumnTypeName(i), meta_st->getColumnTypeName(i));
        ASSERT_EQUALS(meta_ps->getPrecision(i), meta_st->getPrecision(i));
        ASSERT_EQUALS(meta_ps->getScale(i), meta_st->getScale(i));
        ASSERT_EQUALS(meta_ps->getSchemaName(i), meta_st->getSchemaName(i));
        ASSERT_EQUALS(meta_ps->getTableName(i), meta_st->getTableName(i));
        ASSERT_EQUALS(meta_ps->isAutoIncrement(i), meta_st->isAutoIncrement(i));
        ASSERT_EQUALS(meta_ps->isCaseSensitive(i), meta_st->isCaseSensitive(i));
        ASSERT_EQUALS(meta_ps->isCurrency(i), meta_st->isCurrency(i));
        ASSERT_EQUALS(meta_ps->isDefinitelyWritable(i), meta_st->isDefinitelyWritable(i));
        ASSERT_EQUALS(meta_ps->isNullable(i), meta_st->isNullable(i));
        ASSERT_EQUALS(meta_ps->isReadOnly(i), meta_st->isReadOnly(i));
        ASSERT_EQUALS(meta_ps->isSearchable(i), meta_st->isSearchable(i));
        ASSERT_EQUALS(meta_ps->isSigned(i), meta_st->isSigned(i));
        ASSERT_EQUALS(meta_ps->isWritable(i), meta_st->isWritable(i));
      }

      try
      {
        meta_ps->getCatalogName(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

    }
    stmt->execute("DROP TABLE IF EXISTS test");
    if (got_warning)
      FAIL("See warnings");

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

bool preparedstatement::createSP(std::string sp_code)
{
  try
  {
    stmt.reset(con->createStatement());
    stmt->execute("DROP PROCEDURE IF EXISTS p");
  }
  catch (sql::SQLException &e)
  {
    logMsg(e.what());
    return false;
  }

  try
  {
    pstmt.reset(con->prepareStatement(sp_code));
    ASSERT(!pstmt->execute());
    logMsg("... using PS for everything");
  }
  catch (sql::SQLException &e)
  {
    if (e.getErrorCode() != 1295)
    {
      logErr(e.what());
      std::stringstream msg;
      msg.str("");
      msg << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
      logErr(msg.str());
      fail(e.what(), __FILE__, __LINE__);
    }
    stmt->execute(sp_code);
  }

  return true;
}

void preparedstatement::callSP()
{
  logMsg("preparedstatement::callSP() - MySQL_PreparedStatement::*()");
  std::string sp_code("CREATE PROCEDURE p(OUT ver_param VARCHAR(250)) BEGIN SELECT VERSION() INTO ver_param; END;");
  DatabaseMetaData dbmeta(con->getMetaData());
  bool autoCommit= con->getAutoCommit();

  /* Version on the server can be different from the one reported by MaxScale. And we are testing here SP, not the connection metadata */
  if (isSkySqlHA() || isMaxScale())
  {
    sp_code= "CREATE PROCEDURE p(OUT ver_param VARCHAR(250)) BEGIN SELECT '" + dbmeta->getDatabaseProductVersion() + "' INTO ver_param; END;";
  }

  try
  {
    if (!createSP(sp_code))
    {
      logMsg("... skipping:");
      return;
    }
    if (isSkySqlHA() || isMaxScale())
    {
      con->setAutoCommit(false);
    }
    try
    {
      pstmt.reset(con->prepareStatement("CALL p(@version)"));
      ASSERT(!pstmt->execute());
      ASSERT(!pstmt->execute());
    }
    catch (sql::SQLException & e)
    {
      if (isSkySqlHA() || isMaxScale())
      {
        con->rollback();
        con->setAutoCommit(autoCommit);
      }
      if (e.getErrorCode() != 1295)
      {
        logErr(e.what());
        std::stringstream msg;
        msg.str("");
        msg << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
        logErr(msg.str());
        fail(e.what(), __FILE__, __LINE__);
      }
      // PS protocol does not support CALL
      return;
    }

    pstmt.reset(con->prepareStatement("SELECT @version AS _version"));
    res.reset(pstmt->executeQuery());
    ASSERT(res->next());
    ASSERT_EQUALS(dbmeta->getDatabaseProductVersion(), res->getString("_version"));

    pstmt.reset(con->prepareStatement("SET @version='no_version'"));
    ASSERT(!pstmt->execute());
    pstmt.reset(con->prepareStatement("CALL p(@version)"));
    ASSERT(!pstmt->execute());
    pstmt.reset(con->prepareStatement("SELECT @version AS _version"));
    res.reset(pstmt->executeQuery());
    ASSERT(res->next());
    ASSERT_EQUALS(dbmeta->getDatabaseProductVersion(), res->getString("_version"));
    if (isSkySqlHA() || isMaxScale())
    {
      con->commit();
      con->setAutoCommit(autoCommit);
    }
  }
  catch (sql::SQLException &e)
  {
    if (isSkySqlHA() || isMaxScale())
    {
      con->rollback();
      con->setAutoCommit(autoCommit);
    }
    logErr(e.what());
    std::stringstream msg;
    msg.str("");
    msg << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
    logErr(msg.str());
    fail(e.what(), __FILE__, __LINE__);
  }
}

void preparedstatement::callSPInOut()
{
  logMsg("preparedstatement::callSPInOut() - MySQL_PreparedStatement::*()");
  std::string sp_code("CREATE PROCEDURE p(IN ver_in VARCHAR(25), OUT ver_out VARCHAR(25)) BEGIN SELECT ver_in INTO ver_out; END;");
  bool autoCommit = con->getAutoCommit();

  /* Version on the server can be different from the one reported by MaxScale. And we are testing here SP, not the connection metadata */
  if (isSkySqlHA() || isMaxScale())
  {
    con->setAutoCommit(false);
  }
  try
  {
    if (!createSP(sp_code))
    {
      logMsg("... skipping: cannot create SP");
      if (isSkySqlHA() || isMaxScale())
      {
        con->setAutoCommit(autoCommit);
      }
      return;
    }

    try
    {
      cstmt.reset(con->prepareCall("CALL p('myver', @version)"));
      ASSERT(!cstmt->execute());
    }
    catch (sql::SQLException &e)
    {
      if (isSkySqlHA() || isMaxScale())
      {
        con->setAutoCommit(autoCommit);
      }
      if (e.getErrorCode() != 1295)
      {
        logErr(e.what());
        std::stringstream msg1;
        msg1.str("");
        msg1 << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
        logErr(msg1.str());
        fail(e.what(), __FILE__, __LINE__);
      }
      // PS protocol does not support CALL
      logMsg("... skipping: PS protocol does not support CALL");
      return;
    }
    pstmt.reset(con->prepareStatement("SELECT @version AS _version"));
    res.reset(pstmt->executeQuery());
    ASSERT(res->next());
    ASSERT_EQUALS("myver", res->getString("_version"));

    if (isSkySqlHA() || isMaxScale())
    {
      con->setAutoCommit(autoCommit);
    }
  }
  catch (sql::SQLException &e)
  {
    if (isSkySqlHA() || isMaxScale())
    {
      con->setAutoCommit(autoCommit);
    }
    logErr(e.what());
    std::stringstream msg2;
    msg2.str("");
    msg2 << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
    logErr(msg2.str());
    fail(e.what(), __FILE__, __LINE__);
  }
}

void preparedstatement::callSPWithPS()
{
  logMsg("preparedstatement::callSPWithPS() - MySQL_PreparedStatement::*()");

  try
  {
    std::string sp_code("CREATE PROCEDURE p(IN val VARCHAR(25)) BEGIN SET @sql = CONCAT('SELECT \"', val, '\"'); PREPARE stmt FROM @sql; EXECUTE stmt; DROP PREPARE stmt; END;");
    if (!createSP(sp_code))
    {
      logMsg("... skipping:");
      return;
    }

    try
    {
      cstmt.reset(con->prepareCall("CALL p('abc')"));
      res.reset(cstmt->executeQuery());
    }
    catch (sql::SQLException &e)
    {
      if (e.getErrorCode() != 1295)
      {
        logErr(e.what());
        std::stringstream msg1;
        msg1.str("");
        msg1 << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
        logErr(msg1.str());
        fail(e.what(), __FILE__, __LINE__);
      }
      // PS interface cannot call this kind of statement
      return;
    }
    ASSERT(res->next());
    ASSERT_EQUALS("abc", res->getString(1));
    std::stringstream msg2;
    msg2.str("");
    msg2 << "... val = '" << res->getString(1) << "'";
    logMsg(msg2.str());

    while(cstmt->getMoreResults())
    {}

    try
    {
      cstmt.reset(con->prepareCall("CALL p(?)"));
      cstmt->setString(1, "123");
      res.reset(cstmt->executeQuery());
      ASSERT(res->next());
      ASSERT_EQUALS("123", res->getString(1));
      ASSERT(!res->next());
    }
    catch (sql::SQLException &e)
    {
      if (e.getErrorCode() != 1295)
      {
        logErr(e.what());
        std::stringstream msg3;
        msg3.str("");
        msg3 << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
        logErr(msg3.str());
        fail(e.what(), __FILE__, __LINE__);
      }
      // PS interface cannot call this kind of statement
      return;
    }
    res->close();
  }
  catch (sql::SQLException &e)
  {
    if (e.getErrorCode() != 1295)
    {
      logErr(e.what());
      std::stringstream msg4;
      msg4.str("");
      msg4 << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
      logErr(msg4.str());
      fail(e.what(), __FILE__, __LINE__);
    }
  }
}

void preparedstatement::callSPMultiRes()
{
  logMsg("preparedstatement::callSPMultiRes() - MySQL_PreparedStatement::*()");
  try
  {
    std::string sp_code("CREATE PROCEDURE p() BEGIN SELECT 1; SELECT 2; SELECT 3; END;");
    if (!createSP(sp_code))
    {
      logMsg("... skipping:");
      return;
    }

    try
    {
      pstmt.reset(con->prepareStatement("CALL p()"));
      ASSERT(pstmt->execute());
    }
    catch (sql::SQLException &e)
    {
      if (e.getErrorCode() != 1295)
      {
        logErr(e.what());
        std::stringstream msg1;
        msg1.str("");
        msg1 << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
        logErr(msg1.str());
        fail(e.what(), __FILE__, __LINE__);
      }
      // PS interface cannot call this kind of statement
      return;
    }

    // Should not work prior to MySQL 6.0
    std::stringstream msg2;
    msg2.str("");
    do
    {
      res.reset(pstmt->getResultSet());
      while (res->next())
      {
        msg2 << res->getString(1);
      }
    }
    while (pstmt->getMoreResults());

    ASSERT_EQUALS("123", msg2.str());
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    std::stringstream msg3;
    msg3.str("");
    msg3 << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
    logErr(msg3.str());
    fail(e.what(), __FILE__, __LINE__);
  }
}

void preparedstatement::anonymousSelect()
{
  logMsg("preparedstatement::anonymousSelect() - MySQL_PreparedStatement::*, MYSQL_PS_Resultset::*");

  try
  {
    pstmt.reset(con->prepareStatement("SELECT ' ', NULL"));
    res.reset(pstmt->executeQuery());
    ASSERT(res->next());
    ASSERT_EQUALS(" ", res->getString(1));

    std::string mynull(res->getString(2));
    ASSERT(res->isNull(2));
    ASSERT(res->wasNull());

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

void preparedstatement::crash()
{
  logMsg("preparedstatement::crash() - MySQL_PreparedStatement::*");

  try
  {
    stmt.reset(con->createStatement());
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(dummy TIMESTAMP, id VARCHAR(1))");
    pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));

    pstmt->clearParameters();
    pstmt->setDouble(1, 1.23);
    ASSERT_EQUALS(1, pstmt->executeUpdate());
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + e.getSQLState());
    ASSERT_EQUALS("22001", e.getSQLState());
    ASSERT_EQUALS(1406, e.getErrorCode());
    logErr("Error of field overflow is expected");
  }
}

void preparedstatement::getWarnings()
{
  logMsg("preparedstatement::getWarnings() - MySQL_PreparedStatement::get|clearWarnings()");

  //TODO: Enable it after fixing
  SKIP("Testcase needs to be fixed");

  std::stringstream msg;

  stmt.reset(con->createStatement());
  try
  {
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id INT UNSIGNED)");

    // Generating 2  warnings to make sure we get only the last 1 - won't hurt
    // Lets hope that this will always cause a 1264 or similar warning
    pstmt.reset(con->prepareStatement("INSERT INTO test(id) VALUES (?)"));
    pstmt->setInt(1, -2);
    pstmt->executeUpdate();
    pstmt->setInt(1, -1);
    pstmt->executeUpdate();

    int count= 0;
    for (const sql::SQLWarning* warn=pstmt->getWarnings(); warn; warn=warn->getNextWarning())
    {
      msg.str("");
      msg << "... ErrorCode = '" << warn->getErrorCode() << "', ";
      msg << "SQLState = '" << warn->getSQLState() << "', ";
      msg << "ErrorMessage = '" << warn->getMessage() << "'";
      logMsg(msg.str());

      ASSERT((0 != warn->getErrorCode()));
      if (1264 == warn->getErrorCode())
      {
        ASSERT_EQUALS("22003", warn->getSQLState());
      }
      else
      {
        ASSERT(("" != warn->getSQLState()));
      }
      ASSERT(("" != warn->getMessage()));

    ++count;
    }

  ASSERT_EQUALS(1, count);

    for (const sql::SQLWarning* warn=pstmt->getWarnings(); warn; warn=warn->getNextWarning())
    {
      msg.str("");
      msg << "... ErrorCode = '" << warn->getErrorCode() << "', ";
      msg << "SQLState = '" << warn->getSQLState() << "', ";
      msg << "ErrorMessage = '" << warn->getMessage() << "'";
      logMsg(msg.str());

      ASSERT((0 != warn->getErrorCode()));
      if (1264 == warn->getErrorCode())
      {
        ASSERT_EQUALS("22003", warn->getSQLState());
      }
      else
      {
        ASSERT(("" != warn->getSQLState()));
      }
      ASSERT(("" != warn->getMessage()));
    }

    pstmt->clearWarnings();
    for (const sql::SQLWarning* warn=pstmt->getWarnings(); warn; warn=warn->getNextWarning())
    {
      FAIL("There should be no more warnings!");
    }

  pstmt->setInt(1, -3);
  pstmt->executeUpdate();
  ASSERT(pstmt->getWarnings() != NULL);
  // Statement without tables access does not reset warnings.
  pstmt.reset(con->prepareStatement("SELECT 1"));
    res.reset(pstmt->executeQuery());
  ASSERT(pstmt->getWarnings() == NULL);
    ASSERT(res->next());

    // TODO - how to use getNextWarning() ?
    stmt->execute("DROP TABLE IF EXISTS test");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

void preparedstatement::blob()
{
  logMsg("preparedstatement::blob() - MySQL_PreparedStatement::*");

  char blob_input[512];
  std::stringstream blob_input_stream;
  std::stringstream msg;
  char blob_output[512];
  int id;
  int offset=0;

  try
  {
    pstmt.reset(con->prepareStatement("DROP TABLE IF EXISTS test"));
    pstmt->execute();

    pstmt.reset(con->prepareStatement("CREATE TABLE test(id INT, col1 TINYBLOB, col2 TINYBLOB)"));
    pstmt->execute();

    // Most basic INSERT/SELECT...
    pstmt.reset(con->prepareStatement("INSERT INTO test(id, col1) VALUES (?, ?)"));

    for (char ascii_code=CHAR_MIN; ascii_code < CHAR_MAX; ascii_code++)
    {
      blob_output[offset]='\0';
      blob_input[offset++]=ascii_code;
    }
    blob_input[offset]='\0';
    blob_output[offset]='\0';
    for (char ascii_code=CHAR_MAX; ascii_code > CHAR_MIN; ascii_code--)
    {
      blob_output[offset]='\0';
      blob_input[offset++]=ascii_code;
    }
    blob_input[offset]='\0';
    blob_output[offset]='\0';

    id=1;
    blob_input_stream << blob_input;

    pstmt->setInt(1, id);
    pstmt->setBlob(2, &blob_input_stream);
    try
    {
      pstmt->setBlob(3, &blob_input_stream);
      FAIL("Invalid index not detected");
    }
    catch (sql::SQLException&)
    {
    }

    pstmt->execute();
    pstmt.reset(con->prepareStatement("SELECT id, col1 FROM test WHERE id = 1"));
    res.reset(pstmt->executeQuery());

    ASSERT(res->next());

    msg.str("");
    msg << "... simple INSERT/SELECT, '" << std::endl << blob_input << std::endl << "' =? '" << std::endl << res->getString(2) << "'";
    logMsg(msg.str());

    ASSERT_EQUALS(res->getInt(1), id);
    ASSERT_EQUALS(res->getString(2), blob_input_stream.str());
    ASSERT_EQUALS(res->getString(2), blob_input);
    ASSERT_EQUALS(res->getString("col1"), blob_input_stream.str());
    ASSERT_EQUALS(res->getString("col1"), blob_input);

    std::unique_ptr< std::istream > blob_output_stream(res->getBlob(2));
    blob_output_stream->seekg(std::ios::beg);
    blob_output_stream->get(blob_output, offset + 1);
    ASSERT_EQUALS(blob_input_stream.str(), blob_output);

    blob_output_stream.reset(res->getBlob("col1"));
    blob_output_stream->seekg(std::ios::beg);
    blob_output_stream->get(blob_output, offset + 1);
    ASSERT_EQUALS(blob_input, blob_output);

    msg.str("");
    msg << "... second check, '"<< std::endl  << blob_input << std::endl << "' =? '" << std::endl << blob_output << "'";
    logMsg(msg.str());

    ASSERT(!res->next());
    res->close();

    msg.str("");
    // Data is too long to be stored in a TINYBLOB column
    msg << "... this is more than TINYBLOB can hold: ";
    msg << "01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
    pstmt.reset(con->prepareStatement("INSERT INTO test(id, col1) VALUES (?, ?)"));
    id=2;
    pstmt->setInt(1, id);
    pstmt->setBlob(2, &msg);
    try {
      pstmt->execute();
      pstmt.reset(con->prepareStatement("SELECT id, col1 FROM test WHERE id = 2"));
      res.reset(pstmt->executeQuery());
      ASSERT(res->next());
      ASSERT_EQUALS(res->getInt(1), id);
      ASSERT_GT((int)(res->getString(2).length()), (int)(msg.str().length()));
      ASSERT(!res->next());
      res->close();

      msg << "- what has happened to the stream?";
      logMsg(msg.str());
    }
    catch (sql::SQLException & e) {
      // If the server is in the strict mode, inserting too long data causes error
      if (e.getErrorCode() == 1406 && e.getSQLState().compare("22001") == 0) {
        logMsg("The server is in the strict mode - couldn't insert too long stream");
      }
      else {
        logErr(e.what());
        logErr("SQLState: " + std::string(e.getSQLState()));
        fail(e.what(), __FILE__, __LINE__);
      }
    }

    pstmt.reset(con->prepareStatement("DROP TABLE IF EXISTS test"));
    pstmt->execute();

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }

}

void preparedstatement::executeQuery()
{
  logMsg("preparedstatement::executeQuery() - MySQL_PreparedStatement::executeQuery");
  try
  {
    const sql::SQLString option("defaultPreparedStatementResultType");
    int value=sql::ResultSet::TYPE_FORWARD_ONLY;
    con->setClientOption(option, static_cast<void *> (&value));
  }
  catch (sql::MethodNotImplementedException &/*e*/)
  {
    /* not available */
    return;
  }
  try
  {
    stmt.reset(con->createStatement());
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id INT UNSIGNED)");
    stmt->execute("INSERT INTO test(id) VALUES (1), (2), (3)");

    pstmt.reset(con->prepareStatement("SELECT id FROM test ORDER BY id ASC"));
    res.reset(pstmt->executeQuery());
    ASSERT(res->next());
    ASSERT_EQUALS(res->getInt("id"), 1);

    pstmt.reset(con->prepareStatement("DROP TABLE IF EXISTS test"));
    pstmt->execute();
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    std::stringstream msg;
    msg.str("");
    msg << "SQLState: " << e.getSQLState() << ", MySQL error code: " << e.getErrorCode();
    logErr(msg.str());
    fail(e.what(), __FILE__, __LINE__);
  }
}


void preparedstatement::addBatch()
{
  stmt->executeUpdate("DROP TABLE IF EXISTS testAddBatchPs");
  stmt->executeUpdate("CREATE TABLE testAddBatchPs "
    "(id int not NULL)");

  pstmt.reset(con->prepareStatement("INSERT INTO testAddBatchPs VALUES(?)"));
  pstmt->setInt(1, 1);
  pstmt->addBatch();
  pstmt->setInt(1, 2);
  pstmt->addBatch();
  pstmt->setInt(1, 3);
  pstmt->addBatch();

  const sql::Ints& batchRes = pstmt->executeBatch();

  res.reset(stmt->executeQuery("SELECT MIN(id), MAX(id), SUM(id), count(*) FROM testAddBatchPs"));

  ASSERT(res->next());

  ASSERT_EQUALS(1, res->getInt(1));
  ASSERT_EQUALS(3, res->getInt(2));
  ASSERT_EQUALS(6, res->getInt(3));
  ASSERT_EQUALS(3, res->getInt(4));
  ASSERT_EQUALS(3ULL, static_cast<uint64_t>(batchRes.size()));
  ASSERT_EQUALS(1, batchRes[0]);
  ASSERT_EQUALS(1, batchRes[1]);
  ASSERT_EQUALS(1, batchRes[2]);

  ////// The same, but for executeLargeBatch
  stmt->executeUpdate("DELETE FROM testAddBatchPs");

  pstmt->clearBatch();
  pstmt->clearParameters();

  pstmt->setInt(1, 4);
  pstmt->addBatch();
  pstmt->setInt(1, 5);
  pstmt->addBatch();
  pstmt->setInt(1, 6);
  pstmt->addBatch();

  const sql::Longs& batchLRes = pstmt->executeLargeBatch();

  res.reset(stmt->executeQuery("SELECT MIN(id), MAX(id), SUM(id), count(*) FROM testAddBatchPs"));

  ASSERT(res->next());

  ASSERT_EQUALS(4, res->getInt(1));
  ASSERT_EQUALS(6, res->getInt(2));
  ASSERT_EQUALS(15, res->getInt(3));
  ASSERT_EQUALS(3, res->getInt(4));
  ASSERT_EQUALS(3ULL, static_cast<uint64_t>(batchLRes.size()));
  ASSERT_EQUALS(1LL, batchLRes[0]);
  ASSERT_EQUALS(1LL, batchLRes[1]);
  ASSERT_EQUALS(1LL, batchLRes[2]);

  stmt->executeUpdate("DROP TABLE testAddBatchPs");
}

/* Connector wasn't precise enough with double numbers operations and could lose significant digits */
void preparedstatement::bugConcpp96()
{
  createSchemaObject("TABLE", "bugConcpp96", "(`as_double` double NOT NULL, `as_string` varchar(20) NOT NULL, `as_decimal` decimal(15,4) NOT NULL)");

  sql::SQLString selectQuery("SELECT CAST(? AS DOUBLE), ?, CAST(? AS DECIMAL(15,4))");

  pstmt.reset(con->prepareStatement(selectQuery));
  ssps.reset(sspsCon->prepareStatement(selectQuery));
  PreparedStatement ssps2(sspsCon->prepareStatement("INSERT INTO bugConcpp96 (as_double, as_string, as_decimal) VALUES (?, ?, ?)"));

  double numberToInsert(98.765432109), epsilon(0.0001);

  do {
    pstmt->setDouble(1, numberToInsert);
    pstmt->setString(2, std::to_string(numberToInsert));
    pstmt->setDouble(3, numberToInsert);
    res.reset(pstmt->executeQuery());
    ASSERT(res->next());
    ASSERT_EQUALS_EPSILON(numberToInsert, res->getDouble(1), epsilon);
    ASSERT_EQUALS(std::to_string(numberToInsert), res->getString(2));
    ASSERT_EQUALS_EPSILON(numberToInsert, res->getDouble(3), epsilon);

    ssps->setDouble(1, numberToInsert);
    ssps->setString(2, std::to_string(numberToInsert));
    ssps->setDouble(3, numberToInsert);
    ResultSet res2(ssps->executeQuery());
    ASSERT(res2->next());
    ASSERT_EQUALS_EPSILON(numberToInsert, res2->getDouble(1), epsilon);
    ASSERT_EQUALS(std::to_string(numberToInsert), res2->getString(2));
    ASSERT_EQUALS_EPSILON(numberToInsert, res2->getDouble(3), epsilon);

    ssps2->setDouble(1, numberToInsert);
    ssps2->setString(2, std::to_string(numberToInsert));
    ssps2->setDouble(3, numberToInsert);
    ASSERT_EQUALS(1, ssps2->executeUpdate());

    numberToInsert*= 10.0;
  } while (numberToInsert < 1000000000);

  res.reset(stmt->executeQuery("SELECT as_double, as_string, as_decimal FROM bugConcpp96 ORDER BY 1 DESC"));
  
  while (res->next()) {
    // Initially the value is 10 times bigger as the last inserted value
    numberToInsert/= 10.0;
    ASSERT_EQUALS_EPSILON(numberToInsert, res->getDouble(1), epsilon);
    ASSERT_EQUALS(std::to_string(numberToInsert), res->getString(2));
    ASSERT_EQUALS_EPSILON(numberToInsert, res->getDouble(3), epsilon);
    
  }
}

/** Test of rewriteBatchedStatements option. The test does cannot test if the batch is really rewritten, though. 
 */
void preparedstatement::concpp99_batchRewrite()
{
  sql::ConnectOptionsMap connection_properties{{"userName", user}, {"password", passwd}, {"rewriteBatchedStatements", "true"}, {"useTls", useTls ? "true" : "false"}};

  con.reset(driver->connect(url, connection_properties));
  stmt.reset(con->createStatement());
  createSchemaObject("TABLE", "concpp99_batchRewrite", "(id int not NULL PRIMARY KEY, val VARCHAR(31) NOT NULL DEFAULT '')");

  const sql::SQLString insertQuery[]{"INSERT INTO concpp99_batchRewrite VALUES(?,?)",
                                     "INSERT INTO concpp99_batchRewrite(id) VALUES(?) ON DUPLICATE KEY UPDATE val=?"};
  const int32_t id[]{1, 2, 3}, batchResult[]{sql::Statement::SUCCESS_NO_INFO, 1};
  const sql::SQLString val[][3]{{"X'1", "y\"2", "xxx"}, {"","",""}},
    selectQuery("SELECT id, val FROM concpp99_batchRewrite ORDER BY id"),
    deleteQuery("DELETE FROM concpp99_batchRewrite");

  for (std::size_t i= 0; i < sizeof(insertQuery) / sizeof(insertQuery[0]); ++i) {
    pstmt.reset(con->prepareStatement(insertQuery[i]));
    //ssps.reset(sspsCon->prepareStatement(insertQuery[i]));

    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      pstmt->setInt(1, id[row]);
      pstmt->setString(2, val[i][row]);
      pstmt->addBatch();
      /*ssps->setInt(1, id[row]);
      ssps->setString(2, val[0][row]);
      ssps->addBatch();*/
    }

    const sql::Ints& batchRes = pstmt->executeBatch();
    ASSERT_EQUALS(static_cast<uint64_t>(sizeof(id) / sizeof(id[0])), static_cast<uint64_t>(batchRes.size()));

    res.reset(stmt->executeQuery(selectQuery));

    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      ASSERT(res->next());
      ASSERT_EQUALS(id[row], res->getInt(1));
      ASSERT_EQUALS(val[i][row], res->getString(2));
      // With rewriteBatchedStatements we don't have separate results for each parameters set - only SUCCESS_NO_INFO
      ASSERT_EQUALS(batchResult[i], batchRes[row]);
    }
    ASSERT(!res->next());
    ////// The same, but for executeLargeBatch
    stmt->executeUpdate(deleteQuery);
    //const sql::Ints& batchRes2= ssps->executeBatch();

    pstmt->clearBatch();
    pstmt->clearParameters();

    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      pstmt->setInt(1, id[row] + 3);
      pstmt->setString(2, val[0][row]);
      pstmt->addBatch();
    }
    const sql::Longs& batchLRes = pstmt->executeLargeBatch();
    ASSERT_EQUALS(3ULL, static_cast<uint64_t>(batchLRes.size()));

    res.reset(stmt->executeQuery(selectQuery));
    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      ASSERT(res->next());
      ASSERT_EQUALS(id[row] + 3, res->getInt(1));
      ASSERT_EQUALS(val[i][row], res->getString(2));
      // With rewriteBatchedStatements we don't have separate results for each parameters set - only SUCCESS_NO_INFO
      ASSERT_EQUALS(static_cast<int64_t>(batchResult[i]), batchLRes[row]);
    }
    ASSERT(!res->next());
    stmt->executeUpdate(deleteQuery);
  }
  // To make sure the framework provides next test with "standard" connection
  con.reset();
}


/** Test of useBulkStmts option. The test does cannot test if the batch is really rewritten, though.
 */
void preparedstatement::concpp106_batchBulk()
{
  sql::ConnectOptionsMap connection_properties{ {"userName", user}, {"password", passwd}, {"useBulkStmts", "true"}, {"useTls", useTls ? "true" : "false"} };

  con.reset(driver->connect(url, connection_properties));
  stmt.reset(con->createStatement());
  createSchemaObject("TABLE", "concpp106_batchBulk", "(id int not NULL PRIMARY KEY, val VARCHAR(31))");

  const sql::SQLString insertQuery[]{ "INSERT INTO concpp106_batchBulk VALUES(?,?)",
                                     "INSERT INTO concpp106_batchBulk(id) VALUES(?) ON DUPLICATE KEY UPDATE val=?" };
  const int32_t id[]{1, 2, 5, 3}, batchResult[]{ sql::Statement::SUCCESS_NO_INFO, sql::Statement::SUCCESS_NO_INFO }, id_expected[]{1,2,3,5};
  const char* val[][4]{{nullptr , "X'1", "y\"2", "xxx"}, {nullptr, nullptr, nullptr, nullptr}},
    *val_expected[][4]{{nullptr, "X'1", "xxx", "y\"2"}, {nullptr, nullptr, nullptr, nullptr}};
  const sql::SQLString selectQuery("SELECT id, val FROM concpp106_batchBulk ORDER BY id"),
    deleteQuery("DELETE FROM concpp106_batchBulk");

  for (std::size_t i = 0; i < sizeof(insertQuery) / sizeof(insertQuery[0]); ++i) {
    pstmt.reset(con->prepareStatement(insertQuery[i]));
    //ssps.reset(sspsCon->prepareStatement(insertQuery[i]));

    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      pstmt->setInt(1, id[row]);
      if (val[i][row] != nullptr) {
        pstmt->setString(2, val[0][row]);
      }
      else {
        pstmt->setNull(2, sql::Types::VARCHAR);
      }
      pstmt->addBatch();
      /*ssps->setInt(1, id[row]);
      ssps->setString(2, val[0][row]);
      ssps->addBatch();*/
    }

    logMsg("Executing batch");
    const sql::Ints& batchRes = pstmt->executeBatch();
    logMsg("Executing batch - finished");
    ASSERT_EQUALS(static_cast<uint64_t>(sizeof(id) / sizeof(id[0])), static_cast<uint64_t>(batchRes.size()));

    res.reset(stmt->executeQuery(selectQuery));

    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      ASSERT(res->next());
      ASSERT_EQUALS(id_expected[row], res->getInt(1));
      if (val_expected[i][row] == nullptr) {
        ASSERT(res->isNull(2));
        /* SQLString does not have 3rd "null" state, thus for null it returns empty string */
        ASSERT_EQUALS("", res->getString(2));
      }
      else {
        ASSERT_EQUALS(val_expected[i][row], res->getString(2));
      }
      // With rewriteBatchedStatements we don't have separate results for each parameters set - only SUCCESS_NO_INFO
      ASSERT_EQUALS(batchResult[i], batchRes[row]);
    }
    ASSERT(!res->next());
    ////// The same, but for executeLargeBatch
    stmt->executeUpdate(deleteQuery);
    //const sql::Ints& batchRes2= ssps->executeBatch();

    pstmt->clearBatch();
    pstmt->clearParameters();

    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      pstmt->setInt(1, id[row] + 3);
      if (val[i][row] != nullptr) {
        pstmt->setString(2, val[0][row]);
      }
      else {
        pstmt->setNull(2, sql::Types::VARCHAR);
      }
      pstmt->addBatch();
    }
    logMsg("Executing largeBatch");
    const sql::Longs& batchLRes = pstmt->executeLargeBatch();
    logMsg("Executing largeBatch - finished");
    ASSERT_EQUALS(static_cast<uint64_t>(sizeof(id) / sizeof(id[0])), static_cast<uint64_t>(batchLRes.size()));

    res.reset(stmt->executeQuery(selectQuery));
    for (int32_t row = 0; row < sizeof(id) / sizeof(id[0]); ++row) {
      ASSERT(res->next());
      ASSERT_EQUALS(id_expected[row] + 3, res->getInt(1));
      if (val_expected[i][row] == nullptr) {
        ASSERT(res->isNull(2));
        /* SQLString does not have 3rd "null" state, thus for null it returns empty string */
        ASSERT_EQUALS("", res->getString(2));
      }
      else {
        ASSERT_EQUALS(val_expected[i][row], res->getString(2));
      }
      // With rewriteBatchedStatements we don't have separate results for each parameters set - only SUCCESS_NO_INFO
      ASSERT_EQUALS(static_cast<int64_t>(batchResult[i]), batchLRes[row]);
    }
    ASSERT(!res->next());
    stmt->executeUpdate(deleteQuery);
  }
  // To make sure the framework provides next test with "standard" connection
  con.reset();
}

} /* namespace preparedstatement */
} /* namespace testsuite */
