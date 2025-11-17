/************************************************************************************
   Copyright (C) 2023 MariaDB Corporation AB

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


#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <vector>
#include <mutex>
#include <memory>

#include "mysql.h"

#include "lru/pscache.h"
#include "SQLString.h"
#include "Exception.h"
#include "pimpls.h"

namespace mariadb
{

enum IsolationLevel {
  TRANSACTION_NONE= 0,
  TRANSACTION_READ_UNCOMMITTED= 1,
  TRANSACTION_READ_COMMITTED= 2,
  TRANSACTION_REPEATABLE_READ= 4,
  TRANSACTION_SERIALIZABLE =8
};


// Some independent helper functions
SQLException fromStmtError(MYSQL_STMT* stmt);
void         throwStmtError(MYSQL_STMT* stmt);
SQLException fromConnError(MYSQL* dbc);
void         throwConnError(MYSQL* dbc);
SQLString&   addTxIsolationName2Query(SQLString& query, enum IsolationLevel txIsolation);

SQLString&   addQueryTimeout(SQLString& sql, int32_t queryTimeout);


class Protocol
{
  std::mutex lock;
  Unique::MYSQL connection;
  enum IsolationLevel transactionIsolationLevel= TRANSACTION_NONE;
  int64_t maxRows= 0;
  MYSQL_STMT* statementIdToRelease= nullptr;
  bool interrupted= false;
  bool hasWarningsFlag= false;
  /* This cannot be Shared as long as C/C stmt handle is owned by statement(SSPS class in this case) object */
  Results* activeStreamingResult= nullptr;
  uint32_t serverStatus= 0;
  // Last command result, used in process
  int32_t rc= 0;

  int32_t autoIncrementIncrement= 1;

  bool          readOnly= false;
  volatile bool connected= false;
  bool          explicitClosed= false;
  SQLString     database;
 
  std::unique_ptr<Cache<std::string, ServerPrepareResult>> serverPrepareStatementCache;

  int64_t serverCapabilities= 0;
  int32_t socketTimeout= 0;

  SQLString serverVersion;
  bool      serverMariaDb= true;
  uint32_t majorVersion= 11;
  uint32_t minorVersion= 0;
  uint32_t patchVersion= 0;
  SQLString txIsolationVarName;
  bool     mustReset= false;
  bool     ansiQuotes= false;

  // ----- private methods -----
  void unblockConnection();
  void cmdPrologue();
  void cmdEpilog();
  void changeReadTimeout(int32_t millis);
  void checkClose();
  void processResult(Results*, ServerPrepareResult *pr);
  void readResultSet(Results*, ServerPrepareResult *pr);
  void readOk(Results*, ServerPrepareResult *pr);
  void handleStateChange();
  void closeSocket();
  void cleanMemory();
  void parseVersion(const SQLString& _serverVersion);
  void destroySocket();
  void abortActiveStream();
  void sendSessionInfos(const char *trIsolVarName);
  uint32_t             errorOccurred(ServerPrepareResult *pr);
  SQLException         processError(Results*, ServerPrepareResult *pr);
  ServerPrepareResult* prepareInternal(const SQLString& sql);
  Protocol()= delete;
  void unsyncedReset();

  static void resetError(MYSQL_STMT *stmt);

