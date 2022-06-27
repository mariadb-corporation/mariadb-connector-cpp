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


#include <cctype>
#include <algorithm>
#include <sstream>

#include "ResultSet.hpp"

#include "MariaDbDatabaseMetaData.h"
#include "MariaDbConnection.h"
#include "ColumnType.h"
#include "SelectResultSet.h"
#include "ColumnDefinition.h"
#include "util/Utils.h"


#define IMPORTED_KEYS_COLUMN_COUNT 14
#define TYPE_INFO_COLUMN_COUNT 18
#define CLIENT_INFO_COLUMN_COUNT 4

namespace sql
{
  namespace mariadb
  {

    const SQLString MariaDbDatabaseMetaData::DRIVER_NAME("MariaDB Connector/C++");
    /**
     * Constructor.
     *
     * @param connection connection
     * @param urlParser Url parser
     */
    MariaDbDatabaseMetaData::MariaDbDatabaseMetaData(Connection* connection, const UrlParser& urlParser)
      : connection(dynamic_cast<MariaDbConnection*>(connection)),
        urlParser(urlParser),
        datePrecisionColumnExist(false)
    {
    }

    SQLString MariaDbDatabaseMetaData::columnTypeClause(Shared::Options& options){
      SQLString upperCaseWithoutSize =
        " UCASE(IF( COLUMN_TYPE LIKE '%(%)%', CONCAT(SUBSTRING( COLUMN_TYPE,1, LOCATE('(',"
        "COLUMN_TYPE) - 1 ), SUBSTRING(COLUMN_TYPE ,1+locate(')', COLUMN_TYPE))), "
        "COLUMN_TYPE))";

      if (options->tinyInt1isBit){
        upperCaseWithoutSize =
          " IF(COLUMN_TYPE like 'tinyint(1)%', 'BIT', "+upperCaseWithoutSize +")";
      }

      if (!options->yearIsDateType){
        return " IF(COLUMN_TYPE IN ('year(2)', 'year(4)'), 'SMALLINT', "+upperCaseWithoutSize +")";
      }

      return upperCaseWithoutSize;
    }

    // Return new position, or -1 on error
    std::size_t MariaDbDatabaseMetaData::skipWhite(const SQLString& part, std::size_t startPos)
    {
    for (size_t i= startPos; i < part.length();i++)
    {
      if (!std::isspace(part.at(i))){
        return i;
      }
    }
    return part.length();
  }

    std::size_t MariaDbDatabaseMetaData::parseIdentifier(const SQLString& part, std::size_t startPos, Identifier& identifier)
  {
    std::size_t pos= skipWhite(part, startPos);
    if (part.at(pos) != '`'){
      throw ParseException(part, pos);
    }
    pos++;
    SQLString sb;
    int32_t quotes= 0;

    for (; pos < part.length(); ++pos)
    {
      char ch= part.at(pos);
      if (ch == '`'){
        quotes++;
      }else
      {
        for (int32_t j= 0; j < quotes / 2; ++j){
          sb.append('`');
        }
        if (quotes%2 == 1)
        {
          if (ch == '.'){
            if (!identifier.schema.empty()){
              throw ParseException(part, pos);
            }
            identifier.schema= sb;
            return parseIdentifier(part, pos + 1, identifier);
          }
          identifier.name= sb;
          return pos;
        }
        quotes= 0;
        sb.append(ch);
      }
    }
    throw ParseException(part, startPos);
  }

    std::size_t MariaDbDatabaseMetaData::parseIdentifierList(const SQLString& part, std::size_t startPos, std::vector<Identifier>& list)
  {
    std::size_t pos= skipWhite(part, startPos);
    if (part.at(pos) != '(') {
      throw ParseException(part, pos);
    }
    pos++;
    for (;;)
    {
      pos= skipWhite(part, pos);
      char ch= part.at(pos);

      switch (ch) {
      case ')':
        return pos +1;
      case '`':
      {
        Identifier id;
        pos= parseIdentifier(part, pos, id);
        list.push_back(id);
        break;
      }
      case ',':
        pos++;
        break;
      default:
        throw ParseException(std::string(StringImp::get(part), startPos, part.length() - startPos), startPos);
      }
    }
  }

  std::size_t MariaDbDatabaseMetaData::skipKeyword(const SQLString& part, std::size_t startPos, const SQLString& keyword)
  {
    size_t pos= skipWhite(part, startPos);
    for (size_t i= 0;i < keyword.size();i++,pos++){
      if (part.at(pos) != keyword.at(i)){
        throw ParseException(part, pos);
      }
    }
    return pos;
  }

  int32_t MariaDbDatabaseMetaData::getImportedKeyAction(const std::string& actionKey)
  {
    if (actionKey.empty()){
      return DatabaseMetaData::importedKeyRestrict;
    }
    if (actionKey.compare("NO ACTION") == 0)
    {
      return DatabaseMetaData::importedKeyNoAction;
    }
    else if (actionKey.compare("CASCADE") == 0)
    {
      return DatabaseMetaData::importedKeyCascade;
    }
    else if ((actionKey.compare("SET NULL") == 0))
    {
      return DatabaseMetaData::importedKeySetNull;
    }
    else if (actionKey.compare("SET DEFAULT") == 0)
    {
      return DatabaseMetaData::importedKeySetDefault;
    }
    else if (actionKey.compare("RESTRICT") == 0)
    {
      return DatabaseMetaData::importedKeyRestrict;
    }
    else
    {
      throw  SQLException("Illegal key action '" + actionKey + "' specified.");
    }
  }

  /**
    * Get imported keys.
    *
    * @param tableDef table definition
    * @param tableName table name
    * @param catalog catalog
    * @param connection connection
    * @return resultset resultset
    * @throws ParseException exception
    */
  ResultSet* MariaDbDatabaseMetaData::getImportedKeys(
      const SQLString& tableDef, const SQLString& tableName, const SQLString& catalog, MariaDbConnection* connection)
  {
    static std::vector<SQLString> columnNames{"PKTABLE_CAT","PKTABLE_SCHEM","PKTABLE_NAME",
      "PKCOLUMN_NAME","FKTABLE_CAT","FKTABLE_SCHEM",
      "FKTABLE_NAME","FKCOLUMN_NAME","KEY_SEQ",
      "UPDATE_RULE","DELETE_RULE","FK_NAME",
      "PK_NAME","DEFERRABILITY"
    };
    std::vector<ColumnType>columnTypes {
      ColumnType::_NULL,ColumnType::VARCHAR,ColumnType::VARCHAR,
      ColumnType::VARCHAR,ColumnType::_NULL,ColumnType::VARCHAR,
      ColumnType::VARCHAR,ColumnType::VARCHAR,ColumnType::SMALLINT,
      ColumnType::SMALLINT,ColumnType::SMALLINT,ColumnType::VARCHAR,
      ColumnType::_NULL,ColumnType::SMALLINT
    };

    Tokens parts= split(tableDef, "\n");

    std::vector<std::vector<sql::bytes>> data;

    for (auto& part : *parts)
    {
      part= part.trim();
      if (!part.startsWith("CONSTRAINT") && !(StringImp::get(part).find("FOREIGN KEY") != std::string::npos)){
        continue;
      }
      //const char* partChar= part.c_str();

      Identifier constraintName;

      std::size_t pos= skipKeyword(part, 0, "CONSTRAINT");
      pos= parseIdentifier(part, pos, constraintName);
      pos= skipKeyword(part, pos, "FOREIGN KEY");

      std::vector<Identifier> foreignKeyCols;

      pos= parseIdentifierList(part, pos, foreignKeyCols);
      pos= skipKeyword(part, pos, "REFERENCES");
      Identifier pkTable;
      pos= parseIdentifier(part, pos, pkTable);
      std::vector<Identifier>primaryKeyCols;
      parseIdentifierList(part, pos, primaryKeyCols);

      if (primaryKeyCols.size() != foreignKeyCols.size()){
        throw ParseException(tableDef,0);
      }
      int32_t onUpdateReferenceAction= DatabaseMetaData::importedKeyRestrict;
      int32_t onDeleteReferenceAction= DatabaseMetaData::importedKeyRestrict;

      for (std::string referenceAction : {"RESTRICT","CASCADE", "SET NULL","NO ACTION"}) {
        if (StringImp::get(part).find("ON UPDATE " + referenceAction) != std::string::npos) {
          onUpdateReferenceAction= getImportedKeyAction(referenceAction);
        }
        if (StringImp::get(part).find("ON DELETE " + referenceAction) != std::string::npos) {
          onDeleteReferenceAction= getImportedKeyAction(referenceAction);
        }
      }

      std::string f8,
                  f9(std::to_string(onUpdateReferenceAction)),
                  f10(std::to_string(onDeleteReferenceAction)),
                  f13(std::to_string(DatabaseMetaData::importedKeyNotDeferrable));

      for (size_t i= 0; i <primaryKeyCols.size(); i++)
      {
        std::vector<sql::bytes> row;
        row.reserve(columnNames.size());

        // NULL in 1st column
        row.emplace_back(0); // PKTABLE_CAT

        if (pkTable.schema.empty())
        {
          row.push_back(BYTES_STR_INIT(catalog));
        }
        else
        {
          row.push_back(BYTES_STR_INIT(pkTable.schema)); // PKTABLE_SCHEM
        }
        
        f8= std::to_string(i + 1);

        row.push_back(BYTES_STR_INIT(pkTable.name)); // PKTABLE_NAME
        row.push_back(BYTES_STR_INIT(primaryKeyCols[i].name));
        // NULL in 5th column
        row.emplace_back(0);
        row.push_back(BYTES_STR_INIT(catalog));
        row.push_back(BYTES_STR_INIT(tableName));
        row.push_back(BYTES_STR_INIT(foreignKeyCols[i].name));
        row.push_back(BYTES_STR_INIT(f8));
        row.push_back(BYTES_STR_INIT(f9));
        row.push_back(BYTES_STR_INIT(f10));
        row.push_back(BYTES_STR_INIT(constraintName.name));
        row.emplace_back(0); // PK_NAME - unlike using information_schema, cannot know constraint name
        row.push_back(BYTES_STR_INIT(f13));
        data.push_back(row);
      }
    }

    std::sort(data.begin(), data.end(),
        [](const std::vector<sql::bytes>& row1, const std::vector<sql::bytes>& row2){
        std::size_t minSize= std::min<std::size_t>(row1[1].size(), row2[1].size());
        int32_t result= strncmp(row1[1],row2[1], minSize); // PKTABLE_SCHEM 
        if (result == 0){
          if (row1[1].size() != row2[1].size()) {
            return row1[1].size() < row2[1].size();
          }
          minSize = std::min<std::size_t>(row1[2].size(), row2[2].size());
          result= strncmp(row1[2], row2[2], minSize); // PKTABLE_NAME
        if (result == 0){
          if (row1[2].size() != row2[2].size()) {
            return row1[2].size() < row2[2].size();
          }
          result= static_cast<int32_t>(row1[8].size() - row2[8].size()); // KEY_SEQ
        if (result == 0){
          result= strncmp(row1[8], row2[8], row1[8].size());
        }
        }
        }
        return result < 0;
        });

    return SelectResultSet::createResultSet(columnNames, columnTypes, data, connection->getProtocol().get());
  }

  /**
   * Retrieves a description of the primary key columns that are referenced by the given table's
   * foreign key columns (the primary keys imported by a table). They are ordered by PKTABLE_CAT,
   * PKTABLE_SCHEM, PKTABLE_NAME, and KEY_SEQ.
   *
   * <p>Each primary key column description has the following columns:
   *
   * <OL>
   *   <LI><B>PKTABLE_CAT</B> String {@code= } primary key table catalog being imported (may be
   *       <code>null</code>)
   *   <LI><B>PKTABLE_SCHEM</B> String {@code= } primary key table schema being imported (may be
   *       <code>null</code>)
   *   <LI><B>PKTABLE_NAME</B> String {@code= } primary key table name being imported
   *   <LI><B>PKCOLUMN_NAME</B> String {@code= } primary key column name being imported
   *   <LI><B>FKTABLE_CAT</B> String {@code= } foreign key table catalog (may be <code>null</code>)
   *   <LI><B>FKTABLE_SCHEM</B> String {@code= } foreign key table schema (may be <code>null</code>
   *       )
   *   <LI><B>FKTABLE_NAME</B> String {@code= } foreign key table name
   *   <LI><B>FKCOLUMN_NAME</B> String {@code= } foreign key column name
   *   <LI><B>KEY_SEQ</B> short {@code= } sequence number within a foreign key( a value of 1
   *       represents the first column of the foreign key, a value of 2 would represent the second
   *       column within the foreign key).
   *   <LI><B>UPDATE_RULE</B> short {@code= } What happens to a foreign key when the primary key is
   *       updated:
   *       <UL>
   *         <LI>importedNoAction - do not allow update of primary key if it has been imported
   *         <LI>importedKeyCascade - change imported key to agree with primary key update
   *         <LI>importedKeySetNull - change imported key to <code>NULL</code> if its primary key
   *             has been updated
   *         <LI>importedKeySetDefault - change imported key to default values if its primary key
   *             has been updated
   *         <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x compatibility)
   *       </UL>
   *   <LI><B>DELETE_RULE</B> short {@code= } What happens to the foreign key when primary is
   *       deleted.
   *       <UL>
   *         <LI>importedKeyNoAction - do not allow delete of primary key if it has been imported
   *         <LI>importedKeyCascade - delete rows that import a deleted key
   *         <LI>importedKeySetNull - change imported key to NULL if its primary key has been
   *             deleted
   *         <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x compatibility)
   *         <LI>importedKeySetDefault - change imported key to default if its primary key has been
   *             deleted
   *       </UL>
   *   <LI><B>FK_NAME</B> String {@code= } foreign key name (may be <code>null</code>)
   *   <LI><B>PK_NAME</B> String {@code= } primary key name (may be <code>null</code>)
   *   <LI><B>DEFERRABILITY</B> short {@code= } can the evaluation of foreign key constraints be
   *       deferred until commit
   *       <UL>
   *         <LI>importedKeyInitiallyDeferred - see SQL92 for definition
   *         <LI>importedKeyInitiallyImmediate - see SQL92 for definition
   *         <LI>importedKeyNotDeferrable - see SQL92 for definition
   *       </UL>
   * </OL>
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schema a schema name; must match the schema name as it is stored in the database; ""
   *     retrieves those without a schema; <code>null</code> means that the schema name should not
   *     be used to narrow the search
   * @param table a table name; must match the table name as it is stored in the database
   * @return <code>ResultSet</code> - each row is a primary key column description
   * @throws SQLException if a database access error occurs
   * @see #getExportedKeys
   */
  ResultSet* MariaDbDatabaseMetaData::getImportedKeys(const SQLString& catalog, const SQLString& schema, const SQLString& table)  {

    // We use schema as schema.
    SQLString database(schema);

    // We avoid using information schema queries by default, because this appears to be an expensive query
    if (table.empty()){
      throw SQLException("'table' parameter in getImportedKeys cannot be NULL");
    }

    if (database.empty()) {
      return getImportedKeysUsingInformationSchema(database, table);
    }

    try {
      return getImportedKeysUsingShowCreateTable(database, table);
    }catch (std::runtime_error& /*e*/){
      // Likely, parsing failed, try out I_S query
      return getImportedKeysUsingInformationSchema(database, table);
    }
  }

  SQLString MariaDbDatabaseMetaData::dataTypeClause(const SQLString& fullTypeColumnName){
    Shared::Options options= urlParser.getOptions();
    return " CASE data_type"
      " WHEN 'bit' THEN "
      + std::to_string(Types::BIT)
      +" WHEN 'tinyblob' THEN "
      + std::to_string(Types::VARBINARY)
      +" WHEN 'mediumblob' THEN "
      + std::to_string(Types::LONGVARBINARY)
      +" WHEN 'longblob' THEN "
      + std::to_string(Types::LONGVARBINARY)
      +" WHEN 'blob' THEN "
      + std::to_string(Types::LONGVARBINARY)
      +" WHEN 'tinytext' THEN "
      + std::to_string(Types::VARCHAR)
      +" WHEN 'mediumtext' THEN "
      + std::to_string(Types::LONGVARCHAR)
      +" WHEN 'longtext' THEN "
      + std::to_string(Types::LONGVARCHAR)
      +" WHEN 'text' THEN "
      + std::to_string(Types::LONGVARCHAR)
      +" WHEN 'date' THEN "
      + std::to_string(Types::DATE)
      +" WHEN 'datetime' THEN "
      + std::to_string(Types::TIMESTAMP)
      +" WHEN 'decimal' THEN "
      + std::to_string(Types::DECIMAL)
      +" WHEN 'double' THEN "
      + std::to_string(Types::DOUBLE)
      +" WHEN 'enum' THEN "
      + std::to_string(Types::VARCHAR)
      +" WHEN 'float' THEN "
      + std::to_string(Types::REAL)
      +" WHEN 'int' THEN IF( "
      +fullTypeColumnName
      +" like '%unsigned%', "
      + std::to_string(Types::INTEGER)
      +","
      + std::to_string(Types::INTEGER)
      +")"
      " WHEN 'bigint' THEN "
      + std::to_string(Types::BIGINT)
      +" WHEN 'mediumint' THEN "
      + std::to_string(Types::INTEGER)
      +" WHEN 'NULL' THEN "
      + std::to_string(Types::_NULL)
      +" WHEN 'set' THEN "
      + std::to_string(Types::VARCHAR)
      +" WHEN 'smallint' THEN IF( "
      +fullTypeColumnName
      +" like '%unsigned%', "
      + std::to_string(Types::SMALLINT)
      +","
      + std::to_string(Types::SMALLINT)
      +")"
      " WHEN 'varchar' THEN "
      + std::to_string(Types::VARCHAR)
      +" WHEN 'varbinary' THEN "
      + std::to_string(Types::VARBINARY)
      +" WHEN 'char' THEN "
      + std::to_string(Types::CHAR)
      +" WHEN 'binary' THEN "
      + std::to_string(Types::BINARY)
      +" WHEN 'time' THEN "
      + std::to_string(Types::TIME)
      +" WHEN 'timestamp' THEN "
      + std::to_string(Types::TIMESTAMP)
      +" WHEN 'tinyint' THEN "
      +(options->tinyInt1isBit
          ? "IF("
          + fullTypeColumnName
          + " like 'tinyint(1)%',"
          + std::to_string(Types::BIT)
          + ","
          + std::to_string(Types::TINYINT)
          + ") "
          : SQLString(std::to_string(Types::TINYINT)))
      +" WHEN 'year' THEN "
      +(options->yearIsDateType ? std::to_string(Types::DATE) : std::to_string(Types::SMALLINT))
      +" ELSE "
      + std::to_string(Types::OTHER)
      +" END ";
  }


  ResultSet* MariaDbDatabaseMetaData::executeQuery(const SQLString& sql)
  {
    Unique::Statement stmt(connection->createStatement());
    // We are taking responsibility not to stream metadata queries
    stmt->setFetchSize(0);
    SelectResultSet* rs= dynamic_cast<SelectResultSet*>(stmt->executeQuery(sql));
    rs->setForceTableAlias();
    rs->setStatement(nullptr);
    return rs;
  }


