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


#include "Options.h"

#include "Consts.h"
#include "DefaultOptions.h"

namespace sql
{
namespace mariadb
{
  extern std::map<std::string, DefaultOptions> OptionsMap;

  /* hashMap does not compile with SQLString out of the box, thus std::string */
  int64_t hashProps(const Properties& props)
  {
    int64_t result= 0;

    for (auto it : props)
    {
      result+= (it.first.hashCode() ^ (it.second.hashCode() << 1));
    }

    return result;
  }

  int32_t Options::MIN_VALUE__MAX_IDLE_TIME= 60;

#define OPTIONS_FIELD(_FIELD) {#_FIELD, &Options::_FIELD}

  std::map<std::string, ClassField<Options>> Options::Field{
    OPTIONS_FIELD(user),
    OPTIONS_FIELD(password),
    OPTIONS_FIELD(trustServerCertificate),
    OPTIONS_FIELD(serverSslCert),
    OPTIONS_FIELD(tlsKey),
    OPTIONS_FIELD(tlsCRLPath),
    OPTIONS_FIELD(tlsCRL),
    OPTIONS_FIELD(tlsCert),
    OPTIONS_FIELD(tlsCA),
    OPTIONS_FIELD(tlsCAPath),
    OPTIONS_FIELD(keyPassword),
    OPTIONS_FIELD(enabledTlsProtocolSuites),
    OPTIONS_FIELD(useFractionalSeconds),
    OPTIONS_FIELD(pinGlobalTxToPhysicalConnection),
    OPTIONS_FIELD(socketFactory),
    OPTIONS_FIELD(connectTimeout),
    OPTIONS_FIELD(pipe),
    OPTIONS_FIELD(localSocket),
    OPTIONS_FIELD(sharedMemory),
    OPTIONS_FIELD(tcpNoDelay),
    OPTIONS_FIELD(tcpKeepAlive),
    OPTIONS_FIELD(tcpRcvBuf),
    OPTIONS_FIELD(tcpSndBuf),
    OPTIONS_FIELD(tcpAbortiveClose),
    OPTIONS_FIELD(localSocketAddress),
    OPTIONS_FIELD(socketTimeout),
    OPTIONS_FIELD(allowMultiQueries),
    OPTIONS_FIELD(rewriteBatchedStatements),
    OPTIONS_FIELD(useCompression),
    OPTIONS_FIELD(interactiveClient),
    OPTIONS_FIELD(passwordCharacterEncoding),
    OPTIONS_FIELD(useCharacterEncoding),
    OPTIONS_FIELD(blankTableNameMeta),
    OPTIONS_FIELD(credentialType),
    OPTIONS_FIELD(useTls),
    OPTIONS_FIELD(enabledTlsCipherSuites),
    OPTIONS_FIELD(sessionVariables),
    OPTIONS_FIELD(tinyInt1isBit),
    OPTIONS_FIELD(yearIsDateType),
    OPTIONS_FIELD(createDatabaseIfNotExist),
    OPTIONS_FIELD(serverTimezone),
    OPTIONS_FIELD(nullCatalogMeansCurrent),
    OPTIONS_FIELD(dumpQueriesOnException),
    OPTIONS_FIELD(useOldAliasMetadataBehavior),
    OPTIONS_FIELD(useMysqlMetadata),
    OPTIONS_FIELD(allowLocalInfile),
    OPTIONS_FIELD(cachePrepStmts),
    OPTIONS_FIELD(prepStmtCacheSize),
    OPTIONS_FIELD(prepStmtCacheSqlLimit),
    OPTIONS_FIELD(useAffectedRows),
    OPTIONS_FIELD(maximizeMysqlCompatibility),
    OPTIONS_FIELD(useServerPrepStmts),
    OPTIONS_FIELD(continueBatchOnError),
    OPTIONS_FIELD(jdbcCompliantTruncation),
    OPTIONS_FIELD(cacheCallableStmts),
    OPTIONS_FIELD(callableStmtCacheSize),
    OPTIONS_FIELD(connectionAttributes),
    OPTIONS_FIELD(useBatchMultiSend),
    OPTIONS_FIELD(useBatchMultiSendNumber),
    OPTIONS_FIELD(usePipelineAuth),
    OPTIONS_FIELD(enablePacketDebug),
    OPTIONS_FIELD(useBulkStmts),
    OPTIONS_FIELD(disableSslHostnameVerification),
    OPTIONS_FIELD(autocommit),
    OPTIONS_FIELD(includeInnodbStatusInDeadlockExceptions),
    OPTIONS_FIELD(includeThreadDumpInDeadlockExceptions),
    OPTIONS_FIELD(servicePrincipalName),
    OPTIONS_FIELD(defaultFetchSize),
    OPTIONS_FIELD(tlsPeerFPList),
    OPTIONS_FIELD(log),
    OPTIONS_FIELD(profileSql),
    OPTIONS_FIELD(maxQuerySizeToLog),
    OPTIONS_FIELD(slowQueryThresholdNanos),
    OPTIONS_FIELD(assureReadOnly),
    OPTIONS_FIELD(autoReconnect),
    OPTIONS_FIELD(failOnReadOnly),
    OPTIONS_FIELD(retriesAllDown),
    OPTIONS_FIELD(validConnectionTimeout),
    OPTIONS_FIELD(loadBalanceBlacklistTimeout),
    OPTIONS_FIELD(failoverLoopRetries),
    OPTIONS_FIELD(allowMasterDownConnection),
    OPTIONS_FIELD(galeraAllowedState),
    OPTIONS_FIELD(pool),
    OPTIONS_FIELD(poolName),
    OPTIONS_FIELD(maxPoolSize),
    OPTIONS_FIELD(minPoolSize),
    OPTIONS_FIELD(maxIdleTime),
    OPTIONS_FIELD(staticGlobal),
    OPTIONS_FIELD(poolValidMinDelay),
    OPTIONS_FIELD(useResetConnection),
    OPTIONS_FIELD(useReadAheadInput),
    OPTIONS_FIELD(serverRsaPublicKeyFile),
    OPTIONS_FIELD(tlsPeerFP)
  };


