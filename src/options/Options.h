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

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <map>
#include <memory>

#include "util/ClassField.h"
#include "conncpp.hpp"

namespace sql
{
namespace mariadb
{

struct Options
{
  static std::map<std::string, ClassField<Options>> Field;
  static ClassField<Options>& getField(const SQLString& field);
  static int32_t MIN_VALUE__MAX_IDLE_TIME;

  Options();

  SQLString user;
  SQLString password;
  /***** TLS options *****/
  bool      trustServerCertificate;
  SQLString serverSslCert;
  SQLString tlsKey;
  SQLString tlsCRLPath;
  SQLString tlsCRL;
  SQLString tlsCert;
  SQLString tlsCA;
  SQLString tlsCAPath;
  SQLString keyPassword;
  SQLString enabledTlsProtocolSuites;
  SQLString tlsPeerFPList;
  /** TLS options - end **/
  bool      useFractionalSeconds;
  bool      pinGlobalTxToPhysicalConnection;
  SQLString socketFactory;
  int32_t   connectTimeout;
  SQLString pipe;
  SQLString localSocket;
  SQLString sharedMemory;
  bool      tcpNoDelay;
  bool      tcpKeepAlive;
  int32_t   tcpRcvBuf;
  int32_t   tcpSndBuf;
  bool      tcpAbortiveClose;
  SQLString localSocketAddress;
  int32_t   socketTimeout;
  bool      allowMultiQueries;
  bool      rewriteBatchedStatements;
  bool      useCompression;
  bool      interactiveClient;
  SQLString passwordCharacterEncoding;
  SQLString useCharacterEncoding;
  bool      blankTableNameMeta;
  SQLString credentialType;
  bool      useTls;
  SQLString enabledTlsCipherSuites;
  SQLString sessionVariables;
  bool      tinyInt1isBit;
  bool      yearIsDateType;
  bool      createDatabaseIfNotExist;
  SQLString serverTimezone;
  bool      nullCatalogMeansCurrent;
  bool      dumpQueriesOnException;
  bool      useOldAliasMetadataBehavior;
  bool      useMysqlMetadata;
  bool      allowLocalInfile;
  bool      cachePrepStmts;
  int32_t   prepStmtCacheSize;
  int32_t   prepStmtCacheSqlLimit;
  bool      useAffectedRows;
  bool      maximizeMysqlCompatibility;
  bool      useServerPrepStmts;
  bool      continueBatchOnError;
  bool      jdbcCompliantTruncation;
  bool      cacheCallableStmts;
  int32_t   callableStmtCacheSize;
  SQLString connectionAttributes;
  bool      useBatchMultiSend;
  int32_t   useBatchMultiSendNumber;
  bool      usePipelineAuth;
  bool      enablePacketDebug;
  bool      useBulkStmts;
  bool      disableSslHostnameVerification;
  bool      autocommit;
  bool      includeInnodbStatusInDeadlockExceptions;
  bool      includeThreadDumpInDeadlockExceptions;
  SQLString servicePrincipalName;
  int32_t   defaultFetchSize;

  Properties nonMappedOptions;

  bool      log;
  bool      profileSql;
  int32_t   maxQuerySizeToLog;
  int64_t   slowQueryThresholdNanos;
  bool      assureReadOnly;
  bool      autoReconnect;
  bool      failOnReadOnly;
  int32_t   retriesAllDown;
  int32_t   validConnectionTimeout;
  int32_t   loadBalanceBlacklistTimeout;
  int32_t   failoverLoopRetries;
  bool      allowMasterDownConnection;
  SQLString galeraAllowedState;
  bool      pool;
  SQLString poolName;
  int32_t   maxPoolSize;
  int32_t   minPoolSize;
  int32_t   maxIdleTime;
  bool      staticGlobal;
  int32_t   poolValidMinDelay;
  bool      useResetConnection;
  bool      useReadAheadInput;
  SQLString serverRsaPublicKeyFile;
  SQLString tlsPeerFP;

  SQLString toString() const;
  bool      equals(Options* obj);
  int64_t   hashCode() const;
  Options*  clone();
};

}
}

#include "Consts.h"

#endif