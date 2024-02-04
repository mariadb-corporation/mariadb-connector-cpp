/************************************************************************************
   Copyright (C) 2020,2024 MariaDB Corporation plc

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
  bool      useFractionalSeconds= true;
  bool      pinGlobalTxToPhysicalConnection;
  SQLString socketFactory;
  int32_t   connectTimeout= 30000;
  SQLString pipe;
  SQLString localSocket;
  SQLString sharedMemory;
  bool      tcpNoDelay= true;
  bool      tcpKeepAlive= true;
  int32_t   tcpRcvBuf;
  int32_t   tcpSndBuf;
  bool      tcpAbortiveClose;
  SQLString localSocketAddress;
  int32_t   socketTimeout= 0;
  bool      allowMultiQueries;
  bool      rewriteBatchedStatements;
  bool      useCompression;
  bool      interactiveClient;
  SQLString passwordCharacterEncoding;
  SQLString useCharacterEncoding;
  bool      blankTableNameMeta;
  SQLString credentialType;
  bool      useTls= false;
  SQLString enabledTlsCipherSuites;
  SQLString sessionVariables;
  bool      tinyInt1isBit= true;
  bool      yearIsDateType= true;
  bool      createDatabaseIfNotExist;
  SQLString serverTimezone;
  bool      nullCatalogMeansCurrent= true;
  bool      dumpQueriesOnException;
  bool      useOldAliasMetadataBehavior;
  bool      useMysqlMetadata;
  bool      allowLocalInfile= true;
  bool      cachePrepStmts= true;
  int32_t   prepStmtCacheSize= 250;
  int32_t   prepStmtCacheSqlLimit= 2048;
  bool      useAffectedRows;
  bool      maximizeMysqlCompatibility;
  bool      useServerPrepStmts;
  bool      continueBatchOnError= true;
  bool      jdbcCompliantTruncation= true;
  bool      cacheCallableStmts= false;
  int32_t   callableStmtCacheSize= 150;
  SQLString connectionAttributes;
  bool      useBatchMultiSend;
  int32_t   useBatchMultiSendNumber= 100;
  bool      usePipelineAuth;
  bool      enablePacketDebug;
  bool      useBulkStmts;
  bool      disableSslHostnameVerification;
  bool      autocommit= true;
  bool      includeInnodbStatusInDeadlockExceptions;
  bool      includeThreadDumpInDeadlockExceptions;
  SQLString servicePrincipalName;
  int32_t   defaultFetchSize;

  Properties nonMappedOptions;

  uint32_t  log;
  bool      profileSql;
  int32_t   maxQuerySizeToLog= 1024;
  int64_t   slowQueryThresholdNanos;
  bool      assureReadOnly;
  bool      autoReconnect;
  bool      failOnReadOnly;
  int32_t   retriesAllDown= 120;
  int32_t   validConnectionTimeout;
  int32_t   loadBalanceBlacklistTimeout= 50;
  int32_t   failoverLoopRetries= 120;
  bool      allowMasterDownConnection;
  SQLString galeraAllowedState;
  bool      pool= false;
  SQLString poolName;
  int32_t   maxPoolSize= 8;
  int32_t   minPoolSize;
  int32_t   maxIdleTime= 600;
  bool      staticGlobal;
  int32_t   poolValidMinDelay= 1000;
  bool      useResetConnection;
  bool      useReadAheadInput= true;
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