  ClassField<Options>& Options::getField(const SQLString& fieldName)
  {
    static ClassField<Options> emptyField;

    auto it= Field.find(StringImp::get(fieldName));

    if (it != Field.end())
    {
      return it->second;
    }

    return emptyField;
  }


  Options::Options():
        connectTimeout      (30000),
        useFractionalSeconds(true),
        useTls              (false),
        tcpNoDelay          (true),
        tcpKeepAlive        (true),
        tinyInt1isBit       (true),
        yearIsDateType      (true),
        nullCatalogMeansCurrent(true),
        allowLocalInfile    (true),
        cachePrepStmts      (true),
        prepStmtCacheSize   (250),
        prepStmtCacheSqlLimit(2048),
        continueBatchOnError(true),
        jdbcCompliantTruncation(true),
        cacheCallableStmts  (false),
        callableStmtCacheSize(150),
        useBatchMultiSendNumber(100),
        autocommit          (true),
        maxQuerySizeToLog   (1024),
        retriesAllDown      (120),
        loadBalanceBlacklistTimeout(50),
        failoverLoopRetries (120),
        maxPoolSize         (8),
        maxIdleTime         (600),
        poolValidMinDelay   (1000),
        useReadAheadInput   (true),
        pool                (false),
        socketTimeout       (0)
  {
    for (auto& it : Field) {
      const auto& cit= OptionsMap.find(it.first);

      if (cit != OptionsMap.end())
      {
        try {
          switch (it.second.objType())
          {
          case ClassField<Options>::VBOOL: {
            bool Options::*boolField= it.second;
            this->*boolField= cit->second.defaultValue;
            break;
          }
          case ClassField<Options>::VINT32: {
            int32_t Options::*fieldPtr= it.second;
            this->*fieldPtr= cit->second.defaultValue;
            break;
          }
          case ClassField<Options>::VINT64: {
            int64_t Options::*fieldPtr= it.second;
            this->*fieldPtr= cit->second.defaultValue;
            break;
          }
          case ClassField<Options>::VSTRING: {
            SQLString Options::*fieldPtr= it.second;
            this->*fieldPtr= static_cast<const char*>(cit->second.defaultValue);
            break;
          }
          case ClassField<Options>::VNONE:
            break; 
          }
        }
        catch (std::invalid_argument& /*e*/) {
          SQLString msg("Could not load options defaults: field ");
          msg.append(it.first);
          msg.append(", type");
          msg.append(std::to_string(it.second.objType()));
          msg.append(", value type");
          msg.append(std::to_string(cit->second.defaultValue.objType()));
          SQLException refined(msg);
          throw refined;
        }
      }
    }
  }