  SQLString MariaDbDatabaseMetaData::escapeQuote(const SQLString& value){
    if (value.empty() == true){
      return "NULL";
    }
    return "'" + Utils::escapeString(value, connection->getProtocol()->noBackslashEscapes()) + "'";
  }

/**
  * Generate part of the information schema query that restricts catalog names In the driver,
  * catalogs is the equivalent to MariaDB schemas.
  *
  * @param columnName - column name in the information schema table
  * @param catalog - catalog name. This driver does not (always) follow JDBC standard for following
  *     special values, due to ConnectorJ compatibility 1. empty string ("") - matches current
  *     catalog (i.e database). JDBC standard says only tables without catalog should be returned -
  *     such tables do not exist in MariaDB. If there is no current catalog, then empty string
  *     matches any catalog. 2. null - if nullCatalogMeansCurrent=true (which is the default), then
  *     the handling is the same as for "" . i.e return current catalog.JDBC-conforming way would
  *     be to match any catalog with null parameter. This can be switched with
  *     nullCatalogMeansCurrent=false in the connection URL.
  * @return part of SQL query ,that restricts search for the catalog.
  */
SQLString MariaDbDatabaseMetaData::catalogCond(const SQLString& columnName, const SQLString& catalog)
{
  if (catalog.empty()) {

    if (connection->nullCatalogMeansCurrent){
      return "(ISNULL(database()) OR ("+columnName +" = database()))";
    }
    return "(1 = 1)";
  }
  // TODO the upper if is for NULL value. Have to decide if we can have NULL value here, i.e. should catalog and other names be passed by ptr to the connector
  /*if (catalog.empty()){
    return "(ISNULL(database()) OR ("+columnName +" = database()))";
  }*/
  return "(" + columnName + " = " + escapeQuote(catalog) + ")";
}


// table name)
SQLString MariaDbDatabaseMetaData::patternCond(const SQLString& columnName, const SQLString& tableName)
{
  if (tableName.empty()){
    return "(1 = 1)";
  }
  SQLString predicate =
    (tableName.find_first_of('%') == std::string::npos && tableName.find_first_of('_') == std::string::npos) ? "=" : "LIKE";

  return "("+columnName +" "+predicate +" '" + Utils::escapeString(tableName,true)+"')";
}


/* We can't pass NULL, "" should mean "no schema" */
SQLString schemaPatternCond(const SQLString& columnName, const SQLString& schemaName)
{
  SQLString predicate =
    (schemaName.find_first_of('%') == std::string::npos && schemaName.find_first_of('_') == std::string::npos) ? "=" : "LIKE";

  return "(" + columnName + " " + predicate + " '" + Utils::escapeString(schemaName, true) + "')";
}


/**
 * Retrieves a description of the given table's primary key columns. They are ordered by
 * COLUMN_NAME.
 *
 * <p>Each primary key column description has the following columns:
 *
 * <OL>
 *   <li><B>TABLE_CAT</B> String {@code= } table catalog
 *   <li><B>TABLE_SCHEM</B> String {@code= } table schema (may be <code>null</code>)
 *   <li><B>TABLE_NAME</B> String {@code= } table name
 *   <li><B>COLUMN_NAME</B> String {@code= } column name
 *   <li><B>KEY_SEQ</B> short {@code= } sequence number within primary key( a value of 1
 *       represents the first column of the primary key, a value of 2 would represent the second
 *       column within the primary key).
 *   <li><B>PK_NAME</B> String {@code= } primary key name
 * </OL>
 *
 * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
 *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
 *     be used to narrow the search
 * @param schema a schema name; must match the schema name as it is stored in the database; ""
 *     retrieves those without a schema; <code>null</code> means that the schema name should not
 *     be used to narrow the search
 * @param table a table name; must match the table name as it is stored in the database
 * @return <code>ResultSet</code> - each row is a primary key column description
 * @throws SQLException if a database access error occurs
 */
ResultSet* MariaDbDatabaseMetaData::getPrimaryKeys(const SQLString& catalog, const SQLString& schema, const SQLString& table) {


  SQLString sql =
    "SELECT NULL TABLE_CAT, A.TABLE_SCHEMA TABLE_SCHEM, A.TABLE_NAME, A.COLUMN_NAME, B.SEQ_IN_INDEX KEY_SEQ, B.INDEX_NAME PK_NAME "
    " FROM INFORMATION_SCHEMA.COLUMNS A, INFORMATION_SCHEMA.STATISTICS B"
    " WHERE A.COLUMN_KEY in ('PRI','pri') AND B.INDEX_NAME='PRIMARY' "
    " AND "
    + schemaPatternCond("A.TABLE_SCHEMA", schema) //We don't need all schemas on empty schema, that catCond will do
    + " AND B.TABLE_SCHEMA=A.TABLE_SCHEMA"
      " AND "
    + patternCond("A.TABLE_NAME", table)
    + " AND B.TABLE_NAME=A.TABLE_NAME AND A.COLUMN_NAME = B.COLUMN_NAME "
      " ORDER BY A.COLUMN_NAME";

  return executeQuery(sql);
}

/**
 * Retrieves a description of the tables available in the given catalog. Only table descriptions
 * matching the catalog, schema, table name and type criteria are returned. They are ordered by
 * <code>TABLE_TYPE</code>, <code>TABLE_CAT</code>, <code>TABLE_SCHEM</code> and <code>TABLE_NAME
 * </code>. Each table description has the following columns:
 *
 * <OL>
 *   <LI><B>TABLE_CAT</B> String {@code= } table catalog (may be <code>null</code>)
 *   <LI><B>TABLE_SCHEM</B> String {@code= } table schema (may be <code>null</code>)
 *   <LI><B>TABLE_NAME</B> String {@code= } table name
 *   <LI><B>TABLE_TYPE</B> String {@code= } table type. Typical types are "TABLE", "VIEW",
 *       "SYSTEM TABLE", "GLOBAL TEMPORARY", "LOCAL TEMPORARY", "ALIAS", "SYNONYM".
 *   <LI><B>REMARKS</B> String {@code= } explanatory comment on the table
 *   <LI><B>TYPE_CAT</B> String {@code= } the types catalog (may be <code>null</code>)
 *   <LI><B>TYPE_SCHEM</B> String {@code= } the types schema (may be <code>null</code>)
 *   <LI><B>TYPE_NAME</B> String {@code= } type name (may be <code>null</code>)
 *   <LI><B>SELF_REFERENCING_COL_NAME</B> String {@code= } name of the designated "identifier"
 *       column of a typed table (may be <code>null</code>)
 *   <LI><B>REF_GENERATION</B> String {@code= } specifies how values in SELF_REFERENCING_COL_NAME
 *       are created. Values are "SYSTEM", "USER", "DERIVED". (may be <code>null</code>)
 * </OL>
 *
 * <p><B>Note:</B> Some databases may not return information for all tables.
 *
 * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
 *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
 *     be used to narrow the search
 * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
 *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
 *     should not be used to narrow the search
 * @param tableNamePattern a table name pattern; must match the table name as it is stored in the
 *     database
 * @param types a list of table types, which must be from the list of table types returned from
 *     {@link #getTableTypes},to include; <code>null</code> returns all types
 * @return <code>ResultSet</code> - each row is a table description
 * @throws SQLException if a database access error occurs
 * @see #getSearchStringEscape
 */
ResultSet* MariaDbDatabaseMetaData::getTables(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern,
  std::list<SQLString>& wrappedTypes)
{
  const std::list<SQLString>& types= wrappedTypes;
  SQLString sql(
      "SELECT NULL TABLE_CAT, TABLE_SCHEMA TABLE_SCHEM,  TABLE_NAME,"
      " IF(TABLE_TYPE='BASE TABLE', 'TABLE', TABLE_TYPE) as TABLE_TYPE,"
      " TABLE_COMMENT REMARKS, NULL TYPE_CAT, NULL TYPE_SCHEM, NULL TYPE_NAME, NULL SELF_REFERENCING_COL_NAME, "
      " NULL REF_GENERATION"
      " FROM INFORMATION_SCHEMA.TABLES "
      " WHERE "
      + schemaPatternCond("TABLE_SCHEMA", schemaPattern)
      +" AND "
      +patternCond("TABLE_NAME", tableNamePattern));

  if (!types.empty())
  {
    sql.append(" AND TABLE_TYPE IN (");
    for (const auto& type : types) {
      if (type.empty()) {
        continue;
      }
      SQLString escapedType(type.compare("TABLE") == 0 ? "'BASE TABLE'" : escapeQuote(type).c_str());
      sql.append(escapedType).append(",");
    }
    StringImp::get(sql)[sql.length()-1]= ')';
  }
  sql.append(" ORDER BY TABLE_TYPE, TABLE_SCHEMA, TABLE_NAME");

  return executeQuery(sql);
}

/**
  * Retrieves a description of table columns available in the specified catalog.
  *
  * <p>Only column descriptions matching the catalog, schema, table and column name criteria are
  * returned. They are ordered by <code>TABLE_CAT</code>,<code>TABLE_SCHEM</code>, <code>TABLE_NAME
  * </code>, and <code>ORDINAL_POSITION</code>.
  *
  * <p>Each column description has the following columns:
  *
  * <OL>
  *   <LI><B>TABLE_CAT</B> String {@code= } table catalog (may be <code>null</code>)
  *   <LI><B>TABLE_SCHEM</B> String {@code= } table schema (may be <code>null</code>)
  *   <LI><B>TABLE_NAME</B> String {@code= } table name
  *   <LI><B>COLUMN_NAME</B> String {@code= } column name
  *   <LI><B>DATA_TYPE</B> int {@code= } SQL type from java.sql.Types
  *   <LI><B>TYPE_NAME</B> String {@code= } Data source dependent type name, for a UDT the type
  *       name is fully qualified
  *   <LI><B>COLUMN_SIZE</B> int {@code= } column size.
  *   <LI><B>BUFFER_LENGTH</B> is not used.
  *   <LI><B>DECIMAL_DIGITS</B> int {@code= } the number of fractional digits. Null is returned
  *       for data types where DECIMAL_DIGITS is not applicable.
  *   <LI><B>NUM_PREC_RADIX</B> int {@code= } Radix (typically either 10 or 2)
  *   <LI><B>NULLABLE</B> int {@code= } is NULL allowed.
  *       <UL>
  *         <LI>columnNoNulls - might not allow <code>NULL</code> values
  *         <LI>columnNullable - definitely allows <code>NULL</code> values
  *         <LI>columnNullableUnknown - nullability unknown
  *       </UL>
  *   <LI><B>REMARKS</B> String {@code= } comment describing column (may be <code>null</code>)
  *   <LI><B>COLUMN_DEF</B> String {@code= } default value for the column, which should be
  *       interpreted as a string when the value is enclosed in single quotes (may be <code>null
  *       </code>)
  *   <LI><B>SQL_DATA_TYPE</B> int {@code= } unused
  *   <LI><B>SQL_DATETIME_SUB</B> int {@code= } unused
  *   <LI><B>CHAR_OCTET_LENGTH</B> int {@code= } for char types the maximum number of bytes in the
  *       column
  *   <LI><B>ORDINAL_POSITION</B> int {@code= } index of column in table (starting at 1)
  *   <LI><B>IS_NULLABLE</B> String {@code= } ISO rules are used to determine the nullability for
  *       a column.
  *       <UL>
  *         <LI>YES --- if the column can include NULLs
  *         <LI>NO --- if the column cannot include NULLs
  *         <LI>empty string --- if the nullability for the column is unknown
  *       </UL>
  *   <LI><B>SCOPE_CATALOG</B> String {@code= } catalog of table that is the scope of a reference
  *       attribute (<code>null</code> if DATA_TYPE isn't REF)
  *   <LI><B>SCOPE_SCHEMA</B> String {@code= } schema of table that is the scope of a reference
  *       attribute (<code>null</code> if the DATA_TYPE isn't REF)
  *   <LI><B>SCOPE_TABLE</B> String {@code= } table name that this the scope of a reference
  *       attribute (<code>null</code> if the DATA_TYPE isn't REF)
  *   <LI><B>SOURCE_DATA_TYPE</B> short {@code= } source type of a distinct type or user-generated
  *       Ref type, SQL type from java.sql.Types (<code>null</code> if DATA_TYPE isn't DISTINCT or
  *       user-generated REF)
  *   <LI><B>IS_AUTOINCREMENT</B> String {@code= } Indicates whether this column is auto
  *       incremented
  *       <UL>
  *         <LI>YES --- if the column is auto incremented
  *         <LI>NO --- if the column is not auto incremented
  *         <LI>empty string --- if it cannot be determined whether the column is auto incremented
  *       </UL>
  *   <LI><B>IS_GENERATEDCOLUMN</B> String {@code= } Indicates whether this is a generated column
  *       <UL>
  *         <LI>YES --- if this a generated column
  *         <LI>NO --- if this not a generated column
  *         <LI>empty string --- if it cannot be determined whether this is a generated column
  *       </UL>
  * </OL>
  *
  * <p>The COLUMN_SIZE column specifies the column size for the given column. For numeric data,
  * this is the maximum precision. For character data, this is the length in characters. For
  * datetime datatypes, this is the length in characters of the String representation (assuming the
* maximum allowed precision of the fractional seconds component). For binary data, this is the
  * length in bytes. For the ROWID datatype, this is the length in bytes. Null is returned for data
  * types where the column size is not applicable.
  *
  * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
  *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
  *     be used to narrow the search
  * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
  *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
  *     should not be used to narrow the search
  * @param tableNamePattern a table name pattern; must match the table name as it is stored in the
  *     database
  * @param columnNamePattern a column name pattern; must match the column name as it is stored in
  *     the database
  * @return <code>ResultSet</code> - each row is a column description
  * @throws SQLException if a database access error occurs
  * @see #getSearchStringEscape
  */
  ResultSet* MariaDbDatabaseMetaData::getColumns(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern,
                                              const SQLString& columnNamePattern)
  {
    Shared::Options options= urlParser.getOptions();
    SQLString sql= "SELECT NULL TABLE_CAT, TABLE_SCHEMA TABLE_SCHEM, TABLE_NAME, COLUMN_NAME,"
      +dataTypeClause("COLUMN_TYPE")
      +" DATA_TYPE,"
      + columnTypeClause(options)
      +" TYPE_NAME, "
      " CASE DATA_TYPE"
      "  WHEN 'time' THEN "
      +(datePrecisionColumnExist
          ?"IF(DATETIME_PRECISION = 0, 10, CAST(11 + DATETIME_PRECISION as signed integer))"
          :"10")
      +"  WHEN 'date' THEN 10"
      "  WHEN 'datetime' THEN "
      +(datePrecisionColumnExist
          ?"IF(DATETIME_PRECISION = 0, 19, CAST(20 + DATETIME_PRECISION as signed integer))"
          :"19")
      +"  WHEN 'timestamp' THEN "
      +(datePrecisionColumnExist
          ?"IF(DATETIME_PRECISION = 0, 19, CAST(20 + DATETIME_PRECISION as signed integer))"
          :"19")
      +(options->yearIsDateType ?"":" WHEN 'year' THEN 5")
      +"  ELSE "
      "  IF(NUMERIC_PRECISION IS NULL, LEAST(CHARACTER_MAXIMUM_LENGTH,"
      + std::to_string(UINT32_MAX)
      +"), NUMERIC_PRECISION) "
      " END"
      " COLUMN_SIZE, 65535 BUFFER_LENGTH, "
      " CONVERT (CASE DATA_TYPE"
      " WHEN 'year' THEN "
      +(options->yearIsDateType ? SQLString("NUMERIC_SCALE") : SQLString("0"))
      +" WHEN 'tinyint' THEN "
      +(options->tinyInt1isBit ? SQLString("0") : SQLString("NUMERIC_SCALE"))
      +" ELSE NUMERIC_SCALE END, UNSIGNED INTEGER) DECIMAL_DIGITS,"
      " 10 NUM_PREC_RADIX, IF(IS_NULLABLE = 'yes' OR COLUMN_TYPE='timestamp',1,0) NULLABLE, COLUMN_COMMENT REMARKS," /* NULLABLE: 0- columnNoNulls, 1 - columnNullable */
      " COLUMN_DEFAULT COLUMN_DEF, 0 SQL_DATA_TYPE, 0 SQL_DATETIME_SUB,  "
      " LEAST(CHARACTER_OCTET_LENGTH,"
      + std::to_string(INT32_MAX)
      +") CHAR_OCTET_LENGTH,"
      " ORDINAL_POSITION, IF(COLUMN_TYPE='timestamp', 'YES', IS_NULLABLE) IS_NULLABLE, NULL SCOPE_CATALOG, NULL SCOPE_SCHEMA, NULL SCOPE_TABLE, NULL SOURCE_DATA_TYPE,"
      " IF(EXTRA = 'auto_increment','YES','NO') IS_AUTOINCREMENT, "
      " IF(EXTRA in ('VIRTUAL', 'PERSISTENT', 'VIRTUAL GENERATED', 'STORED GENERATED') ,'YES','NO') IS_GENERATEDCOLUMN "
      " FROM INFORMATION_SCHEMA.COLUMNS  WHERE "
      +catalogCond("TABLE_SCHEMA", schemaPattern)
      +" AND "
      +patternCond("TABLE_NAME",tableNamePattern)
      +" AND "
      +patternCond("COLUMN_NAME",columnNamePattern)
      +" ORDER BY TABLE_CAT, TABLE_SCHEM, TABLE_NAME, ORDINAL_POSITION";

    try {
      return executeQuery(sql);
    }catch (SQLException& sqlException){
      if ((StringImp::get(sqlException.getMessage()).find("Unknown column 'DATETIME_PRECISION'") != std::string::npos)){
        datePrecisionColumnExist= false;
        return getColumns(catalog,schemaPattern,tableNamePattern,columnNamePattern);
      }
      throw sqlException;
    }
  }