public:
  static const int64_t MAX_PACKET_LENGTH;
  static bool checkRemainingSize(int64_t newQueryLen);

  ~Protocol() {}
  Protocol(MYSQL *connectedHandle, const SQLString& defaultDb, Cache<std::string,
    ServerPrepareResult> *psCache= nullptr, /* Temporary before move things here */const char *trIsolVarName= nullptr,
    enum IsolationLevel txIsolation= TRANSACTION_REPEATABLE_READ);

  ServerPrepareResult* prepare(const SQLString& sql);
  bool getAutocommit();
  bool noBackslashEscapes();
  //void connect(); //not used
  bool inTransaction();
  void close();
  void abort();
  void reset();
  void closeExplicit();
  void markClosed(bool closed);
  bool isClosed();
  //void resetDatabase(); //not used
  const SQLString& getSchema();
  void setSchema(const SQLString& database);
  const SQLString& getServerVersion() const;
  bool isConnected();
  uint32_t fieldCount(ServerPrepareResult *pr);
  bool getReadonly() const;
  void setReadonly(bool readOnly);
  bool mustBeMasterConnection();
  void commit();
  void rollback();
  const SQLString& getDatabase() const;
  const SQLString& getUsername() const;
  bool ping();
  bool isValid(int32_t timeout);
  void executeQuery(const SQLString& sql);
  void executeQuery(Results*, const SQLString& sql);
  void executePreparedQuery(ServerPrepareResult* serverPrepareResult, Results*);
  void directExecutePreparedQuery(ServerPrepareResult* serverPrepareResult, Results*);

  // Technically, they(particular batch methods) probably should be private,
  // but leaving so far the opportunity to call them from outside
  bool executeBulkBatch(
    Results* results, const SQLString& sql,
    ServerPrepareResult* serverPrepareResult,
    MYSQL_BIND* parametersList,
    uint32_t paramsetsCount);
  // Executes batch by creating singe ;-separated multistatement query or for INSERT can assemble single
  // statement with multiple values sets.
  void executeBatchRewrite(
    Results* results,
    ClientPrepareResult* prepareResult,
    MYSQL_BIND* parametersList,
    uint32_t paramsetsCount,
    bool rewriteValues);
  void executeBatchMulti(
    Results* results,
    ClientPrepareResult* clientPrepareResult,
    MYSQL_BIND* parametersList,
    uint32_t paramsetsCount);

  // Method to execute batch prepated on the client. Chooses what method to use and calls it
  bool executeBatchClient(
    Results* results,
    ClientPrepareResult* prepareResult,
    MYSQL_BIND* parametersList,
    uint32_t paramsetsCount,
    bool hasLongData= false);

  void moveToNextSpsResult(Results*, ServerPrepareResult* spr);
  //void skipNextResult(ServerPrepareResult* spr= nullptr);
  void skipAllResults(ServerPrepareResult* spr);
  void skipAllResults();
  void moveToNextResult(Results*, ServerPrepareResult* spr= nullptr);
  void getResult(Results*, ServerPrepareResult *pr=nullptr, bool readAllResults= false);
  //void cancelCurrentQuery();
  void interrupt();
  void skip();
  bool hasWarnings();
  int64_t getMaxRows();
  void setMaxRows(int64_t max);
  uint32_t getMajorServerVersion();
  uint32_t getMinorServerVersion();
  uint32_t getPatchServerVersion();
  bool versionGreaterOrEqual(uint32_t major, uint32_t minor, uint32_t patch) const;
  int32_t getTimeout();
  void setTimeout(int32_t timeout);
  int64_t getServerThreadId();
  int32_t getTransactionIsolationLevel();
  bool isExplicitClosed();
  // void connectWithoutProxy(); //not used
  void releasePrepareStatement(ServerPrepareResult* serverPrepareResult);
  bool forceReleasePrepareStatement(MYSQL_STMT* statementId);
  void forceReleaseWaitingPrepareStatement();
  //Cache* prepareStatementCache();
  void prolog(int64_t maxRows, bool hasProxy, PreparedStatement* statement);
  Results* getActiveStreamingResult();
  void setActiveStreamingResult(Results* mariaSelectResultSet);
  std::mutex& getLock();
  bool hasMoreResults();
  bool hasMoreResults(Results *results);
  bool hasSpOutparams();
  void setServerStatus(uint32_t serverStatus);
  uint32_t getServerStatus();
  void removeHasMoreResults();
  void setHasWarnings(bool hasWarnings);
  ServerPrepareResult* addPrepareInCache(const SQLString& key, ServerPrepareResult* serverPrepareResult);
  void realQuery(const SQLString& sql);
  void realQuery(const char* query, std::size_t length);
  void sendQuery(const SQLString& sql);
  void sendQuery(const char* query, std::size_t length);
  //mysql_read_result
  void readQueryResult();

private:
  void commitReturnAutocommit(bool justReadMultiSendResults=false);

public:
  void safeRealQuery(const SQLString& sql);
  void removeActiveStreamingResult();
  void resetStateAfterFailover( int64_t maxRows, enum IsolationLevel transactionIsolationLevel, const SQLString& database,bool autocommit);
  bool isServerMariaDb();
  int32_t getAutoIncrementIncrement();
  bool sessionStateAware();
  bool isInterrupted();
  void stopIfInterrupted();
  Cache<std::string, ServerPrepareResult>* prepareStatementCache();
  inline MYSQL* getCHandle() { return connection.get(); }
  void setTransactionIsolation(enum IsolationLevel level);
  inline bool sessionStateChanged() { return (serverStatus & SERVER_SESSION_STATE_CHANGED) != 0; }
  void deferredReset() { mustReset= true; }
  inline bool getAnsiQuotes() const { return serverMariaDb ? serverStatus & SERVER_STATUS_ANSI_QUOTES : ansiQuotes; }
  };

}
#endif