  SQLString Options::toString() const
  {
    SQLString result;
    SQLString newLine("\n");

    result.append("Options");
    result.append(" Options {");
    result.append(newLine);
    // TODO: Get back to this if we really need it in C++
    //Field[] fields= this->getClass().getDeclaredFields();
    //for (Field field : fields) {
    //  result.append("  ");
    //  try {
    //    result.append(field.getName());
    //    result.append(": ");
    //    result.append(field.get(this));
    //  }
    //  catch (std::exception &e) {
    //    /* Not sure if append throw anything. Need to check */
    //  }
    //  result.append(newLine);
    //}
    result.append("}");
    return result;
  }


  bool Options::equals(Options* opt)
  {
    if (opt == NULL) {
      return false;
    }
    if (this == opt) {
      return true;
    }
    if (trustServerCertificate != opt->trustServerCertificate) {
      return false;
    }
    if (useFractionalSeconds != opt->useFractionalSeconds) {
      return false;
    }
    if (pinGlobalTxToPhysicalConnection != opt->pinGlobalTxToPhysicalConnection) {
      return false;
    }
    if (tcpNoDelay != opt->tcpNoDelay) {
      return false;
    }
    if (tcpKeepAlive != opt->tcpKeepAlive) {
      return false;
    }
    if (tcpAbortiveClose != opt->tcpAbortiveClose) {
      return false;
    }
    if (blankTableNameMeta != opt->blankTableNameMeta) {
      return false;
    }
    if (allowMultiQueries != opt->allowMultiQueries) {
      return false;
    }
    if (rewriteBatchedStatements != opt->rewriteBatchedStatements) {
      return false;
    }
    if (useCompression != opt->useCompression) {
      return false;
    }
    if (interactiveClient != opt->interactiveClient) {
      return false;
    }
    if (useTls != opt->useTls) {
      return false;
    }
    if (tinyInt1isBit != opt->tinyInt1isBit) {
      return false;
    }
    if (yearIsDateType != opt->yearIsDateType) {
      return false;
    }
    if (createDatabaseIfNotExist != opt->createDatabaseIfNotExist) {
      return false;
    }
    if (nullCatalogMeansCurrent != opt->nullCatalogMeansCurrent) {
      return false;
    }
    if (dumpQueriesOnException != opt->dumpQueriesOnException) {
      return false;
    }
    if (useOldAliasMetadataBehavior != opt->useOldAliasMetadataBehavior) {
      return false;
    }
    if (allowLocalInfile != opt->allowLocalInfile) {
      return false;
    }
    if (cachePrepStmts != opt->cachePrepStmts) {
      return false;
    }
    if (useAffectedRows != opt->useAffectedRows) {
      return false;
    }
    if (maximizeMysqlCompatibility != opt->maximizeMysqlCompatibility) {
      return false;
    }
    if (useServerPrepStmts != opt->useServerPrepStmts) {
      return false;
    }
    if (continueBatchOnError != opt->continueBatchOnError) {
      return false;
    }
    if (jdbcCompliantTruncation != opt->jdbcCompliantTruncation) {
      return false;
    }
    if (cacheCallableStmts != opt->cacheCallableStmts) {
      return false;
    }
    if (useBatchMultiSendNumber != opt->useBatchMultiSendNumber) {
      return false;
    }
    if (enablePacketDebug != opt->enablePacketDebug) {
      return false;
    }
    if (includeInnodbStatusInDeadlockExceptions != opt->includeInnodbStatusInDeadlockExceptions) {
      return false;
    }
    if (includeThreadDumpInDeadlockExceptions != opt->includeThreadDumpInDeadlockExceptions) {
      return false;
    }
    if (defaultFetchSize != opt->defaultFetchSize) {
      return false;
    }
    if (useBulkStmts != opt->useBulkStmts) {
      return false;
    }
    if (disableSslHostnameVerification != opt->disableSslHostnameVerification) {
      return false;
    }
    if (log != opt->log) {
      return false;
    }
    if (profileSql != opt->profileSql) {
      return false;
    }
    if (assureReadOnly != opt->assureReadOnly) {
      return false;
    }
    if (autoReconnect != opt->autoReconnect) {
      return false;
    }
    if (failOnReadOnly != opt->failOnReadOnly) {
      return false;
    }
    if (allowMasterDownConnection != opt->allowMasterDownConnection) {
      return false;
    }
    if (retriesAllDown != opt->retriesAllDown) {
      return false;
    }
    if (validConnectionTimeout != opt->validConnectionTimeout) {
      return false;
    }
    if (loadBalanceBlacklistTimeout != opt->loadBalanceBlacklistTimeout) {
      return false;
    }
    if (failoverLoopRetries != opt->failoverLoopRetries) {
      return false;
    }
    if (pool != opt->pool) {
      return false;
    }
    if (staticGlobal != opt->staticGlobal) {
      return false;
    }
    if (useResetConnection != opt->useResetConnection) {
      return false;
    }
    if (useReadAheadInput != opt->useReadAheadInput) {
      return false;
    }
    if (maxPoolSize != opt->maxPoolSize) {
      return false;
    }
    if (maxIdleTime != opt->maxIdleTime) {
      return false;
    }
    if (poolValidMinDelay != opt->poolValidMinDelay) {
      return false;
    }
    if (user.compare(opt->user) != 0) {
      return false;
    }
    if (password.compare(opt->password) != 0) {
      return false;
    }
    if (!(serverSslCert.compare(opt->serverSslCert) == 0)) {
      return false;
    }
    if (!(tlsKey.compare(opt->tlsKey) == 0)) {
      return false;
    }
    if (tlsCert.compare(opt->tlsCert) != 0) {
      return false;
    }
    if (!(tlsCA.compare(opt->tlsCA) == 0)) {
      return false;
    }
    if (!(tlsCAPath.compare(opt->tlsCAPath) == 0)) {
      return false;
    }
    if (!(keyPassword.compare(opt->keyPassword) == 0)) {
      return false;
    }
    if (!enabledTlsProtocolSuites.empty()) {
      if (enabledTlsProtocolSuites.compare(opt->enabledTlsProtocolSuites) != 0) {
        return false;
      }
    }
    else if (!opt->enabledTlsProtocolSuites.empty()) {
      return false;
    }
    if (!(socketFactory.compare(opt->socketFactory) == 0)) {
      return false;
    }
    if (connectTimeout != opt->connectTimeout) {
      return false;
    }
    if (!(pipe.compare(opt->pipe) == 0)) {
      return false;
    }
    if (!(localSocket.compare(opt->localSocket) == 0)) {
      return false;
    }
    if (!(sharedMemory.compare(opt->sharedMemory) == 0)) {
      return false;
    }
    if (tcpRcvBuf != opt->tcpRcvBuf) {
      return false;
    }
    if (tcpSndBuf != opt->tcpSndBuf) {
      return false;
    }
    if (!(localSocketAddress.compare(opt->localSocketAddress) == 0)) {
      return false;
    }
    if (socketTimeout != opt->socketTimeout) {
      return false;
    }
    if (!passwordCharacterEncoding.empty()) {
      if (passwordCharacterEncoding.compare(opt->passwordCharacterEncoding) != 0) {
        return false;
      }
    }
    else if (!opt->passwordCharacterEncoding.empty()) {
      return false;
    }
    if (!useCharacterEncoding.empty()) {
      if (useCharacterEncoding.compare(opt->useCharacterEncoding) != 0) {
        return false;
      }
    }
    else if (!opt->useCharacterEncoding.empty()) {
      return false;
    }
    if (!(enabledTlsCipherSuites.compare(opt->enabledTlsCipherSuites) == 0)) {
      return false;
    }
    if (!(sessionVariables.compare(opt->sessionVariables) == 0)) {
      return false;
    }
    if (!(serverTimezone.compare(opt->serverTimezone) == 0)) {
      return false;
    }
    if (prepStmtCacheSize != opt->prepStmtCacheSize) {
      return false;
    }
    if (prepStmtCacheSqlLimit != opt->prepStmtCacheSqlLimit) {
      return false;
    }
    if (callableStmtCacheSize != opt->callableStmtCacheSize) {
      return false;
    }
    if (!(connectionAttributes.compare(opt->connectionAttributes) == 0)) {
      return false;
    }
    if (useBatchMultiSend != opt->useBatchMultiSend) {
      return false;
    }
    if (usePipelineAuth != opt->usePipelineAuth) {
      return false;
    }
    if (maxQuerySizeToLog != opt->maxQuerySizeToLog) {
      return false;
    }
    if (slowQueryThresholdNanos != opt->slowQueryThresholdNanos) {
      return false;
    }
    if (autocommit != opt->autocommit) {
      return false;
    }
    if (!(poolName.compare(opt->poolName) == 0)) {
      return false;
    }
    if (!(galeraAllowedState.compare(opt->galeraAllowedState) == 0)) {
      return false;
    }
    if (!(credentialType.compare(opt->credentialType) == 0)) {
      return false;
    }
    /* TODO: compare maps. Or find other solution */
    /*if (!(nonMappedOptions.compare(opt->nonMappedOptions) == 0)) {
      return false;
    }*/
    if (!(tlsPeerFPList.compare(opt->tlsPeerFPList) == 0)) {
      return false;
    }
    return minPoolSize == opt->minPoolSize;
  }


