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


#ifndef _MARIADBDATABASEMETADATA_H_
#define _MARIADBDATABASEMETADATA_H_

#include "MariaDbConnection.h"
#include "Identifier.h"
#include "Consts.h"

namespace sql
{
namespace mariadb
{

class MariaDbDatabaseMetaData  : public DatabaseMetaData
{

public:

  static const SQLString DRIVER_NAME ; /*"MariaDB Connector/C++"*/

private:

  MariaDbConnection* connection;
  const UrlParser urlParser;
  bool datePrecisionColumnExist ; /*true*/

public:

  MariaDbDatabaseMetaData(Connection* connection, const UrlParser& urlParser);

private:

  static SQLString columnTypeClause(Shared::Options& options);
  static std::size_t skipWhite(const SQLString& part, std::size_t startPos);
  static std::size_t parseIdentifier(const SQLString& part, std::size_t startPos, Identifier &identifier);
  static std::size_t parseIdentifierList(const SQLString& part, std::size_t startPos, std::vector<Identifier>& list);
  static std::size_t skipKeyword(const SQLString& part, std::size_t startPos, const SQLString& keyword);
  static int32_t getImportedKeyAction(const std::string& actionKey);
  static ResultSet* getImportedKeys(const SQLString& tableDef, const SQLString& tableName, const SQLString& catalog,
                                   MariaDbConnection* connection);

public:
  ResultSet* getImportedKeys(const SQLString& catalog, const SQLString& schema, const SQLString& table);

private:
  SQLString dataTypeClause(const SQLString& fullTypeColumnName);
  ResultSet* executeQuery(const SQLString& sql);
  SQLString escapeQuote(const SQLString& value);
  SQLString catalogCond(const SQLString& columnName, const SQLString& catalog);
  SQLString patternCond(const SQLString& columnName, const SQLString& tableName);


public:

  ResultSet* getPrimaryKeys(const SQLString& catalog, const SQLString& schema, const SQLString& table);

  ResultSet* getTables(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern, std::list<SQLString>& types);