  /**
   * Retrieves a description of the foreign key columns that reference the given table's primary key
   * columns (the foreign keys exported by a table). They are ordered by FKTABLE_CAT, FKTABLE_SCHEM,
   * FKTABLE_NAME, and KEY_SEQ.
   *
   * <p>Each foreign key column description has the following columns:
   *
   * <OL>
   *   <LI><B>PKTABLE_CAT</B> String {@code= } primary key table catalog (may be <code>null</code>)
   *   <LI><B>PKTABLE_SCHEM</B> String {@code= } primary key table schema (may be <code>null</code>
   *       )
   *   <LI><B>PKTABLE_NAME</B> String {@code= } primary key table name
   *   <LI><B>PKCOLUMN_NAME</B> String {@code= } primary key column name
   *   <LI><B>FKTABLE_CAT</B> String {@code= } foreign key table catalog (may be <code>null</code>)
   *       being exported (may be <code>null</code>)
   *   <LI><B>FKTABLE_SCHEM</B> String {@code= } foreign key table schema (may be <code>null</code>
   *       ) being exported (may be <code>null</code>)
   *   <LI><B>FKTABLE_NAME</B> String {@code= } foreign key table name being exported
   *   <LI><B>FKCOLUMN_NAME</B> String {@code= } foreign key column name being exported
   *   <LI><B>KEY_SEQ</B> short {@code= } sequence number within foreign key( a value of 1
   *       represents the first column of the foreign key, a value of 2 would represent the second
   *       column within the foreign key).
   *   <LI><B>UPDATE_RULE</B> short {@code= } What happens to foreign key when primary is updated:
   *       <UL>
   *         <LI>importedNoAction - do not allow update of primary key if it has been imported
   *         <LI>importedKeyCascade - change imported key to agree with primary key update
   *         <LI>importedKeySetNull - change imported key to <code>NULL</code> if its primary key
   *             has been updated
   *         <LI>importedKeySetDefault - change imported key to default values if its primary key
   *             has been updated
   *         <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x compatibility)
   *       </UL>
   *   <LI><B>DELETE_RULE</B> short {@code= } What happens to the foreign key when primary is
   *       deleted.
   *       <UL>
   *         <LI>importedKeyNoAction - do not allow delete of primary key if it has been imported
   *         <LI>importedKeyCascade - delete rows that import a deleted key
   *         <LI>importedKeySetNull - change imported key to <code>NULL</code> if its primary key
   *             has been deleted
   *         <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x compatibility)
   *         <LI>importedKeySetDefault - change imported key to default if its primary key has been
   *             deleted
   *       </UL>
   *   <LI><B>FK_NAME</B> String {@code= } foreign key name (may be <code>null</code>)
   *   <LI><B>PK_NAME</B> String {@code= } primary key name (may be <code>null</code>)
   *   <LI><B>DEFERRABILITY</B> short {@code= } can the evaluation of foreign key constraints be
   *       deferred until commit
   *       <UL>
   *         <LI>importedKeyInitiallyDeferred - see SQL92 for definition
   *         <LI>importedKeyInitiallyImmediate - see SQL92 for definition
   *         <LI>importedKeyNotDeferrable - see SQL92 for definition
   *       </UL>
   * </OL>
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in this database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schema a schema name; must match the schema name as it is stored in the database; ""
   *     retrieves those without a schema; <code>null</code> means that the schema name should not
   *     be used to narrow the search
   * @param table a table name; must match the table name as it is stored in this database
   * @return a <code>ResultSet</code> object in which each row is a foreign key column description
   * @throws SQLException if a database access error occurs
   * @see #getImportedKeys
   */
  ResultSet* MariaDbDatabaseMetaData::getExportedKeys(const SQLString& catalog, const SQLString& schema, const SQLString& table)  {
    if (table.empty() == true){
      throw SQLException("'table' parameter in getExportedKeys cannot be NULL");
    }
    SQLString sql =
      "SELECT NULL PKTABLE_CAT, KCU.REFERENCED_TABLE_SCHEMA PKTABLE_SCHEM, KCU.REFERENCED_TABLE_NAME PKTABLE_NAME,"
      " KCU.REFERENCED_COLUMN_NAME PKCOLUMN_NAME, NULL FKTABLE_CAT, KCU.TABLE_SCHEMA FKTABLE_SCHEM, "
      " KCU.TABLE_NAME FKTABLE_NAME, KCU.COLUMN_NAME FKCOLUMN_NAME, KCU.POSITION_IN_UNIQUE_CONSTRAINT KEY_SEQ,"
      " CASE update_rule "
      "   WHEN 'RESTRICT' THEN 1"
      "   WHEN 'NO ACTION' THEN 3"
      "   WHEN 'CASCADE' THEN 0"
      "   WHEN 'SET NULL' THEN 2"
      "   WHEN 'SET DEFAULT' THEN 4"
      " END UPDATE_RULE,"
      " CASE DELETE_RULE"
      "  WHEN 'RESTRICT' THEN 1"
      "  WHEN 'NO ACTION' THEN 3"
      "  WHEN 'CASCADE' THEN 0"
      "  WHEN 'SET NULL' THEN 2"
      "  WHEN 'SET DEFAULT' THEN 4"
      " END DELETE_RULE,"
      " RC.CONSTRAINT_NAME FK_NAME,"
      " RC.UNIQUE_CONSTRAINT_NAME PK_NAME,"
      + std::to_string(importedKeyNotDeferrable)
      +" DEFERRABILITY"
      " FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE KCU"
      " INNER JOIN INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS RC"
      " ON KCU.CONSTRAINT_SCHEMA = RC.CONSTRAINT_SCHEMA"
      " AND KCU.CONSTRAINT_NAME = RC.CONSTRAINT_NAME"
      " WHERE "
      + catalogCond("KCU.REFERENCED_TABLE_SCHEMA",schema)
      +" AND "
      " KCU.REFERENCED_TABLE_NAME = "
      +escapeQuote(table)
      +" ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ";

    return executeQuery(sql);
  }

  /**
   * GetImportedKeysUsingInformationSchema.
   *
   * @param catalog catalog
   * @param table table
   * @return resultset
   * @throws SQLException exception
   */
  ResultSet* MariaDbDatabaseMetaData::getImportedKeysUsingInformationSchema(const SQLString& catalog, const SQLString& table)  {
    if (table.empty() == true){
      throw SQLException("'table' parameter in getImportedKeys cannot be NULL");
    }
    SQLString sql =
      "SELECT NULL PKTABLE_CAT, KCU.REFERENCED_TABLE_SCHEMA PKTABLE_SCHEM, KCU.REFERENCED_TABLE_NAME PKTABLE_NAME,"
      " KCU.REFERENCED_COLUMN_NAME PKCOLUMN_NAME, NULL FKTABLE_CAT, KCU.TABLE_SCHEMA FKTABLE_SCHEM, "
      " KCU.TABLE_NAME FKTABLE_NAME, KCU.COLUMN_NAME FKCOLUMN_NAME, KCU.POSITION_IN_UNIQUE_CONSTRAINT KEY_SEQ,"
      " CASE update_rule "
      "   WHEN 'RESTRICT' THEN 1"
      "   WHEN 'NO ACTION' THEN 3"
      "   WHEN 'CASCADE' THEN 0"
      "   WHEN 'SET NULL' THEN 2"
      "   WHEN 'SET DEFAULT' THEN 4"
      " END UPDATE_RULE,"
      " CASE DELETE_RULE"
      "  WHEN 'RESTRICT' THEN 1"
      "  WHEN 'NO ACTION' THEN 3"
      "  WHEN 'CASCADE' THEN 0"
      "  WHEN 'SET NULL' THEN 2"
      "  WHEN 'SET DEFAULT' THEN 4"
      " END DELETE_RULE,"
      " RC.CONSTRAINT_NAME FK_NAME,"
      " RC.UNIQUE_CONSTRAINT_NAME PK_NAME,"
      + std::to_string(importedKeyNotDeferrable)
      +" DEFERRABILITY"
      " FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE KCU"
      " INNER JOIN INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS RC"
      " ON KCU.CONSTRAINT_SCHEMA = RC.CONSTRAINT_SCHEMA"
      " AND KCU.CONSTRAINT_NAME = RC.CONSTRAINT_NAME"
      " WHERE "
      + catalogCond("KCU.TABLE_SCHEMA", catalog)
      +" AND "
      " KCU.TABLE_NAME = "
      + escapeQuote(table)
      +" ORDER BY PKTABLE_CAT, PKTABLE_SCHEM, PKTABLE_NAME, KEY_SEQ";

    return executeQuery(sql);
  }

  /**
   * GetImportedKeysUsingShowCreateTable.
   *
   * @param catalog catalog
   * @param table table
   * @return resultset
   * @throws Exception exception
   */
  ResultSet* MariaDbDatabaseMetaData::getImportedKeysUsingShowCreateTable(const SQLString& catalog, const SQLString& table)
  {
    if (catalog.empty()){
      throw std::runtime_error("catalog");
    }

    if (table.empty()){
      throw std::runtime_error("table");
    }

    std::unique_ptr<Statement> stmt(connection->createStatement());
    std::unique_ptr<ResultSet> rs(stmt->executeQuery(
          "SHOW CREATE TABLE "
          +MariaDbConnection::quoteIdentifier(catalog)
          +"."
          +MariaDbConnection::quoteIdentifier(table)));
    if (rs->next()){
      SQLString tableDef(rs->getString(2));
      return MariaDbDatabaseMetaData::getImportedKeys(tableDef, table, catalog, connection);
    }
    throw SQLException("Fail to retrieve table information using SHOW CREATE TABLE");
  }

  /**
   * Retrieves a description of a table's optimal set of columns that uniquely identifies a row.
   * They are ordered by SCOPE.
   *
   * <p>Each column description has the following columns:
   *
   * <OL>
   *   <LI><B>SCOPE</B> short {@code= } actual scope of result
   *       <UL>
   *         <LI>bestRowTemporary - very temporary, while using row
   *         <LI>bestRowTransaction - valid for remainder of current transaction
   *         <LI>bestRowSession - valid for remainder of current session
   *       </UL>
   *   <LI><B>COLUMN_NAME</B> String {@code= } column name
   *   <LI><B>DATA_TYPE</B> int {@code= } SQL data type from java.sql.Types
   *   <LI><B>TYPE_NAME</B> String {@code= } Data source dependent type name, for a UDT the type
   *       name is fully qualified
   *   <LI><B>COLUMN_SIZE</B> int {@code= } precision
   *   <LI><B>BUFFER_LENGTH</B> int {@code= } not used
   *   <LI><B>DECIMAL_DIGITS</B> short {@code= } scale - Null is returned for data types where
   *       DECIMAL_DIGITS is not applicable.
   *   <LI><B>PSEUDO_COLUMN</B> short {@code= } is this a pseudo column like an Oracle ROWID
   *       <UL>
   *         <LI>bestRowUnknown - may or may not be pseudo column
   *         <LI>bestRowNotPseudo - is NOT a pseudo column
   *         <LI>bestRowPseudo - is a pseudo column
   *       </UL>
   * </OL>
   *
   * <p>The COLUMN_SIZE column represents the specified column size for the given column. For
   * numeric data, this is the maximum precision. For character data, this is the length in
   * characters. For datetime datatypes, this is the length in characters of the String
   * representation (assuming the maximum allowed precision of the fractional seconds component).
   * For binary data, this is the length in bytes. For the ROWID datatype, this is the length in
   * bytes. Null is returned for data types where the column size is not applicable.
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schema a schema name; must match the schema name as it is stored in the database; ""
   *     retrieves those without a schema; <code>null</code> means that the schema name should not
   *     be used to narrow the search
   * @param table a table name; must match the table name as it is stored in the database
   * @param scope the scope of interest; use same values as SCOPE
   * @param nullable include columns that are nullable.
   * @return <code>ResultSet</code> - each row is a column description
   * @throws SQLException if a database access error occurs
   */
  ResultSet* MariaDbDatabaseMetaData::getBestRowIdentifier(
      const SQLString& catalog, const SQLString& schema, const SQLString& table,int32_t scope,bool nullable)
  {
    if (table.empty() == true){
      throw SQLException("'table' parameter cannot be NULL in getBestRowIdentifier()");
    }

    SQLString sql =
      "SELECT "
      + std::to_string(DatabaseMetaData::bestRowSession)
      +" SCOPE, COLUMN_NAME,"
      + dataTypeClause("COLUMN_TYPE")
      +" DATA_TYPE, DATA_TYPE TYPE_NAME,"
      " IF(NUMERIC_PRECISION IS NULL, CHARACTER_MAXIMUM_LENGTH, NUMERIC_PRECISION) COLUMN_SIZE, 0 BUFFER_LENGTH,"
      " NUMERIC_SCALE DECIMAL_DIGITS,"
      + (connection->getProtocol()->versionGreaterOrEqual(10, 2, 5)
        ? " if(IS_GENERATED='NEVER'," + std::to_string(DatabaseMetaData::bestRowNotPseudo) + "," + std::to_string(DatabaseMetaData::bestRowPseudo) + ")"
        : std::to_string(DatabaseMetaData::bestRowNotPseudo))
      + " PSEUDO_COLUMN"
      " FROM INFORMATION_SCHEMA.COLUMNS"
      " WHERE COLUMN_KEY IN('PRI', 'UNI')"
      " AND IS_NULLABLE='NO' AND "
      +catalogCond("TABLE_SCHEMA", schema)
      +" AND TABLE_NAME = "
      +escapeQuote(table);

    return executeQuery(sql);
  }

  bool MariaDbDatabaseMetaData::generatedKeyAlwaysReturned(){
    return true;
  }

  /**
   * Retrieves a description of the pseudo or hidden columns available in a given table within the
   * specified catalog and schema. Pseudo or hidden columns may not always be stored within a table
   * and are not visible in a ResultSet unless they are specified in the query's outermost SELECT
   * list. Pseudo or hidden columns may not necessarily be able to be modified. If there are no
   * pseudo or hidden columns, an empty ResultSet is returned.
   *
   * <p>Only column descriptions matching the catalog, schema, table and column name criteria are
   * returned. They are ordered by <code>TABLE_CAT</code>,<code>TABLE_SCHEM</code>, <code>TABLE_NAME
   * </code> and <code>COLUMN_NAME</code>.
   *
   * <p>Each column description has the following columns:
   *
   * <OL>
   *   <LI><B>TABLE_CAT</B> String {@code= } table catalog (may be <code>null</code>)
   *   <LI><B>TABLE_SCHEM</B> String {@code= } table schema (may be <code>null</code>)
   *   <LI><B>TABLE_NAME</B> String {@code= } table name
   *   <LI><B>COLUMN_NAME</B> String {@code= } column name
   *   <LI><B>DATA_TYPE</B> int {@code= } SQL type from java.sql.Types
   *   <LI><B>COLUMN_SIZE</B> int {@code= } column size.
   *   <LI><B>DECIMAL_DIGITS</B> int {@code= } the number of fractional digits. Null is returned
   *       for data types where DECIMAL_DIGITS is not applicable.
   *   <LI><B>NUM_PREC_RADIX</B> int {@code= } Radix (typically either 10 or 2)
   *   <LI><B>COLUMN_USAGE</B> String {@code= } The allowed usage for the column. The value
   *       returned will correspond to the enum name returned by PseudoColumnUsage.name()
   *   <LI><B>REMARKS</B> String {@code= } comment describing column (may be <code>null</code>)
   *   <LI><B>CHAR_OCTET_LENGTH</B> int {@code= } for char types the maximum number of bytes in the
   *       column
   *   <LI><B>IS_NULLABLE</B> String {@code= } ISO rules are used to determine the nullability for
   *       a column.
   *       <UL>
   *         <LI>YES --- if the column can include NULLs
   *         <LI>NO --- if the column cannot include NULLs
   *         <LI>empty string --- if the nullability for the column is unknown
   *       </UL>
   * </OL>
   *
   * <p>The COLUMN_SIZE column specifies the column size for the given column. For numeric data,
   * this is the maximum precision. For character data, this is the length in characters. For
   * datetime datatypes, this is the length in characters of the String representation (assuming the
   * maximum allowed precision of the fractional seconds component). For binary data, this is the
   * length in bytes. For the ROWID datatype, this is the length in bytes. Null is returned for data
   * types where the column size is not applicable.
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
   *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
   *     should not be used to narrow the search
   * @param tableNamePattern a table name pattern; must match the table name as it is stored in the
   *     database
   * @param columnNamePattern a column name pattern; must match the column name as it is stored in
   *     the database
   * @return <code>ResultSet</code> - each row is a column description
   * @throws SQLException if a database access error occurs
   * @see PseudoColumnUsage
   * @since 1.7
   */
  ResultSet* MariaDbDatabaseMetaData::getPseudoColumns(
      const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern, const SQLString& columnNamePattern)
  {
    std::unique_ptr<sql::Statement> stmt(connection->createStatement());
    return stmt->executeQuery(
          "SELECT ' ' TABLE_CAT, ' ' TABLE_SCHEM,"
          "' ' TABLE_NAME, ' ' COLUMN_NAME, 0 DATA_TYPE, 0 COLUMN_SIZE, 0 DECIMAL_DIGITS,"
          "10 NUM_PREC_RADIX, ' ' COLUMN_USAGE,  ' ' REMARKS, 0 CHAR_OCTET_LENGTH, 'YES' IS_NULLABLE FROM DUAL "
          "WHERE 1=0");
  }

  bool MariaDbDatabaseMetaData::allProceduresAreCallable(){
    return true;
  }

  bool MariaDbDatabaseMetaData::allTablesAreSelectable(){
    return true;
  }

  SQLString MariaDbDatabaseMetaData::getURL(){
    return urlParser.getInitialUrl();
  }

  SQLString MariaDbDatabaseMetaData::getUserName(){
    return urlParser.getUsername();
  }

  bool MariaDbDatabaseMetaData::isReadOnly(){
    return false;
  }

  bool MariaDbDatabaseMetaData::nullsAreSortedHigh(){
    return false;
  }

  bool MariaDbDatabaseMetaData::nullsAreSortedLow(){
    return !nullsAreSortedHigh();
  }

  bool MariaDbDatabaseMetaData::nullsAreSortedAtStart(){
    return true;
  }

  bool MariaDbDatabaseMetaData::nullsAreSortedAtEnd(){
    return !nullsAreSortedAtStart();
  }

  /**
   * Return Server type. MySQL or MariaDB. MySQL can be forced for compatibility with option
   * "useMysqlMetadata"
   *
   * @return server type
   * @throws SQLException in case of socket error.
   */
  SQLString MariaDbDatabaseMetaData::getDatabaseProductName()
  {
    if (urlParser.getOptions()->useMysqlMetadata){
      return "MySQL";
    }
    if (connection->getProtocol()->isServerMariaDb())
    {
      SQLString svrVer(connection->getProtocol()->getServerVersion());

      if (StringImp::get(svrVer.toLowerCase()).find("mariadb") != std::string::npos)
      {
        return "MariaDB";
      }
    }
    return "MySQL";
  }

  SQLString MariaDbDatabaseMetaData::getDatabaseProductVersion(){
    return connection->getProtocol()->getServerVersion();
  }

  SQLString MariaDbDatabaseMetaData::getDriverName() {
    return DRIVER_NAME;
  }

  SQLString MariaDbDatabaseMetaData::getDriverVersion() {
    return Version::version;
  }

  int32_t MariaDbDatabaseMetaData::getDriverMajorVersion() {
    return Version::majorVersion;
  }

  int32_t MariaDbDatabaseMetaData::getDriverMinorVersion() {
    return Version::minorVersion;
  }


  int32_t MariaDbDatabaseMetaData::getDriverPatchVersion() {
    return Version::patchVersion;
  }


  bool MariaDbDatabaseMetaData::usesLocalFiles(){
    return false;
  }