  int64_t Options::hashCode() const
  {
    int64_t result= !user.empty() ? user.hashCode() : 0;
    result= 31 *result + (!password.empty() ? password.hashCode() : 0);
    result= 31 *result + (trustServerCertificate ? 1 : 0);
    result= 31 *result + (!serverSslCert.empty() ? serverSslCert.hashCode() : 0);
    result= 31 *result + (!tlsKey.empty() ? tlsKey.hashCode() : 0);
    result= 31 *result + (!tlsCert.empty() ? tlsCert.hashCode() : 0);
    result= 31 *result + (!tlsCA.empty() ? tlsCA.hashCode() : 0);
    result= 31 *result + (!tlsCAPath.empty() ? tlsCAPath.hashCode() : 0);
    result= 31 *result + (!keyPassword.empty() ? keyPassword.hashCode() : 0);
    result =
      31 *result + (!enabledTlsProtocolSuites.empty() ? enabledTlsProtocolSuites.hashCode() : 0);
    result= 31 *result + (useFractionalSeconds ? 1 : 0);
    result= 31 *result + (pinGlobalTxToPhysicalConnection ? 1 : 0);
    result= 31 *result + (!socketFactory.empty() ? socketFactory.hashCode() : 0);
    result= 31 *result +connectTimeout;
    result= 31 *result + (!pipe.empty() ? pipe.hashCode() : 0);
    result= 31 *result + (!localSocket.empty() ? localSocket.hashCode() : 0);
    result= 31 *result + (!sharedMemory.empty() ? sharedMemory.hashCode() : 0);
    result= 31 *result + (tcpNoDelay ? 1 : 0);
    result= 31 *result + (tcpKeepAlive ? 1 : 0);
    result= 31 *result + (tcpRcvBuf > 0 ? hash(tcpRcvBuf) : 0);
    result= 31 *result + (tcpSndBuf ? hash(tcpSndBuf) : 0);
    result= 31 *result + (tcpAbortiveClose ? 1 : 0);
    result= 31 *result + (!localSocketAddress.empty() ? localSocketAddress.hashCode() : 0);
    result= 31 *result + (socketTimeout ? hash(socketTimeout) : 0);
    result= 31 *result + (allowMultiQueries ? 1 : 0);
    result= 31 *result + (rewriteBatchedStatements ? 1 : 0);
    result= 31 *result + (useCompression ? 1 : 0);
    result= 31 *result + (interactiveClient ? 1 : 0);
    result= 31 *result + (!passwordCharacterEncoding.empty() ? passwordCharacterEncoding.hashCode() : 0);
    result= 31 *result + (!useCharacterEncoding.empty() ? useCharacterEncoding.hashCode() : 0);
    result= 31 *result + (useTls ? 1 : 0);
    result= 31 *result + (!enabledTlsCipherSuites.empty() ? enabledTlsCipherSuites.hashCode() : 0);
    result= 31 *result + (!sessionVariables.empty() ? sessionVariables.hashCode() : 0);
    result= 31 *result + (tinyInt1isBit ? 1 : 0);
    result= 31 *result + (yearIsDateType ? 1 : 0);
    result= 31 *result + (createDatabaseIfNotExist ? 1 : 0);
    result= 31 *result + (!serverTimezone.empty() ? serverTimezone.hashCode() : 0);
    result= 31 *result + (nullCatalogMeansCurrent ? 1 : 0);
    result= 31 *result + (dumpQueriesOnException ? 1 : 0);
    result= 31 *result + (useOldAliasMetadataBehavior ? 1 : 0);
    result= 31 *result + (allowLocalInfile ? 1 : 0);
    result= 31 *result + (cachePrepStmts ? 1 : 0);
    result= 31 *result +prepStmtCacheSize;
    result= 31 *result +prepStmtCacheSqlLimit;
    result= 31 *result + (useAffectedRows ? 1 : 0);
    result= 31 *result + (maximizeMysqlCompatibility ? 1 : 0);
    result= 31 *result + (useServerPrepStmts ? 1 : 0);
    result= 31 *result + (continueBatchOnError ? 1 : 0);
    result= 31 *result + (jdbcCompliantTruncation ? 1 : 0);
    result= 31 *result + (cacheCallableStmts ? 1 : 0);
    result= 31 *result +callableStmtCacheSize;
    result= 31 *result + (!connectionAttributes.empty() ? connectionAttributes.hashCode() : 0);
    result= 31 *result + (useBatchMultiSend ? hash(useBatchMultiSend) : 0);
    result= 31 *result + useBatchMultiSendNumber;
    result= 31 *result + (usePipelineAuth ? hash(usePipelineAuth) : 0);
    result= 31 *result + (enablePacketDebug ? 1 : 0);
    result= 31 *result + (includeInnodbStatusInDeadlockExceptions ? 1 : 0);
    result= 31 *result + (includeThreadDumpInDeadlockExceptions ? 1 : 0);
    result= 31 *result + (useBulkStmts ? 1 : 0);
    result= 31 *result + defaultFetchSize;
    result= 31 *result + (disableSslHostnameVerification ? 1 : 0);
    result= 31 *result + (log ? 1 : 0);
    result= 31 *result + (profileSql ? 1 : 0);
    result= 31 *result + maxQuerySizeToLog;
    result= 31 *result + (slowQueryThresholdNanos > 0 ? hash(slowQueryThresholdNanos) : 0);
    result= 31 *result + (assureReadOnly ? 1 : 0);
    result= 31 *result + (autoReconnect ? 1 : 0);
    result= 31 *result + (failOnReadOnly ? 1 : 0);
    result= 31 *result + (allowMasterDownConnection ? 1 : 0);
    result= 31 *result +retriesAllDown;
    result= 31 *result +validConnectionTimeout;
    result= 31 *result +loadBalanceBlacklistTimeout;
    result= 31 *result +failoverLoopRetries;
    result= 31 *result + (pool ? 1 : 0);
    result= 31 *result + (useResetConnection ? 1 : 0);
    result= 31 *result + (useReadAheadInput ? 1 : 0);
    result= 31 *result + (staticGlobal ? 1 : 0);
    result= 31 *result + (!poolName.empty() ? poolName.hashCode() : 0);
    result= 31 *result + (!galeraAllowedState.empty() ? galeraAllowedState.hashCode() : 0);
    result= 31 *result + maxPoolSize;
    result= 31 *result + (minPoolSize > 0 ? hash(minPoolSize) : 0);
    result= 31 *result + maxIdleTime;
    result= 31 *result + poolValidMinDelay;
    result= 31 *result + (autocommit ? 1 : 0);
    result= 31 *result + (!credentialType.empty() ? credentialType.hashCode() : 0);

    result= 31 *result + (!nonMappedOptions.empty() ? hashProps(nonMappedOptions) : 0);

    result= 31 *result + (!tlsPeerFPList.empty() ? tlsPeerFPList.hashCode() : 0);
    return result;
  }

  Options* Options::clone() {
    return new Options(*this);
  }

}
}