  ResultSet* getColumns(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern,
                       const SQLString& columnNamePattern);
  ResultSet* getExportedKeys(const SQLString& catalog, const SQLString& schema, const SQLString& table);
  ResultSet* getImportedKeysUsingInformationSchema(const SQLString& catalog, const SQLString& table);
  ResultSet* getImportedKeysUsingShowCreateTable(const SQLString& catalog, const SQLString& table);
  ResultSet* getBestRowIdentifier(const SQLString& catalog, const SQLString& schema, const SQLString& table, int32_t scope, bool nullable);
  bool generatedKeyAlwaysReturned();
  ResultSet* getPseudoColumns(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern,
                            const SQLString& columnNamePattern);
  bool allProceduresAreCallable();
  bool allTablesAreSelectable();
  SQLString getURL();
  SQLString getUserName();
  bool isReadOnly();
  bool nullsAreSortedHigh();
  bool nullsAreSortedLow();
  bool nullsAreSortedAtStart();
  bool nullsAreSortedAtEnd();
  SQLString getDatabaseProductName();
  SQLString getDatabaseProductVersion();
  SQLString getDriverName();
  SQLString getDriverVersion();
  int32_t getDriverMajorVersion();
  int32_t getDriverMinorVersion();
  int32_t getDriverPatchVersion();
  bool usesLocalFiles();
  bool usesLocalFilePerTable();
  bool supportsMixedCaseIdentifiers();
  bool storesUpperCaseIdentifiers();
  bool storesLowerCaseIdentifiers();
  bool storesMixedCaseIdentifiers();
  bool supportsMixedCaseQuotedIdentifiers();
  bool storesUpperCaseQuotedIdentifiers();
  bool storesLowerCaseQuotedIdentifiers();
  bool storesMixedCaseQuotedIdentifiers();
  SQLString getIdentifierQuoteString();
  SQLString getSQLKeywords();
  SQLString getNumericFunctions();
  SQLString getStringFunctions();
  SQLString getSystemFunctions();
  SQLString getTimeDateFunctions();
  SQLString getSearchStringEscape();
  SQLString getExtraNameCharacters();
  bool supportsAlterTableWithAddColumn();
  bool supportsAlterTableWithDropColumn();
  bool supportsColumnAliasing();
  bool nullPlusNonNullIsNull();
  bool supportsConvert();
  bool supportsConvert(int32_t fromType,int32_t toType);
  bool supportsTableCorrelationNames();
  bool supportsDifferentTableCorrelationNames();
  bool supportsExpressionsInOrderBy();
  bool supportsOrderByUnrelated();
  bool supportsGroupBy();
  bool supportsGroupByUnrelated();
  bool supportsGroupByBeyondSelect();
  bool supportsLikeEscapeClause();
  bool supportsMultipleResultSets();
  bool supportsMultipleTransactions();
  bool supportsNonNullableColumns();
  bool supportsMinimumSQLGrammar();
  bool supportsCoreSQLGrammar();
  bool supportsExtendedSQLGrammar();
  bool supportsANSI92EntryLevelSQL();
  bool supportsANSI92IntermediateSQL();
  bool supportsANSI92FullSQL();
  bool supportsIntegrityEnhancementFacility();
  bool supportsOuterJoins();
  bool supportsFullOuterJoins();
  bool supportsLimitedOuterJoins();
  SQLString getSchemaTerm();
  SQLString getProcedureTerm();
  SQLString getCatalogTerm();
  bool isCatalogAtStart();
  SQLString getCatalogSeparator();
  bool supportsSchemasInDataManipulation();
  bool supportsSchemasInProcedureCalls();
  bool supportsSchemasInTableDefinitions();
  bool supportsSchemasInIndexDefinitions();
  bool supportsSchemasInPrivilegeDefinitions();
  bool supportsCatalogsInDataManipulation();
  bool supportsCatalogsInProcedureCalls();
  bool supportsCatalogsInTableDefinitions();
  bool supportsCatalogsInIndexDefinitions();
  bool supportsCatalogsInPrivilegeDefinitions();
  bool supportsPositionedDelete();
  bool supportsPositionedUpdate();
  bool supportsSelectForUpdate();
  bool supportsStoredProcedures();
  bool supportsSubqueriesInComparisons();
  bool supportsSubqueriesInExists();
  bool supportsSubqueriesInIns();
  bool supportsSubqueriesInQuantifieds();
  bool supportsCorrelatedSubqueries();
  bool supportsUnion();
  bool supportsUnionAll();
  bool supportsOpenCursorsAcrossCommit();
  bool supportsOpenCursorsAcrossRollback();
  bool supportsOpenStatementsAcrossCommit();
  bool supportsOpenStatementsAcrossRollback();
  int32_t getMaxBinaryLiteralLength();
  int32_t getMaxCharLiteralLength();
  int32_t getMaxColumnNameLength();
  int32_t getMaxColumnsInGroupBy();
  int32_t getMaxColumnsInIndex();
  int32_t getMaxColumnsInOrderBy();
  int32_t getMaxColumnsInSelect();
  int32_t getMaxColumnsInTable();
  int32_t getMaxConnections();
  int32_t getMaxCursorNameLength();
  int32_t getMaxIndexLength();
  int32_t getMaxSchemaNameLength();
  int32_t getMaxProcedureNameLength();
  int32_t getMaxCatalogNameLength();
  int32_t getMaxRowSize();
  bool doesMaxRowSizeIncludeBlobs();
  int32_t getMaxStatementLength();
  int32_t getMaxStatements();
  int32_t getMaxTableNameLength();
  int32_t getMaxTablesInSelect();
  int32_t getMaxUserNameLength();
  int32_t getDefaultTransactionIsolation();
  bool supportsTransactions();
  bool supportsTransactionIsolationLevel(int32_t level);
  bool supportsDataDefinitionAndDataManipulationTransactions();
  bool supportsDataManipulationTransactionsOnly();
  bool dataDefinitionCausesTransactionCommit();
  bool dataDefinitionIgnoredInTransactions();
  ResultSet* getProcedures(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& procedureNamePattern);

private:
  bool haveInformationSchemaParameters();

public:
  ResultSet* getProcedureColumns(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& procedureNamePattern, const SQLString& columnNamePattern);
  ResultSet* getFunctionColumns(const SQLString&catalog, const SQLString& schemaPattern, const SQLString& functionNamePattern, const SQLString& columnNamePattern);
  ResultSet* getSchemas();
  ResultSet* getSchemas(const SQLString& catalog, const SQLString& schemaPattern);
  ResultSet* getCatalogs();
  ResultSet* getTableTypes();
  ResultSet* getColumnPrivileges(const SQLString& catalog, const SQLString& schema, const SQLString& table, const SQLString& columnNamePattern);
  ResultSet* getTablePrivileges(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern);
  ResultSet* getVersionColumns(const SQLString& catalog, const SQLString& schema, const SQLString& table);