  bool MariaDbDatabaseMetaData::usesLocalFilePerTable(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsMixedCaseIdentifiers() {
    return (connection->getLowercaseTableNames()==0);
  }

  bool MariaDbDatabaseMetaData::storesUpperCaseIdentifiers(){
    return false;
  }

  bool MariaDbDatabaseMetaData::storesLowerCaseIdentifiers() {
    return (connection->getLowercaseTableNames()==1);
  }

  bool MariaDbDatabaseMetaData::storesMixedCaseIdentifiers() {
    return (connection->getLowercaseTableNames()==2);
  }

  bool MariaDbDatabaseMetaData::supportsMixedCaseQuotedIdentifiers() {
    return supportsMixedCaseIdentifiers();
  }

  bool MariaDbDatabaseMetaData::storesUpperCaseQuotedIdentifiers(){
    return storesUpperCaseIdentifiers();
  }

  bool MariaDbDatabaseMetaData::storesLowerCaseQuotedIdentifiers() {
    return storesLowerCaseIdentifiers();
  }

  bool MariaDbDatabaseMetaData::storesMixedCaseQuotedIdentifiers() {
    return storesMixedCaseIdentifiers();
  }

  SQLString MariaDbDatabaseMetaData::getIdentifierQuoteString(){
    return "`";
  }

  SQLString MariaDbDatabaseMetaData::getSQLKeywords(){
    return "ACCESSIBLE,ANALYZE,ASENSITIVE,BEFORE,BIGINT,BINARY,BLOB,CALL,CHANGE,CONDITION,DATABASE,DATABASES,"
      "DAY_HOUR,DAY_MICROSECOND,DAY_MINUTE,DAY_SECOND,DELAYED,DETERMINISTIC,DISTINCTROW,DIV,DUAL,EACH,"
      "ELSEIF,ENCLOSED,ESCAPED,EXIT,EXPLAIN,FLOAT4,FLOAT8,FORCE,FULLTEXT,GENERAL,HIGH_PRIORITY,"
      "HOUR_MICROSECOND,HOUR_MINUTE,HOUR_SECOND,IF,IGNORE,IGNORE_SERVER_IDS,INDEX,INFILE,INOUT,INT1,INT2,"
      "INT3,INT4,INT8,ITERATE,KEY,KEYS,KILL,LEAVE,LIMIT,LINEAR,LINES,LOAD,LOCALTIME,LOCALTIMESTAMP,LOCK,"
      "LONG,LONGBLOB,LONGTEXT,LOOP,LOW_PRIORITY,MASTER_HEARTBEAT_PERIOD,MASTER_SSL_VERIFY_SERVER_CERT,"
      "MAXVALUE,MEDIUMBLOB,MEDIUMINT,MEDIUMTEXT,MIDDLEINT,MINUTE_MICROSECOND,MINUTE_SECOND,MOD,MODIFIES,"
      "NO_WRITE_TO_BINLOG,OPTIMIZE,OPTIONALLY,OUT,OUTFILE,PURGE,RANGE,READ_WRITE,READS,REGEXP,RELEASE,"
      "RENAME,REPEAT,REPLACE,REQUIRE,RESIGNAL,RESTRICT,RETURN,RLIKE,SCHEMAS,SECOND_MICROSECOND,SENSITIVE,"
      "SEPARATOR,SHOW,SIGNAL,SLOW,SPATIAL,SPECIFIC,SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,SQL_SMALL_RESULT,"
      "SQLEXCEPTION,SSL,STARTING,STRAIGHT_JOIN,TERMINATED,TINYBLOB,TINYINT,TINYTEXT,TRIGGER,UNDO,UNLOCK,"
      "UNSIGNED,USE,UTC_DATE,UTC_TIME,UTC_TIMESTAMP,VARBINARY,VARCHARACTER,WHILE,XOR,YEAR_MONTH,ZEROFILL";
  }

  SQLString MariaDbDatabaseMetaData::getNumericFunctions(){
    return "ABS,ACOS,ASIN,ATAN,ATAN2,BIT_COUNT,CEIL,CEILING,CONV,COS,COT,CRC32,DEGREES,DIV,EXP,FLOOR,GREATEST,LEAST,LN,LOG,"
      "LOG10,LOG2,MAX,MIN,MOD,OCT,PI,POW,POWER,RADIANS,RAND,ROUND,SIGN,SIN,SQRT,TAN,TRUNCATE";
  }

  SQLString MariaDbDatabaseMetaData::getStringFunctions(){
    return "ASCII,BIN,BIT_LENGTH,CAST,CHAR,CHARACTER_LENGTH,CHAR_LENGTH,CONCAT,CONCAT_WS,CONV,CONVERT,ELT,EXPORT_SET,"
      "EXTRACTVALUE,FIELD,FIND_IN_SET,FORMAT,FROM_BASE64,HEX,INSERT,INSTR,LCASE,LEFT,LENGTH,LIKE,LOAD_FILE,LOCATE,"
      "LOWER,LPAD,LTRIM,MAKE_SET,MATCH AGAINST,MID,NOT LIKE,NOT REGEXP,OCT,OCTET_LENGTH,ORD,POSITION,QUOTE,"
      "REPEAT,REPLACE,REVERSE,RIGHT,RPAD,RTRIM,SOUNDEX,SOUNDS LIKE,SPACE,STRCMP,SUBSTR,SUBSTRING,"
      "SUBSTRING_INDEX,TO_BASE64,TRIM,UCASE,UNHEX,UPDATEXML,UPPER,WEIGHT_STRING";
  }

  SQLString MariaDbDatabaseMetaData::getSystemFunctions(){
    return "DATABASE,USER,SYSTEM_USER,SESSION_USER,PASSWORD,ENCRYPT,LAST_INSERT_ID,VERSION";
  }

  SQLString MariaDbDatabaseMetaData::getTimeDateFunctions(){
    return "ADDDATE,ADDTIME,CONVERT_TZ,CURDATE,CURRENT_DATE,CURRENT_TIME,CURRENT_TIMESTAMP,CURTIME,DATEDIFF,"
      "DATE_ADD,DATE_FORMAT,DATE_SUB,DAY,DAYNAME,DAYOFMONTH,DAYOFWEEK,DAYOFYEAR,EXTRACT,FROM_DAYS,"
      "FROM_UNIXTIME,GET_FORMAT,HOUR,LAST_DAY,LOCALTIME,LOCALTIMESTAMP,MAKEDATE,MAKETIME,MICROSECOND,"
      "MINUTE,MONTH,MONTHNAME,NOW,PERIOD_ADD,PERIOD_DIFF,QUARTER,SECOND,SEC_TO_TIME,STR_TO_DATE,SUBDATE,"
      "SUBTIME,SYSDATE,TIMEDIFF,TIMESTAMPADD,TIMESTAMPDIFF,TIME_FORMAT,TIME_TO_SEC,TO_DAYS,TO_SECONDS,"
      "UNIX_TIMESTAMP,UTC_DATE,UTC_TIME,UTC_TIMESTAMP,WEEK,WEEKDAY,WEEKOFYEAR,YEAR,YEARWEEK";
  }

  SQLString MariaDbDatabaseMetaData::getSearchStringEscape(){
    return "\\";
  }

  SQLString MariaDbDatabaseMetaData::getExtraNameCharacters(){
    return "#@";
  }

  bool MariaDbDatabaseMetaData::supportsAlterTableWithAddColumn(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsAlterTableWithDropColumn(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsColumnAliasing(){
    return true;
  }

  bool MariaDbDatabaseMetaData::nullPlusNonNullIsNull(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsConvert(){
    return false;
  }

  /**
   * Retrieves whether this database supports the JDBC scalar function CONVERT for conversions
   * between the JDBC types fromType and toType. The JDBC types are the generic SQL data types
   * defined in java.sql.Types::
   *
   * @param fromType the type to convert from; one of the type codes from the class java.sql.Types
   * @param toType the type to convert to; one of the type codes from the class java.sql.Types
   * @return true if so; false otherwise
   */
  bool MariaDbDatabaseMetaData::supportsConvert(int32_t fromType,int32_t toType){
    switch (fromType){
      case Types::TINYINT:
      case Types::SMALLINT:
      case Types::INTEGER:
      case Types::BIGINT:
      case Types::REAL:
      case Types::FLOAT:
      case Types::DECIMAL:
      case Types::NUMERIC:
      case Types::DOUBLE:
      case Types::BIT:
      case Types::BOOLEAN:
        switch (toType){
          case Types::TINYINT:
          case Types::SMALLINT:
          case Types::INTEGER:
          case Types::BIGINT:
          case Types::REAL:
          case Types::FLOAT:
          case Types::DECIMAL:
          case Types::NUMERIC:
          case Types::DOUBLE:
          case Types::BIT:
          case Types::BOOLEAN:
          case Types::CHAR:
          case Types::VARCHAR:
          case Types::LONGVARCHAR:
          case Types::BINARY:
          case Types::VARBINARY:
          case Types::LONGVARBINARY:
            return true;
          default:
            return false;
        }

      case Types::BLOB:
        switch (toType){
          case Types::BINARY:
          case Types::VARBINARY:
          case Types::LONGVARBINARY:
          case Types::CHAR:
          case Types::VARCHAR:
          case Types::LONGVARCHAR:
          case Types::TINYINT:
          case Types::SMALLINT:
          case Types::INTEGER:
          case Types::BIGINT:
          case Types::REAL:
          case Types::FLOAT:
          case Types::DECIMAL:
          case Types::NUMERIC:
          case Types::DOUBLE:
          case Types::BIT:
          case Types::BOOLEAN:
            return true;
          default:
            return false;
        }

      case Types::CHAR:
      case Types::CLOB:
      case Types::VARCHAR:
      case Types::LONGVARCHAR:
      case Types::BINARY:
      case Types::VARBINARY:
      case Types::LONGVARBINARY:
        switch (toType){
          case Types::BIT:
          case Types::TINYINT:
          case Types::SMALLINT:
          case Types::INTEGER:
          case Types::BIGINT:
          case Types::FLOAT:
          case Types::REAL:
          case Types::DOUBLE:
          case Types::NUMERIC:
          case Types::DECIMAL:
          case Types::CHAR:
          case Types::VARCHAR:
          case Types::LONGVARCHAR:
          case Types::BINARY:
          case Types::VARBINARY:
          case Types::LONGVARBINARY:
          case Types::DATE:
          case Types::TIME:
          case Types::TIMESTAMP:
          case Types::BLOB:
          case Types::CLOB:
          case Types::BOOLEAN:
          case Types::NCHAR:
          case Types::LONGNVARCHAR:
          case Types::NCLOB:
            return true;
          default:
            return false;
        }

      case Types::DATE:
        switch (toType){
          case Types::DATE:
          case Types::CHAR:
          case Types::VARCHAR:
          case Types::LONGVARCHAR:
          case Types::BINARY:
          case Types::VARBINARY:
          case Types::LONGVARBINARY:
            return true;

          default:
            return false;
        }

      case Types::TIME:
        switch (toType){
          case Types::TIME:
          case Types::CHAR:
          case Types::VARCHAR:
          case Types::LONGVARCHAR:
          case Types::BINARY:
          case Types::VARBINARY:
          case Types::LONGVARBINARY:
            return true;
          default:
            return false;
        }

      case Types::TIMESTAMP:
        switch (toType){
          case Types::TIMESTAMP:
          case Types::CHAR:
          case Types::VARCHAR:
          case Types::LONGVARCHAR:
          case Types::BINARY:
          case Types::VARBINARY:
          case Types::LONGVARBINARY:
          case Types::TIME:
          case Types::DATE:
            return true;
          default:
            return false;
        }
      default:
        return false;
    }
  }

  bool MariaDbDatabaseMetaData::supportsTableCorrelationNames(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsDifferentTableCorrelationNames(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsExpressionsInOrderBy(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsOrderByUnrelated(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsGroupBy(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsGroupByUnrelated(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsGroupByBeyondSelect(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsLikeEscapeClause(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsMultipleResultSets(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsMultipleTransactions(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsNonNullableColumns(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsMinimumSQLGrammar(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsCoreSQLGrammar(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsExtendedSQLGrammar(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsANSI92EntryLevelSQL(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsANSI92IntermediateSQL(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsANSI92FullSQL(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsIntegrityEnhancementFacility(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsOuterJoins(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsFullOuterJoins(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsLimitedOuterJoins(){
    return true;
  }

  SQLString MariaDbDatabaseMetaData::getSchemaTerm(){
    return "schema";
  }

  SQLString MariaDbDatabaseMetaData::getProcedureTerm(){
    return "procedure";
  }

  SQLString MariaDbDatabaseMetaData::getCatalogTerm(){
    return emptyStr;
  }

  bool MariaDbDatabaseMetaData::isCatalogAtStart(){
    return true;
  }

  SQLString MariaDbDatabaseMetaData::getCatalogSeparator(){
    return emptyStr;
  }

  bool MariaDbDatabaseMetaData::supportsSchemasInDataManipulation(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSchemasInProcedureCalls(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSchemasInTableDefinitions(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSchemasInIndexDefinitions(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSchemasInPrivilegeDefinitions(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsCatalogsInDataManipulation(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsCatalogsInProcedureCalls(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsCatalogsInTableDefinitions(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsCatalogsInIndexDefinitions(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsCatalogsInPrivilegeDefinitions(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsPositionedDelete(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsPositionedUpdate(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsSelectForUpdate(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsStoredProcedures(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSubqueriesInComparisons(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSubqueriesInExists(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSubqueriesInIns(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsSubqueriesInQuantifieds(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsCorrelatedSubqueries(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsUnion(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsUnionAll(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsOpenCursorsAcrossCommit(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsOpenCursorsAcrossRollback(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsOpenStatementsAcrossCommit(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsOpenStatementsAcrossRollback(){
    return true;
  }

  int32_t MariaDbDatabaseMetaData::getMaxBinaryLiteralLength(){
    return 16777208;
  }

  int32_t MariaDbDatabaseMetaData::getMaxCharLiteralLength(){
    return 16777208;
  }

  int32_t MariaDbDatabaseMetaData::getMaxColumnNameLength(){
    return 64;
  }

  int32_t MariaDbDatabaseMetaData::getMaxColumnsInGroupBy(){
    return 64;
  }

  int32_t MariaDbDatabaseMetaData::getMaxColumnsInIndex(){
    return 16;
  }

  int32_t MariaDbDatabaseMetaData::getMaxColumnsInOrderBy(){
    return 64;
  }

  int32_t MariaDbDatabaseMetaData::getMaxColumnsInSelect(){
    return 256;
  }

  int32_t MariaDbDatabaseMetaData::getMaxColumnsInTable(){
    return 0;
  }

  int32_t MariaDbDatabaseMetaData::getMaxConnections(){
    Unique::ResultSet rs(executeQuery("SELECT @@max_connections"));
    if (rs && rs->next()) {
      return rs->getInt(1);
    }

    return 0;
  }

  int32_t MariaDbDatabaseMetaData::getMaxCursorNameLength(){
    return 64;
  }

  int32_t MariaDbDatabaseMetaData::getMaxIndexLength(){
    return 256;
  }

  int32_t MariaDbDatabaseMetaData::getMaxSchemaNameLength(){
    return 64;
  }

  int32_t MariaDbDatabaseMetaData::getMaxProcedureNameLength(){
    return 64;
  }

  int32_t MariaDbDatabaseMetaData::getMaxCatalogNameLength(){
    return 0;
  }

  int32_t MariaDbDatabaseMetaData::getMaxRowSize(){
    return 0;
  }

  bool MariaDbDatabaseMetaData::doesMaxRowSizeIncludeBlobs(){
    return false;
  }

  int32_t MariaDbDatabaseMetaData::getMaxStatementLength(){
    return 0;
  }

  int32_t MariaDbDatabaseMetaData::getMaxStatements(){
    return 0;
  }

  int32_t MariaDbDatabaseMetaData::getMaxTableNameLength(){
    return 64;
  }

  int32_t MariaDbDatabaseMetaData::getMaxTablesInSelect(){
    return 256;
  }

  int32_t MariaDbDatabaseMetaData::getMaxUserNameLength(){
    return 16;
  }

  int32_t MariaDbDatabaseMetaData::getDefaultTransactionIsolation(){
    return sql::TRANSACTION_REPEATABLE_READ;
  }

  /**
   * Retrieves whether this database supports transactions. If not, invoking the method <code>commit
   * </code> is a noop, and the isolation level is <code>TRANSACTION_NONE</code>.
   *
   * @return <code>true</code> if transactions are supported; <code>false</code> otherwise
   */
  bool MariaDbDatabaseMetaData::supportsTransactions(){
    return true;
  }

  /**
   * Retrieves whether this database supports the given transaction isolation level.
   *
   * @param level one of the transaction isolation levels defined in <code>java.sql.Connection
   *     </code>
   * @return <code>true</code> if so; <code>false</code> otherwise
   * @see Connection
   */
  bool MariaDbDatabaseMetaData::supportsTransactionIsolationLevel(int32_t level){
    switch (level){
      case sql::TRANSACTION_NONE:
      case sql::TRANSACTION_READ_UNCOMMITTED:
      case sql::TRANSACTION_READ_COMMITTED:
      case sql::TRANSACTION_REPEATABLE_READ:
      case sql::TRANSACTION_SERIALIZABLE:
        return true;
      default:
        return false;
    }
  }

  bool MariaDbDatabaseMetaData::supportsDataDefinitionAndDataManipulationTransactions(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsDataManipulationTransactionsOnly(){
    return false;
  }

  bool MariaDbDatabaseMetaData::dataDefinitionCausesTransactionCommit(){
    return true;
  }

  bool MariaDbDatabaseMetaData::dataDefinitionIgnoredInTransactions(){
    return false;
  }

  /**
   * Retrieves a description of the stored procedures available in the given catalog. Only procedure
   * descriptions matching the schema and procedure name criteria are returned. They are ordered by
   * <code>PROCEDURE_CAT</code>, <code>PROCEDURE_SCHEM</code>, <code>PROCEDURE_NAME</code> and
   * <code>SPECIFIC_ NAME</code>.
   *
   * <p>Each procedure description has the the following columns:
   *
   * <OL>
   *   <LI><B>PROCEDURE_CAT</B> String {@code= } procedure catalog (may be <code>null</code>)
   *   <LI><B>PROCEDURE_SCHEM</B> String {@code= } procedure schema (may be <code>null</code>)
   *   <LI><B>PROCEDURE_NAME</B> String {@code= } procedure name
   *   <LI>reserved for future use
   *   <LI>reserved for future use
   *   <LI>reserved for future use
   *   <LI><B>REMARKS</B> String {@code= } explanatory comment on the procedure
   *   <LI><B>PROCEDURE_TYPE</B> short {@code= } kind of procedure:
   *       <UL>
   *         <LI>procedureResultUnknown - Cannot determine if a return value will be returned
   *         <LI>procedureNoResult - Does not return a return value
   *         <LI>procedureReturnsResult - Returns a return value
   *       </UL>
   *   <LI><B>SPECIFIC_NAME</B> String {@code= } The name which uniquely identifies this procedure
   *       within its schema.
   * </OL>
   *
   * <p>A user may not have permissions to execute any of the procedures that are returned by <code>
   * getProcedures</code>
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
   *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
   *     should not be used to narrow the search
   * @param procedureNamePattern a procedure name pattern; must match the procedure name as it is
   *     stored in the database
   * @return <code>ResultSet</code> - each row is a procedure description
   * @throws SQLException if a database access error occurs
   * @see #getSearchStringEscape
   */
  ResultSet* MariaDbDatabaseMetaData::getProcedures(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& procedureNamePattern)
  {


    SQLString sql =
      "SELECT NULL PROCEDURE_CAT, ROUTINE_SCHEMA PROCEDURE_SCHEM, ROUTINE_NAME PROCEDURE_NAME,"
      " NULL RESERVED1, NULL RESERVED2, NULL RESERVED3, ROUTINE_COMMENT REMARKS,"
      " CASE ROUTINE_TYPE "
      "  WHEN 'FUNCTION' THEN "
      + std::to_string(procedureReturnsResult)
      +"  WHEN 'PROCEDURE' THEN "
      + std::to_string(procedureNoResult)
      +"  ELSE "
      + std::to_string(procedureResultUnknown)
      +" END PROCEDURE_TYPE,"
      "  SPECIFIC_NAME "
      " FROM INFORMATION_SCHEMA.ROUTINES "
      " WHERE "
      + (schemaPattern.empty() ? catalogCond("ROUTINE_SCHEMA", schemaPattern) : patternCond("ROUTINE_SCHEMA", schemaPattern))
      +" AND "
      + patternCond("ROUTINE_NAME", procedureNamePattern)
      +"/* AND ROUTINE_TYPE='PROCEDURE' */";
    return executeQuery(sql);
  }

  /* Is INFORMATION_SCHEMA.PARAMETERS available ?*/  bool MariaDbDatabaseMetaData::haveInformationSchemaParameters(){
    return connection->getProtocol()->versionGreaterOrEqual(5,5,3);
  }

  /**
   * Retrieves a description of the given catalog's stored procedure parameter and result columns.
   *
   * <p>Only descriptions matching the schema, procedure and parameter name criteria are returned.
   * They are ordered by PROCEDURE_CAT, PROCEDURE_SCHEM, PROCEDURE_NAME and SPECIFIC_NAME. Within
   * this, the return value, if any, is first. Next are the parameter descriptions in call order.
   * The column descriptions follow in column number order.
   *
   * <p>Each row in the <code>ResultSet</code> is a parameter description or column description with
   * the following fields:
   *
   * <OL>
   *   <LI><B>PROCEDURE_CAT</B> String {@code= } procedure catalog (may be <code>null</code>)
   *   <LI><B>PROCEDURE_SCHEM</B> String {@code= } procedure schema (may be <code>null</code>)
   *   <LI><B>PROCEDURE_NAME</B> String {@code= } procedure name
   *   <LI><B>COLUMN_NAME</B> String {@code= } column/parameter name
   *   <LI><B>COLUMN_TYPE</B> Short {@code= } kind of column/parameter:
   *       <UL>
   *         <LI>procedureColumnUnknown - nobody knows
   *         <LI>procedureColumnIn - IN parameter
   *         <LI>procedureColumnInOut - INOUT parameter
   *         <LI>procedureColumnOut - OUT parameter
   *         <LI>procedureColumnReturn - procedure return value
   *         <LI>procedureColumnResult - result column in <code>ResultSet</code>
   *       </UL>
   *   <LI><B>DATA_TYPE</B> int {@code= } SQL type from java.sql.Types
   *   <LI><B>TYPE_NAME</B> String {@code= } SQL type name, for a UDT type the type name is fully
   *       qualified
   *   <LI><B>PRECISION</B> int {@code= } precision
   *   <LI><B>LENGTH</B> int {@code= } length in bytes of data
   *   <LI><B>SCALE</B> short {@code= } scale - null is returned for data types where SCALE is not
   *       applicable.
   *   <LI><B>RADIX</B> short {@code= } radix
   *   <LI><B>NULLABLE</B> short {@code= } can it contain NULL.
   *       <UL>
   *         <LI>procedureNoNulls - does not allow NULL values
   *         <LI>procedureNullable - allows NULL values
   *         <LI>procedureNullableUnknown - nullability unknown
   *       </UL>
   *   <LI><B>REMARKS</B> String {@code= } comment describing parameter/column
   *   <LI><B>COLUMN_DEF</B> String {@code= } default value for the column, which should be
   *       interpreted as a string when the value is enclosed in single quotes (may be <code>null
   *       </code>)
   *       <UL>
   *         <LI>The string NULL (not enclosed in quotes) - if NULL was specified as the default
   *             value
   *         <LI>TRUNCATE (not enclosed in quotes) - if the specified default value cannot be
   *             represented without truncation
   *         <LI>NULL - if a default value was not specified
   *       </UL>
   *   <LI><B>SQL_DATA_TYPE</B> int {@code= } reserved for future use
   *   <LI><B>SQL_DATETIME_SUB</B> int {@code= } reserved for future use
   *   <LI><B>CHAR_OCTET_LENGTH</B> int {@code= } the maximum length of binary and character based
   *       columns. For any other datatype the returned value is a NULL
   *   <LI><B>ORDINAL_POSITION</B> int {@code= } the ordinal position, starting from 1, for the
   *       input and output parameters for a procedure. A value of 0 is returned if this row
   *       describes the procedure's return value. For result set columns, it is the ordinal
   *       position of the column in the result set starting from 1. If there are multiple result
   *       sets, the column ordinal positions are implementation defined.
   *   <LI><B>IS_NULLABLE</B> String {@code= } ISO rules are used to determine the nullability for
   *       a column.
   *       <UL>
   *         <LI>YES --- if the column can include NULLs
   *         <LI>NO --- if the column cannot include NULLs
   *         <LI>empty string --- if the nullability for the column is unknown
   *       </UL>
   *   <LI><B>SPECIFIC_NAME</B> String {@code= } the name which uniquely identifies this procedure
   *       within its schema.
   * </OL>
   *
   * <p><B>Note:</B> Some databases may not return the column descriptions for a procedure.
  *
    * <p>The PRECISION column represents the specified column size for the given column. For numeric
    * data, this is the maximum precision. For character data, this is the length in characters. For
    * datetime datatypes, this is the length in characters of the String representation (assuming the
        * maximum allowed precision of the fractional seconds component). For binary data, this is the
    * length in bytes. For the ROWID datatype, this is the length in bytes. Null is returned for data
    * types where the column size is not applicable.
    *
    * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
    *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
    *     be used to narrow the search
    * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
    *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
    *     should not be used to narrow the search
    * @param procedureNamePattern a procedure name pattern; must match the procedure name as it is
    *     stored in the database
    * @param columnNamePattern a column name pattern; must match the column name as it is stored in
    *     the database
    * @return <code>ResultSet</code> - each row describes a stored procedure parameter or column
    * @throws SQLException if a database access error occurs
    * @see #getSearchStringEscape
    */
  ResultSet* MariaDbDatabaseMetaData::getProcedureColumns(
    const SQLString& catalog, const SQLString& schemaPattern, const SQLString& procedureNamePattern, const SQLString& columnNamePattern)
  {
    SQLString sql;

    if (haveInformationSchemaParameters()) {
      /*
      *  Get info from information_schema.parameters
      */
      sql= "SELECT SPECIFIC_SCHEMA PROCEDURE_CAT, NULL PROCEDURE_SCHEM, SPECIFIC_NAME PROCEDURE_NAME,"
          " PARAMETER_NAME COLUMN_NAME, "
          " CASE PARAMETER_MODE "
          "  WHEN 'IN' THEN "
        + std::to_string(procedureColumnIn)
        + "  WHEN 'OUT' THEN "
        + std::to_string(procedureColumnOut)
        + "  WHEN 'INOUT' THEN "
        + std::to_string(procedureColumnInOut)
        + "  ELSE IF(PARAMETER_MODE IS NULL,"
        + std::to_string(procedureColumnReturn)
        + ","
        + std::to_string(procedureColumnUnknown)
        + ")"
          " END COLUMN_TYPE,"
        + dataTypeClause("DTD_IDENTIFIER")
        + " DATA_TYPE,"
          "DATA_TYPE TYPE_NAME,"
          " CASE DATA_TYPE"
          "  WHEN 'time' THEN "
        + (datePrecisionColumnExist
          ? "IF(DATETIME_PRECISION = 0, 10, CAST(11 + DATETIME_PRECISION as signed integer))"
          : "10")
        + "  WHEN 'date' THEN 10"
          "  WHEN 'datetime' THEN "
        + (datePrecisionColumnExist
          ? "IF(DATETIME_PRECISION = 0, 19, CAST(20 + DATETIME_PRECISION as signed integer))"
          : "19")
        + "  WHEN 'timestamp' THEN "
        + (datePrecisionColumnExist
          ? "IF(DATETIME_PRECISION = 0, 19, CAST(20 + DATETIME_PRECISION as signed integer))"
          : "19")
        + "  ELSE "
          "  IF(NUMERIC_PRECISION IS NULL, LEAST(CHARACTER_MAXIMUM_LENGTH,"
        + std::to_string(INT32_MAX)
        + "), NUMERIC_PRECISION) "
          " END `PRECISION`,"
          " CASE DATA_TYPE"
          "  WHEN 'time' THEN "
        + (datePrecisionColumnExist
          ? "IF(DATETIME_PRECISION = 0, 10, CAST(11 + DATETIME_PRECISION as signed integer))"
          : "10")
        + "  WHEN 'date' THEN 10"
          "  WHEN 'datetime' THEN "
        + (datePrecisionColumnExist
          ? "IF(DATETIME_PRECISION = 0, 19, CAST(20 + DATETIME_PRECISION as signed integer))"
          : "19")
        + "  WHEN 'timestamp' THEN "
        + (datePrecisionColumnExist
          ? "IF(DATETIME_PRECISION = 0, 19, CAST(20 + DATETIME_PRECISION as signed integer))"
          : "19")
        + "  ELSE "
          "  IF(NUMERIC_PRECISION IS NULL, LEAST(CHARACTER_MAXIMUM_LENGTH,"
        + std::to_string(INT32_MAX)
        + "), NUMERIC_PRECISION) "
        + " END `LENGTH`,"
        + (datePrecisionColumnExist
          ? " CASE DATA_TYPE"
            "  WHEN 'time' THEN CAST(DATETIME_PRECISION as signed integer)"
            "  WHEN 'datetime' THEN CAST(DATETIME_PRECISION as signed integer)"
            "  WHEN 'timestamp' THEN CAST(DATETIME_PRECISION as signed integer)"
            "  ELSE NUMERIC_SCALE "
            " END `SCALE`,"
          : " NUMERIC_SCALE `SCALE`,")
        + "10 RADIX,"
        + std::to_string(procedureNullableUnknown)
        + " NULLABLE,NULL REMARKS,NULL COLUMN_DEF,0 SQL_DATA_TYPE,0 SQL_DATETIME_SUB,"
          "CHARACTER_OCTET_LENGTH CHAR_OCTET_LENGTH ,ORDINAL_POSITION, '' IS_NULLABLE, SPECIFIC_NAME "
          " FROM INFORMATION_SCHEMA.PARAMETERS "
          " WHERE "
        + catalogCond("SPECIFIC_SCHEMA", catalog)
        + " AND "
        + patternCond("SPECIFIC_NAME", procedureNamePattern)
        + " AND "
        + patternCond("PARAMETER_NAME", columnNamePattern)
        + " /* AND ROUTINE_TYPE='PROCEDURE' */ "
          " ORDER BY SPECIFIC_SCHEMA, SPECIFIC_NAME, ORDINAL_POSITION";
    }
    else {
      /* No information_schema.parameters
      * TODO : figure out what to do with older versions (get info via mysql.proc)
      * For now, just a dummy result set is returned.
      */
      sql=
        "SELECT '' PROCEDURE_CAT, '' PROCEDURE_SCHEM , '' PROCEDURE_NAME,'' COLUMN_NAME, 0 COLUMN_TYPE,"
        "0 DATA_TYPE,'' TYPE_NAME, 0 `PRECISION`,0 LENGTH, 0 SCALE,10 RADIX,"
        "0 NULLABLE,NULL REMARKS,NULL COLUMN_DEF,0 SQL_DATA_TYPE,0 SQL_DATETIME_SUB,"
        "0 CHAR_OCTET_LENGTH ,0 ORDINAL_POSITION, '' IS_NULLABLE, '' SPECIFIC_NAME "
        " FROM DUAL "
        " WHERE 1=0 ";
    }

    try {
      return executeQuery(sql);
    }
    catch (SQLException& sqlException) {
      if (StringImp::get(sqlException.getMessage()).find("Unknown column 'DATETIME_PRECISION'") != std::string::npos) {
        datePrecisionColumnExist = false;
        return getProcedureColumns(catalog, schemaPattern, procedureNamePattern, columnNamePattern);
      }
      throw sqlException;
    }
  }
/**
  * Retrieves a description of the given catalog's system or user function parameters and return
  * type.
  *
  * <p>Only descriptions matching the schema, function and parameter name criteria are returned.
  * They are ordered by <code>FUNCTION_CAT</code>, <code>FUNCTION_SCHEM</code>, <code>FUNCTION_NAME
  * </code> and <code>SPECIFIC_ NAME</code>. Within this, the return value, if any, is first. Next
  * are the parameter descriptions in call order. The column descriptions follow in column number
  * order.
  *
  * <p>Each row in the <code>ResultSet</code> is a parameter description, column description or
  * return type description with the following fields:
  *
  * <OL>
  *   <LI><B>FUNCTION_CAT</B> String {@code= } function catalog (may be <code>null</code>)
  *   <LI><B>FUNCTION_SCHEM</B> String {@code= } function schema (may be <code>null</code>)
  *   <LI><B>FUNCTION_NAME</B> String {@code= } function name. This is the name used to invoke the
  *       function
  *   <LI><B>COLUMN_NAME</B> String {@code= } column/parameter name
  *   <LI><B>COLUMN_TYPE</B> Short {@code= } kind of column/parameter:
  *       <UL>
  *         <LI>functionColumnUnknown - nobody knows
  *         <LI>functionColumnIn - IN parameter
  *         <LI>functionColumnInOut - INOUT parameter
  *         <LI>functionColumnOut - OUT parameter
  *         <LI>functionColumnReturn - function return value
  *         <LI>functionColumnResult - Indicates that the parameter or column is a column in the
  *             <code>ResultSet</code>
  *       </UL>
  *   <LI><B>DATA_TYPE</B> int {@code= } SQL type from java.sql.Types
  *   <LI><B>TYPE_NAME</B> String {@code= } SQL type name, for a UDT type the type name is fully
  *       qualified
  *   <LI><B>PRECISION</B> int {@code= } precision
  *   <LI><B>LENGTH</B> int {@code= } length in bytes of data
  *   <LI><B>SCALE</B> short {@code= } scale - null is returned for data types where SCALE is not
  *       applicable.
  *   <LI><B>RADIX</B> short {@code= } radix
  *   <LI><B>NULLABLE</B> short {@code= } can it contain NULL.
  *       <UL>
  *         <LI>functionNoNulls - does not allow NULL values
  *         <LI>functionNullable - allows NULL values
  *         <LI>functionNullableUnknown - nullability unknown
  *       </UL>
  *   <LI><B>REMARKS</B> String {@code= } comment describing column/parameter
  *   <LI><B>CHAR_OCTET_LENGTH</B> int {@code= } the maximum length of binary and character based
  *       parameters or columns. For any other datatype the returned value is a NULL
  *   <LI><B>ORDINAL_POSITION</B> int {@code= } the ordinal position, starting from 1, for the
  *       input and output parameters. A value of 0 is returned if this row describes the
  *       function's return value. For result set columns, it is the ordinal position of the column
  *       in the result set starting from 1.
  *   <LI><B>IS_NULLABLE</B> String {@code= } ISO rules are used to determine the nullability for
  *       a parameter or column.
  *       <UL>
  *         <LI>YES --- if the parameter or column can include NULLs
  *         <LI>NO --- if the parameter or column cannot include NULLs
  *         <LI>empty string --- if the nullability for the parameter or column is unknown
  *       </UL>
  *   <LI><B>SPECIFIC_NAME</B> String {@code= } the name which uniquely identifies this function
  *       within its schema. This is a user specified, or DBMS generated, name that may be
  *       different then the <code>FUNCTION_NAME</code> for example with overload functions
  * </OL>
  *
  * <p>The PRECISION column represents the specified column size for the given parameter or column.
  * For numeric data, this is the maximum precision. For character data, this is the length in
  * characters. For datetime datatypes, this is the length in characters of the String
  * representation (assuming the maximum allowed precision of the fractional seconds component).
  * For binary data, this is the length in bytes. For the ROWID datatype, this is the length in
  * bytes. Null is returned for data types where the column size is not applicable.
  *
  * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
  *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
*     be used to narrow the search
  * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
  *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
  *     should not be used to narrow the search
  * @param functionNamePattern a procedure name pattern; must match the function name as it is
  *     stored in the database
  * @param columnNamePattern a parameter name pattern; must match the parameter or column name as
  *     it is stored in the database
  * @return <code>ResultSet</code> - each row describes a user function parameter, column or return
  *     type
  * @throws SQLException if a database access error occurs
  * @see #getSearchStringEscape
  * @since 1.6
  */
  ResultSet* MariaDbDatabaseMetaData::getFunctionColumns(
    const SQLString& catalog, const SQLString& schemaPattern, const SQLString& functionNamePattern, const SQLString& columnNamePattern)
  {
    SQLString sql;
    if (haveInformationSchemaParameters()) {

      sql= "SELECT SPECIFIC_SCHEMA `FUNCTION_CAT`, NULL `FUNCTION_SCHEM`, SPECIFIC_NAME FUNCTION_NAME,"
          " PARAMETER_NAME COLUMN_NAME, "
          " CASE PARAMETER_MODE "
          "  WHEN 'IN' THEN "
        + std::to_string(functionColumnIn)
        + "  WHEN 'OUT' THEN "
        + std::to_string(functionColumnOut)
        + "  WHEN 'INOUT' THEN "
        + std::to_string(functionColumnInOut)
        + "  ELSE "
        + std::to_string(functionReturn)
        + " END COLUMN_TYPE,"
        + dataTypeClause("DTD_IDENTIFIER")
        + " DATA_TYPE,"
          "DATA_TYPE TYPE_NAME,NUMERIC_PRECISION `PRECISION`,CHARACTER_MAXIMUM_LENGTH LENGTH,NUMERIC_SCALE SCALE,10 RADIX,"
        + std::to_string(procedureNullableUnknown)
        + " NULLABLE,NULL REMARKS,"
          "CHARACTER_OCTET_LENGTH CHAR_OCTET_LENGTH ,ORDINAL_POSITION, '' IS_NULLABLE, SPECIFIC_NAME "
          " FROM INFORMATION_SCHEMA.PARAMETERS "
          " WHERE "
        + catalogCond("SPECIFIC_SCHEMA", catalog)
        + " AND "
        + patternCond("SPECIFIC_NAME", functionNamePattern)
        + " AND "
        + patternCond("PARAMETER_NAME", columnNamePattern)
        + " AND ROUTINE_TYPE='FUNCTION'"
          " ORDER BY FUNCTION_CAT, SPECIFIC_NAME, ORDINAL_POSITION";
    }
    else {
      /*
      * No information_schema.parameters
      * TODO : figure out what to do with older versions (get info via mysql.proc)
      * For now, just a dummy result set is returned.
      */
      sql=
        "SELECT '' FUNCTION_CAT, NULL FUNCTION_SCHEM, '' FUNCTION_NAME,"
        " '' COLUMN_NAME, 0  COLUMN_TYPE, 0 DATA_TYPE,"
        " '' TYPE_NAME,0 `PRECISION`,0 LENGTH, 0 SCALE,0 RADIX,"
        " 0 NULLABLE,NULL REMARKS, 0 CHAR_OCTET_LENGTH , 0 ORDINAL_POSITION, "
        " '' IS_NULLABLE, '' SPECIFIC_NAME "
        " FROM DUAL WHERE 1=0 ";
    }
    return executeQuery(sql);
  }


  ResultSet* MariaDbDatabaseMetaData::getSchemas() {
    return executeQuery("SELECT SCHEMA_NAME TABLE_SCHEM FROM INFORMATION_SCHEMA.SCHEMATA ORDER BY 1");
  }


  ResultSet* MariaDbDatabaseMetaData::getSchemas(const SQLString& catalog, const SQLString& schemaPattern) {
    // TODO: need obviously smth more than that
    std::ostringstream query("SELECT SCHEMA_NAME TABLE_SCHEM, '' TABLE_CATALOG  FROM INFORMATION_SCHEMA.SCHEMATA ", std::ios_base::ate);

    if (!catalog.empty() && catalog.compare("def") != 0) {
      query << "WHERE 1=0 ";
      return executeQuery(query.str());
    }

    if (!schemaPattern.empty()) {
      query << "WHERE SCHEMA_NAME=" << escapeQuote(schemaPattern) << " ";
    }
    query << "ORDER BY 1";
    return executeQuery(query.str());
  }


  ResultSet* MariaDbDatabaseMetaData::getCatalogs() {
    return executeQuery("SELECT '' TABLE_CAT FROM DUAL WHERE 1=0");
  }

  ResultSet* MariaDbDatabaseMetaData::getTableTypes() {
    return executeQuery(
        "SELECT 'TABLE' TABLE_TYPE UNION SELECT 'SYSTEM VIEW' TABLE_TYPE UNION SELECT 'VIEW' TABLE_TYPE");
  }

  /**
    * Retrieves a description of the access rights for a table's columns.
    *
    * <p>Only privileges matching the column name criteria are returned. They are ordered by
    * COLUMN_NAME and PRIVILEGE.
    *
    * <p>Each privilege description has the following columns:
    *
    * <OL>
    *   <LI><B>TABLE_CAT</B> String {@code= } table catalog (may be <code>null</code>)
    *   <LI><B>TABLE_SCHEM</B> String {@code= } table schema (may be <code>null</code>)
    *   <LI><B>TABLE_NAME</B> String {@code= } table name
    *   <LI><B>COLUMN_NAME</B> String {@code= } column name
    *   <LI><B>GRANTOR</B> String {@code= } grantor of access (may be <code>null</code>)
    *   <LI><B>GRANTEE</B> String {@code= } grantee of access
    *   <LI><B>PRIVILEGE</B> String {@code= } name of access (SELECT, INSERT, UPDATE, REFRENCES,
    *       ...)
    *   <LI><B>IS_GRANTABLE</B> String {@code= } "YES" if grantee is permitted to grant to others;
    *       "NO" if not; <code>null</code> if unknown
    * </OL>
    *
    * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
    *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
    *     be used to narrow the search
    * @param schema a schema name; must match the schema name as it is stored in the database; ""
    *     retrieves those without a schema; <code>null</code> means that the schema name should not
    *     be used to narrow the search
    * @param table a table name; must match the table name as it is stored in the database
    * @param columnNamePattern a column name pattern; must match the column name as it is stored in
    *     the database
    * @return <code>ResultSet</code> - each row is a column privilege description
    * @throws SQLException if a database access error occurs
    * @see #getSearchStringEscape
    */
  ResultSet* MariaDbDatabaseMetaData::getColumnPrivileges(
      const SQLString& catalog, const SQLString& schema, const SQLString& table, const SQLString& columnNamePattern) {


    if (table.empty() == true){
      throw SQLException("'table' parameter must not be empty");
    }
    SQLString sql =
      "SELECT NULL TABLE_CAT, TABLE_SCHEMA TABLE_SCHEM, TABLE_NAME,"
      " COLUMN_NAME, NULL AS GRANTOR, GRANTEE, PRIVILEGE_TYPE AS PRIVILEGE, IS_GRANTABLE FROM "
      " INFORMATION_SCHEMA.COLUMN_PRIVILEGES WHERE "
      + catalogCond("TABLE_SCHEMA", schema)
      +" AND "
      " TABLE_NAME = "
      + escapeQuote(table)
      +" AND "
      + patternCond("COLUMN_NAME", columnNamePattern)
      +" ORDER BY COLUMN_NAME, PRIVILEGE_TYPE";

    return executeQuery(sql);
  }

  /**
    * Retrieves a description of the access rights for each table available in a catalog. Note that a
    * table privilege applies to one or more columns in the table. It would be wrong to assume that
    * this privilege applies to all columns (this may be true for some systems but is not true for
    * all.)
    *
    * <p>Only privileges matching the schema and table name criteria are returned. They are ordered
    * by <code>TABLE_CAT</code>, <code>TABLE_SCHEM</code>, <code>TABLE_NAME</code>, and <code>
    * PRIVILEGE</code>.
    *
    * <p>Each privilege description has the following columns:
    *
    * <OL>
    *   <LI><B>TABLE_CAT</B> String {@code= } table catalog (may be <code>null</code>)
    *   <LI><B>TABLE_SCHEM</B> String {@code= } table schema (may be <code>null</code>)
    *   <LI><B>TABLE_NAME</B> String {@code= } table name
    *   <LI><B>GRANTOR</B> String {@code= } grantor of access (may be <code>null</code>)
    *   <LI><B>GRANTEE</B> String {@code= } grantee of access
    *   <LI><B>PRIVILEGE</B> String {@code= } name of access (SELECT, INSERT, UPDATE, REFRENCES,
    *       ...)
    *   <LI><B>IS_GRANTABLE</B> String {@code= } "YES" if grantee is permitted to grant to others;
    *       "NO" if not; <code>null</code> if unknown
    * </OL>
    *
    * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
    *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
    *     be used to narrow the search
    * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
    *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
    *     should not be used to narrow the search
    * @param tableNamePattern a table name pattern; must match the table name as it is stored in the
    *     database
    * @return <code>ResultSet</code> - each row is a table privilege description
    * @throws SQLException if a database access error occurs
    * @see #getSearchStringEscape
    */
  ResultSet* MariaDbDatabaseMetaData::getTablePrivileges(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern)
  {
    SQLString sql =
      "SELECT TABLE_SCHEMA TABLE_CAT,NULL  TABLE_SCHEM, TABLE_NAME, NULL GRANTOR,"
      "GRANTEE, PRIVILEGE_TYPE  PRIVILEGE, IS_GRANTABLE  FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
      " WHERE "
      +catalogCond("TABLE_SCHEMA",catalog)
      +" AND "
      +patternCond("TABLE_NAME",tableNamePattern)
      +"ORDER BY TABLE_SCHEMA, TABLE_NAME,  PRIVILEGE_TYPE ";

    return executeQuery(sql);
  }

  /**
    * Retrieves a description of a table's columns that are automatically updated when any value in a
    * row is updated. They are unordered.
    *
    * <p>Each column description has the following columns:
    *
    * <OL>
    *   <LI><B>SCOPE</B> short {@code= } is not used
    *   <LI><B>COLUMN_NAME</B> String {@code= } column name
    *   <LI><B>DATA_TYPE</B> int {@code= } SQL data type from <code>java.sql.Types</code>
    *   <LI><B>TYPE_NAME</B> String {@code= } Data source-dependent type name
    *   <LI><B>COLUMN_SIZE</B> int {@code= } precision
    *   <LI><B>BUFFER_LENGTH</B> int {@code= } length of column value in bytes
    *   <LI><B>DECIMAL_DIGITS</B> short {@code= } scale - Null is returned for data types where
    *       DECIMAL_DIGITS is not applicable.
    *   <LI><B>PSEUDO_COLUMN</B> short {@code= } whether this is pseudo column like an Oracle ROWID
    *       <UL>
    *         <LI>versionColumnUnknown - may or may not be pseudo column
    *         <LI>versionColumnNotPseudo - is NOT a pseudo column
    *         <LI>versionColumnPseudo - is a pseudo column
    *       </UL>
    * </OL>
    *
    * <p>The COLUMN_SIZE column represents the specified column size for the given column. For
    * numeric data, this is the maximum precision. For character data, this is the length in
    * characters. For datetime datatypes, this is the length in characters of the String
    * representation (assuming the maximum allowed precision of the fractional seconds component).
    * For binary data, this is the length in bytes. For the ROWID datatype, this is the length in
    * bytes. Null is returned for data types where the column size is not applicable.
    *
    * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
    *     retrieves those without a catalog;<code>null</code> means that the catalog name should not
    *     be used to narrow the search
    * @param schema a schema name; must match the schema name as it is stored in the database; ""
    *     retrieves those without a schema; <code>null</code> means that the schema name should not
    *     be used to narrow the search
    * @param table a table name; must match the table name as it is stored in the database
    * @return a <code>ResultSet</code> object in which each row is a column description
    * @throws SQLException if a database access error occurs
    */
  ResultSet* MariaDbDatabaseMetaData::getVersionColumns(const SQLString& catalog, const SQLString& schema, const SQLString& table)  {
    SQLString sql =
      "SELECT 0 SCOPE, ' ' COLUMN_NAME, 0 DATA_TYPE,"
      " ' ' TYPE_NAME, 0 COLUMN_SIZE, 0 BUFFER_LENGTH,"
      " 0 DECIMAL_DIGITS, 0 PSEUDO_COLUMN "
      " FROM DUAL WHERE 1 = 0";
    return executeQuery(sql);
  }

/**
  * Retrieves a description of the foreign key columns in the given foreign key table that
  * reference the primary key or the columns representing a unique constraint of the parent table
  * (could be the same or a different table). The number of columns returned from the parent table
  * must match the number of columns that make up the foreign key. They are ordered by FKTABLE_CAT,
  * FKTABLE_SCHEM, FKTABLE_NAME, and KEY_SEQ.
  *
  * <p>Each foreign key column description has the following columns:
  *
  * <OL>
  *   <LI><B>PKTABLE_CAT</B> String {@code= } parent key table catalog (may be <code>null</code>)
  *   <LI><B>PKTABLE_SCHEM</B> String {@code= } parent key table schema (may be <code>null</code>)
  *   <LI><B>PKTABLE_NAME</B> String {@code= } parent key table name
  *   <LI><B>PKCOLUMN_NAME</B> String {@code= } parent key column name
  *   <LI><B>FKTABLE_CAT</B> String {@code= } foreign key table catalog (may be <code>null</code>)
  *       being exported (may be <code>null</code>)
  *   <LI><B>FKTABLE_SCHEM</B> String {@code= } foreign key table schema (may be <code>null</code>
  *       ) being exported (may be <code>null</code>)
  *   <LI><B>FKTABLE_NAME</B> String {@code= } foreign key table name being exported
  *   <LI><B>FKCOLUMN_NAME</B> String {@code= } foreign key column name being exported
  *   <LI><B>KEY_SEQ</B> short {@code= } sequence number within foreign key( a value of 1
  *       represents the first column of the foreign key, a value of 2 would represent the second
  *       column within the foreign key).
  *   <LI><B>UPDATE_RULE</B> short {@code= } What happens to foreign key when parent key is
  *       updated:
  *       <UL>
  *         <LI>importedNoAction - do not allow update of parent key if it has been imported
  *         <LI>importedKeyCascade - change imported key to agree with parent key update
  *         <LI>importedKeySetNull - change imported key to <code>NULL</code> if its parent key has
  *             been updated
  *         <LI>importedKeySetDefault - change imported key to default values if its parent key has
  *             been updated
  *         <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x compatibility)
  *       </UL>
  *   <LI><B>DELETE_RULE</B> short {@code= } What happens to the foreign key when parent key is
  *       deleted.
  *       <UL>
  *         <LI>importedKeyNoAction - do not allow delete of parent key if it has been imported
  *         <LI>importedKeyCascade - delete rows that import a deleted key
  *         <LI>importedKeySetNull - change imported key to <code>NULL</code> if its primary key
  *             has been deleted
  *         <LI>importedKeyRestrict - same as importedKeyNoAction (for ODBC 2.x compatibility)
  *         <LI>importedKeySetDefault - change imported key to default if its parent key has been
  *             deleted
  *       </UL>
  *   <LI><B>FK_NAME</B> String {@code= } foreign key name (may be <code>null</code>)
  *   <LI><B>PK_NAME</B> String {@code= } parent key name (may be <code>null</code>)
  *   <LI><B>DEFERRABILITY</B> short {@code= } can the evaluation of foreign key constraints be
  *       deferred until commit
  *       <UL>
  *         <LI>importedKeyInitiallyDeferred - see SQL92 for definition
  *         <LI>importedKeyInitiallyImmediate - see SQL92 for definition
  *         <LI>importedKeyNotDeferrable - see SQL92 for definition
  *       </UL>
  * </OL>
  *
  * @param parentCatalog a catalog name; must match the catalog name as it is stored in the
  *     database; "" retrieves those without a catalog; <code>null</code> means drop catalog name
  *     from the selection criteria
  * @param parentSchema a schema name; must match the schema name as it is stored in the database;
  *     "" retrieves those without a schema; <code>null</code> means drop schema name from the
  *     selection criteria
  * @param parentTable the name of the table that exports the key; must match the table name as it
  *     is stored in the database
  * @param foreignCatalog a catalog name; must match the catalog name as it is stored in the
  *     database; "" retrieves those without a catalog; <code>null</code> means drop catalog name
  *     from the selection criteria
  * @param foreignSchema a schema name; must match the schema name as it is stored in the database;
  *     "" retrieves those without a schema; <code>null</code> means drop schema name from the
  *     selection criteria
  * @param foreignTable the name of the table that imports the key; must match the table name as it
*     is stored in the database
  * @return <code>ResultSet</code> - each row is a foreign key column description
  * @throws SQLException if a database access error occurs
  * @see #getImportedKeys
  */
  ResultSet* MariaDbDatabaseMetaData::getCrossReference(
      const SQLString& parentCatalog, const SQLString& parentSchema, const SQLString& parentTable, const SQLString& foreignCatalog, const SQLString& foreignSchema, const SQLString& foreignTable)
  {
    SQLString sql =
      "SELECT NULL PKTABLE_CAT, KCU.REFERENCED_TABLE_SCHEMA PKTABLE_SCHEM, KCU.REFERENCED_TABLE_NAME PKTABLE_NAME,"
      " KCU.REFERENCED_COLUMN_NAME PKCOLUMN_NAME, NULL FKTABLE_CAT, KCU.TABLE_SCHEMA FKTABLE_SCHEM, "
      " KCU.TABLE_NAME FKTABLE_NAME, KCU.COLUMN_NAME FKCOLUMN_NAME, KCU.POSITION_IN_UNIQUE_CONSTRAINT KEY_SEQ,"
      " CASE update_rule "
      "   WHEN 'RESTRICT' THEN 1"
      "   WHEN 'NO ACTION' THEN 3"
      "   WHEN 'CASCADE' THEN 0"
      "   WHEN 'SET NULL' THEN 2"
      "   WHEN 'SET DEFAULT' THEN 4"
      " END UPDATE_RULE,"
      " CASE DELETE_RULE"
      "  WHEN 'RESTRICT' THEN 1"
      "  WHEN 'NO ACTION' THEN 3"
      "  WHEN 'CASCADE' THEN 0"
      "  WHEN 'SET NULL' THEN 2"
      "  WHEN 'SET DEFAULT' THEN 4"
      " END DELETE_RULE,"
      " RC.CONSTRAINT_NAME FK_NAME,"
      " RC.UNIQUE_CONSTRAINT_NAME PK_NAME,"
      + std::to_string(importedKeyNotDeferrable)
      +" DEFERRABILITY"
      " FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE KCU"
      " INNER JOIN INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS RC"
      " ON KCU.CONSTRAINT_SCHEMA = RC.CONSTRAINT_SCHEMA"
      " AND KCU.CONSTRAINT_NAME = RC.CONSTRAINT_NAME"
      " WHERE "
      +catalogCond("KCU.REFERENCED_TABLE_SCHEMA",parentSchema)
      +" AND "
      +catalogCond("KCU.TABLE_SCHEMA",foreignSchema)
      +" AND "
      " KCU.REFERENCED_TABLE_NAME = "
      +escapeQuote(parentTable)
      +" AND "
      " KCU.TABLE_NAME = "
      +escapeQuote(foreignTable)
      +" ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ";

    return executeQuery(sql);
  }

  /**
    * Retrieves a description of all the data types supported by this database. They are ordered by
    * DATA_TYPE and then by how closely the data type maps to the corresponding JDBC SQL type.
    *
    * <p>If the database supports SQL distinct types, then getTypeInfo() will return a single row
    * with a TYPE_NAME of DISTINCT and a DATA_TYPE of Types::DISTINCT. If the database supports SQL
    * structured types, then getTypeInfo() will return a single row with a TYPE_NAME of STRUCT and a
    * DATA_TYPE of Types::STRUCT.
    *
    * <p>If SQL distinct or structured types are supported, then information on the individual types
    * may be obtained from the getUDTs() method.
    *
    * <p>Each type description has the following columns:
    *
    * <OL>
    *   <LI><B>TYPE_NAME</B> String {@code= } Type name
    *   <LI><B>DATA_TYPE</B> int {@code= } SQL data type from java.sql.Types
    *   <LI><B>PRECISION</B> int {@code= } maximum precision
    *   <LI><B>LITERAL_PREFIX</B> String {@code= } prefix used to quote a literal (may be <code>null
    *       </code>)
    *   <LI><B>LITERAL_SUFFIX</B> String {@code= } suffix used to quote a literal (may be <code>null
    *       </code>)
    *   <LI><B>CREATE_PARAMS</B> String {@code= } parameters used in creating the type (may be
    *       <code>null</code>)
    *   <LI><B>NULLABLE</B> short {@code= } can you use NULL for this type.
    *       <UL>
    *         <LI>typeNoNulls - does not allow NULL values
    *         <LI>typeNullable - allows NULL values
    *         <LI>typeNullableUnknown - nullability unknown
    *       </UL>
    *   <LI><B>CASE_SENSITIVE</B> boolean{@code= } is it case sensitive.
    *   <LI><B>SEARCHABLE</B> short {@code= } can you use "WHERE" based on this type:
    *       <UL>
    *         <LI>typePredNone - No support
    *         <LI>typePredChar - Only supported with WHERE .. LIKE
    *         <LI>typePredBasic - Supported except for WHERE .. LIKE
    *         <LI>typeSearchable - Supported for all WHERE ..
    *       </UL>
    *   <LI><B>UNSIGNED_ATTRIBUTE</B> boolean {@code= } is it unsigned.
    *   <LI><B>FIXED_PREC_SCALE</B> boolean {@code= } can it be a money value.
    *   <LI><B>AUTO_INCREMENT</B> boolean {@code= } can it be used for an auto-increment value.
    *   <LI><B>LOCAL_TYPE_NAME</B> String {@code= } localized version of type name (may be <code>
    *       null</code>)
    *   <LI><B>MINIMUM_SCALE</B> short {@code= } minimum scale supported
    *   <LI><B>MAXIMUM_SCALE</B> short {@code= } maximum scale supported
    *   <LI><B>SQL_DATA_TYPE</B> int {@code= } unused
    *   <LI><B>SQL_DATETIME_SUB</B> int {@code= } unused
    *   <LI><B>NUM_PREC_RADIX</B> int {@code= } usually 2 or 10
    * </OL>
    *
    * <p>The PRECISION column represents the maximum column size that the server supports for the
    * given datatype. For numeric data, this is the maximum precision. For character data, this is
    * the length in characters. For datetime datatypes, this is the length in characters of the
    * String representation (assuming the maximum allowed precision of the fractional seconds
    * component). For binary data, this is the length in bytes. For the ROWID datatype, this is the
    * length in bytes. Null is returned for data types where the column size is not applicable.
    *
    * @return a <code>ResultSet</code> object in which each row is an SQL type description
    */
  ResultSet* MariaDbDatabaseMetaData::getTypeInfo()
  {
    static std::vector<SQLString> columnNames{
      "TYPE_NAME",
      "DATA_TYPE",
      "PRECISION",
      "LITERAL_PREFIX",
      "LITERAL_SUFFIX",     /*5*/
      "CREATE_PARAMS",
      "NULLABLE",
      "CASE_SENSITIVE",
      "SEARCHABLE",
      "UNSIGNED_ATTRIBUTE", /*10*/
      "FIXED_PREC_SCALE",
      "AUTO_INCREMENT",
      "LOCAL_TYPE_NAME",
      "MINIMUM_SCALE",
      "MAXIMUM_SCALE",      /*15*/
      "SQL_DATA_TYPE",
      "SQL_DATETIME_SUB",
      "NUM_PREC_RADIX"      /*18*/
    };
    std::vector<ColumnType> columnTypes{
      ColumnType::VARCHAR,
      ColumnType::INTEGER,
      ColumnType::INTEGER,
      ColumnType::VARCHAR,
      ColumnType::VARCHAR,
      ColumnType::VARCHAR,
      ColumnType::INTEGER,
      ColumnType::BIT,
      ColumnType::SMALLINT,
      ColumnType::BIT,
      ColumnType::BIT,
      ColumnType::BIT,
      ColumnType::VARCHAR,
      ColumnType::SMALLINT,
      ColumnType::SMALLINT,
      ColumnType::INTEGER,
      ColumnType::INTEGER,
      ColumnType::INTEGER
    };

    static std::vector<std::vector<sql::bytes>> data{
    {
      BYTES_INIT("BIT"),
      BYTES_INIT("-7"),
      BYTES_INIT("1"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("BIT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("BOOL"),
      BYTES_INIT("-7"),
      BYTES_INIT("1"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("BOOL"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("TINYINT"),
      BYTES_INIT("-6"),
      BYTES_INIT("3"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [UNSIGNED] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("TINYINT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("TINYINT UNSIGNED"),
      BYTES_INIT("-6"),
      BYTES_INIT("3"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [UNSIGNED] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("TINYINT UNSIGNED"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("BIGINT"),
      BYTES_INIT("-5"),
      BYTES_INIT("19"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [UNSIGNED] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("BIGINT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("BIGINT UNSIGNED"),
      BYTES_INIT("-5"),
      BYTES_INIT("20"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("BIGINT UNSIGNED"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("LONG VARBINARY"),
      BYTES_INIT("-4"),
      BYTES_INIT("16777215"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("LONG VARBINARY"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("MEDIUMBLOB"),
      BYTES_INIT("-4"),
      BYTES_INIT("16777215"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("MEDIUMBLOB"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("LONGBLOB"),
      BYTES_INIT("-4"),
      BYTES_INIT("2147483647"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("LONGBLOB"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("BLOB"),BYTES_INIT("-4"),BYTES_INIT("65535"),BYTES_INIT("'"),BYTES_INIT("'"),BYTES_INIT(""),BYTES_INIT("1"),BYTES_INIT("1"),BYTES_INIT("3"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("BLOB"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),
      BYTES_INIT("0"),BYTES_INIT("10")
    },{
      BYTES_INIT("TINYBLOB"),
      BYTES_INIT("-4"),
      BYTES_INIT("255"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("TINYBLOB"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("VARBINARY"),
      BYTES_INIT("-3"),
      BYTES_INIT("255"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT("(M)"),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("VARBINARY"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("BINARY"),
      BYTES_INIT("-2"),
      BYTES_INIT("255"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT("(M)"),
      BYTES_INIT("1"),
      BYTES_INIT("1"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("BINARY"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("LONG VARCHAR"),
      BYTES_INIT("-1"),
      BYTES_INIT("16777215"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("LONG VARCHAR"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("MEDIUMTEXT"),
      BYTES_INIT("-1"),
      BYTES_INIT("16777215"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("MEDIUMTEXT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("LONGTEXT"),
      BYTES_INIT("-1"),
      BYTES_INIT("2147483647"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("LONGTEXT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("TEXT"),
      BYTES_INIT("-1"),
      BYTES_INIT("65535"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("TEXT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("TINYTEXT"),
      BYTES_INIT("-1"),
      BYTES_INIT("255"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT(""),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("TINYTEXT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("CHAR"),
      BYTES_INIT("1"),
      BYTES_INIT("255"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT("(M)"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("CHAR"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("NUMERIC"),
      BYTES_INIT("2"),
      BYTES_INIT("65"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M,D])] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("NUMERIC"),
      BYTES_INIT("-308"),
      BYTES_INIT("308"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("DECIMAL"),
      BYTES_INIT("3"),
      BYTES_INIT("65"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M,D])] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("DECIMAL"),
      BYTES_INIT("-308"),
      BYTES_INIT("308"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("INTEGER"),
      BYTES_INIT("4"),
      BYTES_INIT("10"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [UNSIGNED] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("INTEGER"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("INTEGER UNSIGNED"),
      BYTES_INIT("4"),
      BYTES_INIT("10"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("INTEGER UNSIGNED"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("INT"),
      BYTES_INIT("4"),
      BYTES_INIT("10"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [UNSIGNED] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("INT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("INT UNSIGNED"),
      BYTES_INIT("4"),
      BYTES_INIT("10"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("INT UNSIGNED"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("MEDIUMINT"),
      BYTES_INIT("4"),
      BYTES_INIT("7"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [UNSIGNED] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("MEDIUMINT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("MEDIUMINT UNSIGNED"),
      BYTES_INIT("4"),
      BYTES_INIT("8"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("MEDIUMINT UNSIGNED"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("SMALLINT"),
      BYTES_INIT("5"),
      BYTES_INIT("5"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [UNSIGNED] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("SMALLINT"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("SMALLINT UNSIGNED"),
      BYTES_INIT("5"),
      BYTES_INIT("5"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("SMALLINT UNSIGNED"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("FLOAT"),
      BYTES_INIT("7"),
      BYTES_INIT("10"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M|D)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("FLOAT"),
      BYTES_INIT("-38"),
      BYTES_INIT("38"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("DOUBLE"),
      BYTES_INIT("8"),
      BYTES_INIT("17"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M|D)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("DOUBLE"),
      BYTES_INIT("-308"),
      BYTES_INIT("308"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("DOUBLE PRECISION"),
      BYTES_INIT("8"),
      BYTES_INIT("17"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M,D)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("DOUBLE PRECISION"),
      BYTES_INIT("-308"),
      BYTES_INIT("308"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("REAL"),
      BYTES_INIT("8"),
      BYTES_INIT("17"),
      BYTES_INIT(""),
      BYTES_INIT(""),
      BYTES_INIT("[(M,D)] [ZEROFILL]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("1"),
      BYTES_INIT("REAL"),
      BYTES_INIT("-308"),
      BYTES_INIT("308"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("VARCHAR"),BYTES_INIT("12"),BYTES_INIT("255"),BYTES_INIT("'"),BYTES_INIT("'"),BYTES_INIT("(M)"),BYTES_INIT("1"),
      BYTES_INIT("0"),BYTES_INIT("3"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("VARCHAR"),BYTES_INIT("0"),BYTES_INIT("0"),
      BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("10")
    },{
      BYTES_INIT("ENUM"),BYTES_INIT("12"),BYTES_INIT("65535"),BYTES_INIT("'"),BYTES_INIT("'"),BYTES_INIT(""),BYTES_INIT("1"),BYTES_INIT("0"),
      BYTES_INIT("3"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("ENUM"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),
      BYTES_INIT("0"),BYTES_INIT("10")
    },{
      BYTES_INIT("SET"),BYTES_INIT("12"),BYTES_INIT("64"),BYTES_INIT("'"),BYTES_INIT("'"),BYTES_INIT(""),BYTES_INIT("1"),BYTES_INIT("0"),
      BYTES_INIT("3"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("SET"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("DATE"),BYTES_INIT("91"),BYTES_INIT("10"),BYTES_INIT("'"),BYTES_INIT("'"),BYTES_INIT(""),BYTES_INIT("1"),BYTES_INIT("0"),
      BYTES_INIT("3"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("DATE"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("TIME"),BYTES_INIT("92"),BYTES_INIT("18"),BYTES_INIT("'"),BYTES_INIT("'"),BYTES_INIT("[(M)]"),BYTES_INIT("1"),
      BYTES_INIT("0"),BYTES_INIT("3"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("TIME"),BYTES_INIT("0"),BYTES_INIT("0"),BYTES_INIT("0"),
      BYTES_INIT("0"),BYTES_INIT("10")
    },{
      BYTES_INIT("DATETIME"),
      BYTES_INIT("93"),
      BYTES_INIT("27"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT("[(M)]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("DATETIME"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
    },{
      BYTES_INIT("TIMESTAMP"),
      BYTES_INIT("93"),
      BYTES_INIT("27"),
      BYTES_INIT("'"),
      BYTES_INIT("'"),
      BYTES_INIT("[(M)]"),
      BYTES_INIT("1"),
      BYTES_INIT("0"),
      BYTES_INIT("3"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("TIMESTAMP"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("0"),
      BYTES_INIT("10")
      }
    };

    return SelectResultSet::createResultSet(columnNames, columnTypes, data, connection->getProtocol().get());
  }

  /**
   * Retrieves a description of the given table's indices and statistics. They are ordered by
   * NON_UNIQUE, TYPE, INDEX_NAME, and ORDINAL_POSITION.
   *
   * <p>Each index column description has the following columns:
   *
   * <ol>
   *   <li><B>TABLE_CAT</B> String {@code= } table catalog (may be <code>null</code>)
   *   <li><B>TABLE_SCHEM</B> String {@code= } table schema (may be <code>null</code>)
   *   <li><B>TABLE_NAME</B> String {@code= } table name
   *   <li><B>NON_UNIQUE</B> boolean {@code= } Can index values be non-unique. false when TYPE is
   *       tableIndexStatistic
   *   <li><B>INDEX_QUALIFIER</B> String {@code= } index catalog (may be <code>null</code>); <code>
   *       null</code> when TYPE is tableIndexStatistic
   *   <li><B>INDEX_NAME</B> String {@code= } index name; <code>null</code> when TYPE is
   *       tableIndexStatistic
   *   <li><B>TYPE</B> short {@code= } index type:
   *       <ul>
   *         <li>tableIndexStatistic - this identifies table statistics that are returned in
   *             conjuction with a table's index descriptions
   *         <li>tableIndexClustered - this is a clustered index
   *         <li>tableIndexHashed - this is a hashed index
   *         <li>tableIndexOther - this is some other style of index
   *       </ul>
   *   <li><B>ORDINAL_POSITION</B> short {@code= } column sequence number within index; zero when
   *       TYPE is tableIndexStatistic
   *   <li><B>COLUMN_NAME</B> String {@code= } column name; <code>null</code> when TYPE is
   *       tableIndexStatistic
   *   <li><B>ASC_OR_DESC</B> String {@code= } column sort sequence, "A" {@code= } ascending, "D"
   *       {@code= } descending, may be <code>null</code> if sort sequence is not supported; <code>
   *       null</code> when TYPE is tableIndexStatistic
   *   <li><B>CARDINALITY</B> long {@code= } When TYPE is tableIndexStatistic, then this is the
   *       number of rows in the table; otherwise, it is the number of unique values in the index.
   *   <li><B>PAGES</B> long {@code= } When TYPE is tableIndexStatisic then this is the number of
   *       pages used for the table, otherwise it is the number of pages used for the current index.
   *   <li><B>FILTER_CONDITION</B> String {@code= } Filter condition, if any. (may be <code>null
   *       </code>)
   * </ol>
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in this database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schema a schema name; must match the schema name as it is stored in this database; ""
   *     retrieves those without a schema; <code>null</code> means that the schema name should not
   *     be used to narrow the search
   * @param table a table name; must match the table name as it is stored in this database
   * @param unique when true, return only indices for unique values; when false, return indices
   *     regardless of whether unique or not
   * @param approximate when true, result is allowed to reflect approximate or out of data values;
   *     when false, results are requested to be accurate
   * @return <code>ResultSet</code> - each row is an index column description
   * @throws SQLException if a database access error occurs
   */
  ResultSet* MariaDbDatabaseMetaData::getIndexInfo(
      const SQLString& catalog, const SQLString& schema, const SQLString& table, bool unique, bool approximate)
  {


    SQLString sql =
      "SELECT NULL TABLE_CAT, TABLE_SCHEMA TABLE_SCHEM, TABLE_NAME, NON_UNIQUE, "
      " TABLE_SCHEMA INDEX_QUALIFIER, INDEX_NAME, " + std::to_string(DatabaseMetaData::tableIndexOther) + " TYPE,"
      " SEQ_IN_INDEX ORDINAL_POSITION, COLUMN_NAME, COLLATION ASC_OR_DESC,"
      " CARDINALITY, NULL PAGES, NULL FILTER_CONDITION"
      " FROM INFORMATION_SCHEMA.STATISTICS"
      " WHERE TABLE_NAME = "
      +escapeQuote(table)
      +" AND "
      +catalogCond("TABLE_SCHEMA", schema)
      +((unique)?" AND NON_UNIQUE = 0":"")
      +" ORDER BY NON_UNIQUE, TYPE, INDEX_NAME, ORDINAL_POSITION";

    return executeQuery(sql);
  }

  /**
   * Retrieves whether this database supports the given result set type. ResultSet.TYPE_FORWARD_ONLY
   * and ResultSet.TYPE_SCROLL_INSENSITIVE are supported.
   *
   * @param type one of the following <code>ResultSet</code> constants:
   *     <ul>
   *       <li><code>ResultSet.TYPE_FORWARD_ONLY</code>
   *       <li><code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>
   *       <li><code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
   *     </ul>
   *
   * @return true if supported
   */
  bool MariaDbDatabaseMetaData::supportsResultSetType(int32_t type) {
    return (type == ResultSet::TYPE_SCROLL_INSENSITIVE || type == ResultSet::TYPE_FORWARD_ONLY);
  }

  /**
   * Retrieves whether this database supports the given concurrency type in combination with the
   * given result set type. All are supported, but combination that use
   * ResultSet.TYPE_SCROLL_INSENSITIVE.
   *
   * @param type one of the following <code>ResultSet</code> constants:
   *     <ul>
   *       <li><code>ResultSet.TYPE_FORWARD_ONLY</code>
   *       <li><code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>
   *       <li><code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
   *     </ul>
   *
   * @param concurrency one of the following <code>ResultSet</code> constants:
   *     <ul>
   *       <li><code>ResultSet.CONCUR_READ_ONLY</code>
   *       <li><code>ResultSet.CONCUR_UPDATABLE</code>
   *     </ul>
   *
   * @return true if supported
   */
  bool MariaDbDatabaseMetaData::supportsResultSetConcurrency(int32_t type,int32_t concurrency){

    return type == ResultSet::TYPE_SCROLL_INSENSITIVE || type== ResultSet::TYPE_FORWARD_ONLY;
  }

  bool MariaDbDatabaseMetaData::ownUpdatesAreVisible(int32_t type){
    return supportsResultSetType(type);
  }

  bool MariaDbDatabaseMetaData::ownDeletesAreVisible(int32_t type){
    return supportsResultSetType(type);
  }

  bool MariaDbDatabaseMetaData::ownInsertsAreVisible(int32_t type){
    return supportsResultSetType(type);
  }

  bool MariaDbDatabaseMetaData::othersUpdatesAreVisible(int32_t type){
    return false;
  }

  bool MariaDbDatabaseMetaData::othersDeletesAreVisible(int32_t type){
    return false;
  }

  bool MariaDbDatabaseMetaData::othersInsertsAreVisible(int32_t type){
    return false;
  }

  bool MariaDbDatabaseMetaData::updatesAreDetected(int32_t type){
    return false;
  }

  bool MariaDbDatabaseMetaData::deletesAreDetected(int32_t type){
    return false;
  }

  bool MariaDbDatabaseMetaData::insertsAreDetected(int32_t type){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsBatchUpdates(){
    return true;
  }

  ResultSet* MariaDbDatabaseMetaData::getUDTs(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& typeNamePattern, std::list<int32_t>& types)  {
    SQLString sql =
      "SELECT ' ' TYPE_CAT, NULL TYPE_SCHEM, ' ' TYPE_NAME, ' ' CLASS_NAME, 0 DATA_TYPE, ' ' REMARKS, 0 BASE_TYPE"
      " FROM DUAL WHERE 1=0";

    return executeQuery(sql);
  }

  Connection* MariaDbDatabaseMetaData::getConnection(){
    return connection;
  }

  bool MariaDbDatabaseMetaData::supportsSavepoints(){
    return true;
  }

  bool MariaDbDatabaseMetaData::supportsNamedParameters(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsMultipleOpenResults(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsGetGeneratedKeys(){
    return true;
  }

  /**
   * Retrieves a description of the user-defined type (UDT) hierarchies defined in a particular
   * schema in this database. Only the immediate super type/ sub type relationship is modeled. Only
   * supertype information for UDTs matching the catalog, schema, and type name is returned. The
   * type name parameter may be a fully-qualified name. When the UDT name supplied is a
   * fully-qualified name, the catalog and schemaPattern parameters are ignored. If a UDT does not
   * have a direct super type, it is not listed here. A row of the <code>ResultSet</code> object
   * returned by this method describes the designated UDT and a direct supertype. A row has the
   * following columns:
   *
   * <OL>
   *   <li><B>TYPE_CAT</B> String {@code= } the UDT's catalog (may be <code>null</code>)
   *   <li><B>TYPE_SCHEM</B> String {@code= } UDT's schema (may be <code>null</code>)
   *   <li><B>TYPE_NAME</B> String {@code= } type name of the UDT
   *   <li><B>SUPERTYPE_CAT</B> String {@code= } the direct super type's catalog (may be <code>null
   *       </code>)
   *   <li><B>SUPERTYPE_SCHEM</B> String {@code= } the direct super type's schema (may be <code>
   *       null</code>)
   *   <li><B>SUPERTYPE_NAME</B> String {@code= } the direct super type's name
   * </OL>
   *
   * <p><B>Note:</B> If the driver does not support type hierarchies, an empty result set is
   * returned.
   *
   * @param catalog a catalog name; "" retrieves those without a catalog; <code>null</code> means
   *     drop catalog name from the selection criteria
   * @param schemaPattern a schema name pattern; "" retrieves those without a schema
   * @param typeNamePattern a UDT name pattern; may be a fully-qualified name
   * @return a <code>ResultSet</code> object in which a row gives information about the designated
   *     UDT
   * @throws SQLException if a database access error occurs
   * @see #getSearchStringEscape
   * @since 1.4
   */
  ResultSet* MariaDbDatabaseMetaData::getSuperTypes(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& typeNamePattern)  {
    SQLString sql =
      "SELECT  ' ' TYPE_CAT, NULL TYPE_SCHEM, ' ' TYPE_NAME, ' ' SUPERTYPE_CAT, ' ' SUPERTYPE_SCHEM, ' '  SUPERTYPE_NAME"
      " FROM DUAL WHERE 1=0";

    return executeQuery(sql);
  }

  /**
   * Retrieves a description of the table hierarchies defined in a particular schema in this
   * database.
   *
   * <p>Only supertable information for tables matching the catalog, schema and table name are
   * returned. The table name parameter may be a fully-qualified name, in which case, the catalog
   * and schemaPattern parameters are ignored. If a table does not have a super table, it is not
   * listed here. Supertables have to be defined in the same catalog and schema as the sub tables.
   * Therefore, the type description does not need to include this information for the supertable.
   *
   * <p>Each type description has the following columns:
   *
   * <OL>
   *   <li><B>TABLE_CAT</B> String {@code= } the type's catalog (may be <code>null</code>)
   *   <li><B>TABLE_SCHEM</B> String {@code= } type's schema (may be <code>null</code>)
   *   <li><B>TABLE_NAME</B> String {@code= } type name
   *   <li><B>SUPERTABLE_NAME</B> String {@code= } the direct super type's name
   * </OL>
   *
   * <p><B>Note:</B> If the driver does not support type hierarchies, an empty result set is
   * returned.
   *
   * @param catalog a catalog name; "" retrieves those without a catalog; <code>null</code> means
   *     drop catalog name from the selection criteria
   * @param schemaPattern a schema name pattern; "" retrieves those without a schema
   * @param tableNamePattern a table name pattern; may be a fully-qualified name
   * @return a <code>ResultSet</code> object in which each row is a type description
   * @throws SQLException if a database access error occurs
   * @see #getSearchStringEscape
   * @since 1.4
   */
  ResultSet* MariaDbDatabaseMetaData::getSuperTables(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern)  {
    SQLString sql =
      "SELECT  ' ' TABLE_CAT, ' ' TABLE_SCHEM, ' ' TABLE_NAME, ' ' SUPERTABLE_NAME FROM DUAL WHERE 1=0";
    return executeQuery(sql);
  }

  /**
   * Retrieves a description of the given attribute of the given type for a user-defined type (UDT)
   * that is available in the given schema and catalog. Descriptions are returned only for
   * attributes of UDTs matching the catalog, schema, type, and attribute name criteria. They are
   * ordered by <code>TYPE_CAT</code>, <code>TYPE_SCHEM</code>, <code>TYPE_NAME</code> and <code>
   * ORDINAL_POSITION</code>. This description does not contain inherited attributes. The <code>
   * ResultSet</code> object that is returned has the following columns:
   *
   * <OL>
   *   <li><B>TYPE_CAT</B> String {@code= } type catalog (may be <code>null</code>)
   *   <li><B>TYPE_SCHEM</B> String {@code= } type schema (may be <code>null</code>)
   *   <li><B>TYPE_NAME</B> String {@code= } type name
   *   <li><B>ATTR_NAME</B> String {@code= } attribute name
   *   <li><B>DATA_TYPE</B> int {@code= } attribute type SQL type from java.sql.Types
   *   <li><B>ATTR_TYPE_NAME</B> String {@code= } Data source dependent type name. For a UDT, the
   *       type name is fully qualified. For a REF, the type name is fully qualified and represents
   *       the target type of the reference type.
   *   <li><B>ATTR_SIZE</B> int {@code= } column size. For char or date types this is the maximum
   *       number of characters; for numeric or decimal types this is precision.
   *   <li><B>DECIMAL_DIGITS</B> int {@code= } the number of fractional digits. Null is returned
   *       for data types where DECIMAL_DIGITS is not applicable.
   *   <li><B>NUM_PREC_RADIX</B> int {@code= } Radix (typically either 10 or 2)
   *   <li><B>NULLABLE</B> int {@code= } whether NULL is allowed
   *       <UL>
   *         <li>attributeNoNulls - might not allow NULL values
   *         <li>attributeNullable - definitely allows NULL values
   *         <li>attributeNullableUnknown - nullability unknown
   *       </UL>
   *   <li><B>REMARKS</B> String {@code= } comment describing column (may be <code>null</code>)
   *   <li><B>ATTR_DEF</B> String {@code= } default value (may be<code>null</code>)
   *   <li><B>SQL_DATA_TYPE</B> int {@code= } unused
   *   <li><B>SQL_DATETIME_SUB</B> int {@code= } unused
   *   <li><B>CHAR_OCTET_LENGTH</B> int {@code= } for char types the maximum number of bytes in the
   *       column
   *   <li><B>ORDINAL_POSITION</B> int {@code= } index of the attribute in the UDT (starting at 1)
   *   <li><B>IS_NULLABLE</B> String {@code= } ISO rules are used to determine the nullability for
   *       a attribute.
   *       <UL>
   *         <li>YES --- if the attribute can include NULLs
   *         <li>NO --- if the attribute cannot include NULLs
   *         <li>empty string --- if the nullability for the attribute is unknown
   *       </UL>
   *   <li><B>SCOPE_CATALOG</B> String {@code= } catalog of table that is the scope of a reference
   *       attribute (<code>null</code> if DATA_TYPE isn't REF)
   *   <li><B>SCOPE_SCHEMA</B> String {@code= } schema of table that is the scope of a reference
   *       attribute (<code>null</code> if DATA_TYPE isn't REF)
   *   <li><B>SCOPE_TABLE</B> String {@code= } table name that is the scope of a reference
   *       attribute (<code>null</code> if the DATA_TYPE isn't REF)
   *   <li><B>SOURCE_DATA_TYPE</B> short {@code= } source type of a distinct type or user-generated
   *       Ref type,SQL type from java.sql.Types (<code>null</code> if DATA_TYPE isn't DISTINCT or
   *       user-generated REF)
   * </OL>
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
   *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
   *     should not be used to narrow the search
   * @param typeNamePattern a type name pattern; must match the type name as it is stored in the
   *     database
   * @param attributeNamePattern an attribute name pattern; must match the attribute name as it is
   *     declared in the database
   * @return a <code>ResultSet</code> object in which each row is an attribute description
   * @throws SQLException if a database access error occurs
   * @see #getSearchStringEscape
   * @since 1.4
   */
  ResultSet* MariaDbDatabaseMetaData::getAttributes(
      const SQLString& catalog, const SQLString& schemaPattern, const SQLString& typeNamePattern, const SQLString& attributeNamePattern)
  {


    SQLString sql =
      "SELECT ' ' TYPE_CAT, ' ' TYPE_SCHEM, ' ' TYPE_NAME, ' ' ATTR_NAME, 0 DATA_TYPE,"
      " ' ' ATTR_TYPE_NAME, 0 ATTR_SIZE, 0 DECIMAL_DIGITS, 0 NUM_PREC_RADIX, 0 NULLABLE,"
      " ' ' REMARKS, ' ' ATTR_DEF,  0 SQL_DATA_TYPE, 0 SQL_DATETIME_SUB, 0 CHAR_OCTET_LENGTH,"
      " 0 ORDINAL_POSITION, ' ' IS_NULLABLE, ' ' SCOPE_CATALOG, ' ' SCOPE_SCHEMA, ' ' SCOPE_TABLE,"
      " 0 SOURCE_DATA_TYPE"
      " FROM DUAL "
      " WHERE 1=0";

    return executeQuery(sql);
  }

  bool MariaDbDatabaseMetaData::supportsResultSetHoldability(int32_t holdability){
    return holdability == ResultSet::HOLD_CURSORS_OVER_COMMIT;
  }


  int32_t MariaDbDatabaseMetaData::getResultSetHoldability(){
    return ResultSet::HOLD_CURSORS_OVER_COMMIT;
  }

  uint32_t MariaDbDatabaseMetaData::getDatabaseMajorVersion(){
    return connection->getProtocol()->getMajorServerVersion();
  }

  uint32_t MariaDbDatabaseMetaData::getDatabaseMinorVersion(){
    return connection->getProtocol()->getMinorServerVersion();
  }

  uint32_t MariaDbDatabaseMetaData::getDatabasePatchVersion(){
    return connection->getProtocol()->getPatchServerVersion();
  }


  uint32_t MariaDbDatabaseMetaData::getJDBCMajorVersion(){
    return 4;
  }

  uint32_t MariaDbDatabaseMetaData::getJDBCMinorVersion(){
    return 2;
  }

  int32_t MariaDbDatabaseMetaData::getSQLStateType(){
    return sqlStateSQL;
  }

  bool MariaDbDatabaseMetaData::locatorsUpdateCopy(){
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsStatementPooling(){
    return false;
  }

  RowIdLifetime MariaDbDatabaseMetaData::getRowIdLifetime(){
    return RowIdLifetime::ROWID_UNSUPPORTED;
  }

  bool MariaDbDatabaseMetaData::supportsStoredFunctionsUsingCallSyntax(){
    return true;
  }

  bool MariaDbDatabaseMetaData::autoCommitFailureClosesAllResultSets(){
    return false;
  }

  /**
   * Retrieves a list of the client info properties that the driver supports. The result set
   * contains the following columns
   *
   * <ol>
   *   <li>NAME String : The name of the client info property
   *   <li>MAX_LEN int : The maximum length of the value for the property
   *   <li>DEFAULT_VALUE String : The default value of the property
   *   <li>DESCRIPTION String : A description of the property. This will typically contain
   *       information as to where this property is stored in the database.
   * </ol>
   *
   * <p>The ResultSet is sorted by the NAME column
   *
   * @return A ResultSet object; each row is a supported client info property
   */
  ResultSet* MariaDbDatabaseMetaData::getClientInfoProperties()
  {
    static std::vector<SQLString> columnNames{ "NAME", "MAX_LEN", "DEFAULT_VALUE", "DESCRIPTION" };
    std::vector<ColumnType> columnTypes{
      ColumnType::STRING,
      ColumnType::INTEGER,
      ColumnType::STRING,
      ColumnType::STRING };
    /*std::vector<ColumnDefinition> columns{
      ColumnDefinition::create("NAME",ColumnType::STRING),
      ColumnDefinition::create("MAX_LEN",ColumnType::INTEGER),
      ColumnDefinition::create("DEFAULT_VALUE",ColumnType::STRING),
      ColumnDefinition::create("DESCRIPTION",ColumnType::STRING),
    };
    ColumnType types[]{
        ColumnType::STRING, ColumnType::INTEGER, ColumnType::STRING, ColumnType::STRING
      };*/
    const char* sixteenMb= "16777215";// { 49, 54, 55, 55, 55, 50, 49, 53, 0};

    std::vector<std::vector<sql::bytes>> rows{
      {
        BYTES_INIT("ApplicationName"),
        BYTES_INIT(sixteenMb),
        BYTES_STR_INIT(emptyStr),
        BYTES_INIT("The name of the application currently utilizing the connection")
      },
      {
        BYTES_INIT("ClientUser"),
        BYTES_INIT(sixteenMb),
        BYTES_STR_INIT(emptyStr),
        BYTES_INIT("The name of the user that the application using the connection is performing work for. "
        "This may not be the same as the user name that was used in establishing the connection->")
      },
      {
        BYTES_INIT("ClientHostname"),
        BYTES_INIT(sixteenMb),
        BYTES_STR_INIT(emptyStr),
        BYTES_INIT("The hostname of the computer the application using the connection is running on")
      }
    };

    /*rows.reserve(3);
    rows.push_back(
        StandardPacketInputStream::create(
          {
          "ApplicationName",
          sixteenMb,
          empty,
          "The name of the application currently utilizing the connection"
          },
          types));
    rows.push_back(
      StandardPacketInputStream::create(
          {
          "ClientUser",
          sixteenMb,
          empty,
          "The name of the user that the application using the connection is performing work for. "
          "This may not be the same as the user name that was used in establishing the connection->"
          },
          types));
    rows.push_back(
        StandardPacketInputStream::create(
          {
          "ClientHostname",
          sixteenMb,
          empty,
          "The hostname of the computer the application using the connection is running on"
          },
          types));*/
    return SelectResultSet::createResultSet(columnNames, columnTypes, rows, connection->getProtocol().get());
    /*return new SelectResultSet(
        columns, rows, connection->getProtocol(), ResultSet::TYPE_SCROLL_INSENSITIVE);*/
  }

  /**
   * Retrieves a description of the system and user functions available in the given catalog. Only
   * system and user function descriptions matching the schema and function name criteria are
   * returned. They are ordered by <code>FUNCTION_CAT</code>, <code>FUNCTION_SCHEM</code>, <code>
   * FUNCTION_NAME</code> and <code>SPECIFIC_ NAME</code>.
   *
   * <p>Each function description has the the following columns:
   *
   * <OL>
   *   <li><B>FUNCTION_CAT</B> String {@code= } function catalog (may be <code>null</code>)
   *   <li><B>FUNCTION_SCHEM</B> String {@code= } function schema (may be <code>null</code>)
   *   <li><B>FUNCTION_NAME</B> String {@code= } function name. This is the name used to invoke the
   *       function
   *   <li><B>REMARKS</B> String {@code= } explanatory comment on the function
   *   <li><B>FUNCTION_TYPE</B> short {@code= } kind of function:
   *       <UL>
   *         <li>functionResultUnknown - Cannot determine if a return value or table will be
   *             returned
   *         <li>functionNoTable- Does not return a table
   *         <li>functionReturnsTable - Returns a table
   *       </UL>
   *   <li><B>SPECIFIC_NAME</B> String {@code= } the name which uniquely identifies this function
   *       within its schema. This is a user specified, or DBMS generated, name that may be
   *       different then the <code>FUNCTION_NAME</code> for example with overload functions
   * </OL>
   *
   * <p>A user may not have permission to execute any of the functions that are returned by <code>
   * getFunctions</code>
   *
   * @param catalog a catalog name; must match the catalog name as it is stored in the database; ""
   *     retrieves those without a catalog; <code>null</code> means that the catalog name should not
   *     be used to narrow the search
   * @param schemaPattern a schema name pattern; must match the schema name as it is stored in the
   *     database; "" retrieves those without a schema; <code>null</code> means that the schema name
   *     should not be used to narrow the search
   * @param functionNamePattern a function name pattern; must match the function name as it is
   *     stored in the database
   * @return <code>ResultSet</code> - each row is a function description
   * @throws SQLException if a database access error occurs
   * @see #getSearchStringEscape
   * @since 1.6
   */
  ResultSet* MariaDbDatabaseMetaData::getFunctions(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& functionNamePattern)  {
    SQLString sql(
      "SELECT ROUTINE_SCHEMA FUNCTION_CAT,NULL FUNCTION_SCHEM, ROUTINE_NAME FUNCTION_NAME,"
      " ROUTINE_COMMENT REMARKS,"
      + std::to_string(functionNoTable)
      + " FUNCTION_TYPE, SPECIFIC_NAME "
      " FROM INFORMATION_SCHEMA.ROUTINES "
      " WHERE "
      +catalogCond("ROUTINE_SCHEMA", catalog)
      +" AND "
      +patternCond("ROUTINE_NAME", functionNamePattern)
      +" AND ROUTINE_TYPE='FUNCTION'");

    return executeQuery(sql);
  }

  int64_t MariaDbDatabaseMetaData::getMaxLogicalLobSize(){
    return 4294967295L;
  }

  bool MariaDbDatabaseMetaData::supportsRefCursors() {
    return false;
  }

  bool MariaDbDatabaseMetaData::supportsTypeConversion() {
    return true;
  }

  /* Group of not supported methods */
  ResultSet* MariaDbDatabaseMetaData::getSchemaObjectTypes() {
    throw SQLFeatureNotImplementedException("getSchemaObjectTypes is not implemented");
  }

  ResultSet* MariaDbDatabaseMetaData::getSchemaObjects(const SQLString& c, const SQLString& s, const SQLString& t) {
    throw SQLFeatureNotImplementedException("getSchemaObjects is not implemented");
  }

  ResultSet* MariaDbDatabaseMetaData::getSchemaObjects() {
    throw SQLFeatureNotImplementedException("getSchemaObjects is not implemented");
  }
  uint32_t MariaDbDatabaseMetaData::getCDBCMajorVersion() {
    throw SQLFeatureNotSupportedException("getCDBCMajorVersion is not supported");
  }
  uint32_t MariaDbDatabaseMetaData::getCDBCMinorVersion() {
    throw SQLFeatureNotSupportedException("getCDBCMinorVersion is not supported");
  }
  ResultSet* MariaDbDatabaseMetaData::getSchemaCollation(const SQLString& c, const SQLString& s) {
    throw SQLFeatureNotImplementedException("getSchemaCollation is not implemented");
  }
  ResultSet* MariaDbDatabaseMetaData::getSchemaCharset(const SQLString& c, const SQLString& s) {
    throw SQLFeatureNotImplementedException("getSchemaCharset is not implemented");
  }
  ResultSet* MariaDbDatabaseMetaData::getTableCollation(const SQLString& c, const SQLString& s, const SQLString& t) {
    throw SQLFeatureNotImplementedException("getTableCollation is not implemented");
  }
  ResultSet* MariaDbDatabaseMetaData::getTableCharset(const SQLString& c, const SQLString& s, const SQLString& t) {
    throw SQLFeatureNotImplementedException("getTableCharset is not implemented");

  }

} /* namespace mariadb */
} /* namespace sql     */