  ResultSet* getCrossReference(const SQLString& parentCatalog, const SQLString& parentSchema, const SQLString& parentTable, const SQLString& foreignCatalog, const SQLString& foreignSchema, const SQLString& foreignTable);
  ResultSet* getTypeInfo();
  ResultSet* getIndexInfo(const SQLString& catalog, const SQLString& schema, const SQLString& table,bool unique,bool approximate);
  bool supportsResultSetType(int32_t type);
  bool supportsResultSetConcurrency(int32_t type,int32_t concurrency);
  bool ownUpdatesAreVisible(int32_t type);
  bool ownDeletesAreVisible(int32_t type);
  bool ownInsertsAreVisible(int32_t type);
  bool othersUpdatesAreVisible(int32_t type);
  bool othersDeletesAreVisible(int32_t type);
  bool othersInsertsAreVisible(int32_t type);
  bool updatesAreDetected(int32_t type);
  bool deletesAreDetected(int32_t type);
  bool insertsAreDetected(int32_t type);
  bool supportsBatchUpdates();
  ResultSet* getUDTs(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& typeNamePattern, std::list<int32_t>& types);
  Connection* getConnection();
  bool supportsSavepoints();
  bool supportsNamedParameters();
  bool supportsMultipleOpenResults();
  bool supportsGetGeneratedKeys();
  ResultSet* getSuperTypes(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& typeNamePattern);
  ResultSet* getSuperTables(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& tableNamePattern);

  ResultSet* getAttributes(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& typeNamePattern, const SQLString& attributeNamePattern);
  bool supportsResultSetHoldability(int32_t holdability);
  int32_t getResultSetHoldability();
  uint32_t getDatabaseMajorVersion();
  uint32_t getDatabaseMinorVersion();
  uint32_t getDatabasePatchVersion();
  uint32_t getJDBCMajorVersion();
  uint32_t getJDBCMinorVersion();
  int32_t getSQLStateType();
  bool locatorsUpdateCopy();
  bool supportsStatementPooling();
  RowIdLifetime getRowIdLifetime();
  bool supportsStoredFunctionsUsingCallSyntax();
  bool autoCommitFailureClosesAllResultSets();
  ResultSet* getClientInfoProperties();
  ResultSet* getFunctions(const SQLString& catalog, const SQLString& schemaPattern, const SQLString& functionNamePattern);
  //public:  <T>T unwrap(const Class<T>iface);
  //public:  bool isWrapperFor(const Class<?>iface);
  int64_t getMaxLogicalLobSize();
  bool supportsRefCursors();

  bool supportsTypeConversion();
  /* Group of not supported methods */
  ResultSet* getSchemaObjectTypes();
  ResultSet* getSchemaObjects(const SQLString& c, const SQLString& s, const SQLString& t);
  ResultSet* getSchemaObjects();
  uint32_t getCDBCMajorVersion();
  uint32_t getCDBCMinorVersion();
  ResultSet* getSchemaCollation(const SQLString& c, const SQLString& s);
  ResultSet* getSchemaCharset(const SQLString& c, const SQLString& s);
  ResultSet* getTableCollation(const SQLString& c, const SQLString& s, const SQLString& t);
  ResultSet* getTableCharset(const SQLString& c, const SQLString& s, const SQLString& t);

};

}
}
#endif