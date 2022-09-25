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


#include <cassert>
#include <algorithm>

#include "DefaultOptions.h"
#include "Consts.h"
#include "Exception.hpp"

namespace sql
{
  namespace mariadb
  {
    static const int32_t SocketTimeoutDefault[]= {30000, 0, 0, 0, 0, 0};
    std::map<std::string, DefaultOptions> OptionsMap{
      {"user",     {"user",      "0.9.1", "Database user name",            false} },
      {"password", {"password",  "0.9.1", "Password of the database user", false} },

      /**
      * The connect timeout value, in milliseconds, or zero for no timeout. Default: 30000 (30 seconds)
      * (was 0 before 2.1.2)
      */
       {
         "connectTimeout", {"connectTimeout",
         "0.9.1",
         "The connect timeout value, in milliseconds, or zero for no timeout.",
         false,
         (int32_t)30000,
         int32_t(0)}} ,
       {"pipe", {"pipe",  "0.9.1", "On Windows, specify the named pipe name to connect", false}},
       {
         "localSocket", {"localSocket",
         "0.9.1",
         "For connections to localhost, the Unix socket file to use."
         " \nThe value is the path of Unix domain socket (i.e \"socket\" database parameter : "
         "select @@socket).",
         false}},
       {
         "sharedMemory", {"sharedMemory",
         "0.9.1",
         "Permits connecting to the database via shared memory, if the server allows "
         "it. \nThe value is the base name of the shared memory.",
         false}},
       {
         "tcpNoDelay", {"tcpNoDelay",
         "0.9.1",
         "Sets corresponding option on the connection socket.",
         false,
         true}},
       {
         "tcpAbortiveClose", {"tcpAbortiveClose",
         "0.9.1",
         "Sets corresponding option on the connection socket.",
         false,
         false}},
       {
         "localSocketAddress", {"localSocketAddress",
         "0.9.1",
         "Hostname or IP address to bind the connection socket to a local (UNIX domain) socket.",
         false}} ,
      {
        "socketTimeout", {"socketTimeout",
        "0.9.1",
        "Defined the "
        "network socket timeout (SO_TIMEOUT) in milliseconds. Value of 0 disables this timeout. \n"
        "If the goal is to set a timeout for all queries, since MariaDB 10.1.1, the server has permitted a "
        "solution to limit the query time by setting a system variable, max_statement_time. The advantage is that"
        " the connection then is still usable.\n"
        "Default: 0 (standard configuration) or 10000ms (using \"aurora\" failover configuration).",
        false,
        (int32_t)10000,
        int32_t(0)}},
      {
        "interactiveClient", {"interactiveClient",
        "0.9.1",
        "Session timeout is defined by the wait_timeout "
        "server variable. Setting interactiveClient to true will tell the server to use the interactive_timeout "
        "server variable.",
        false,
        false}},
      {
        "dumpQueriesOnException", {"dumpQueriesOnException",
        "0.9.1",
        "If set to 'true', an exception is thrown "
        "during query execution containing a query string.",
        false,
        false}},
      {
        "useOldAliasMetadataBehavior", {"useOldAliasMetadataBehavior",
        "0.9.1",
        "Metadata ResultSetMetaData.getTableName() returns the physical table name. \"useOldAliasMetadataBehavior\""
        " permits activating the legacy code that sends the table alias if set.",
        false,
        false}},
      {
        "allowLocalInfile", {"allowLocalInfile", "1.0.2", "Permits loading data from local file(on the client)", false, false}},
      {
        "sessionVariables", {"sessionVariables",
        "0.9.1",
        "<var>=<value> pairs separated by comma, mysql session variables, "
        "set upon establishing successful connection.",
        false}},
      {
        "createDatabaseIfNotExist", {"createDatabaseIfNotExist",
        "0.9.1",
        "the specified database in the url will be created if non-existent.",
        false,
        false}},
      {
        "serverTimezone", {"serverTimezone",
        "0.9.1",
        "Defines the server time zone.\n"
        "to use only if the jre server has a different time implementation of the server.\n"
        "(best to have the same server time zone when possible).",
        false}},
      {
        "nullCatalogMeansCurrent", {"nullCatalogMeansCurrent",
        "0.9.1",
        "DatabaseMetaData use current catalog if null.",
        false,
        true}},
      {
        "tinyInt1isBit", {"tinyInt1isBit",
        "0.9.1",
        "Datatype mapping flag, handle Tiny as {boolean).",
        false,
        true}},
      {
        "yearIsDateType", {"yearIsDateType", "0.9.1", "Year is date type, rather than numerical.", false, false}
      },
      {
        "useCompression", {"useCompression",
        "0.9.1",
        "Compresses the exchange with the database through gzip."
        " This permits better performance when the database is not in the same location.",
        false,
        false}},
      {
        "allowMultiQueries", {"allowMultiQueries",
        "0.9.1",
        "permit multi-queries like insert into ab (i) "
        "values (1); insert into ab (i) values (2).",
        false,
        false}},
      {
        "rewriteBatchedStatements", {"rewriteBatchedStatements",
        "1.0.2",
        "For insert queries, rewrite "
        "batchedStatement to execute in a single executeQuery.\n"
        "example:\n"
        "   insert into ab (i) values (?) with first batch values = 1, second = 2 will be rewritten\n"
        "   insert into ab (i) values (1), (2). \n"
        "\n"
        "If query cannot be rewriten in \"multi-values\", rewrite will use multi-queries : INSERT INTO "
        "TABLE(col1) VALUES (?) ON DUPLICATE KEY UPDATE col2=? with values [1,2] and [2,3]\" will be rewritten\n"
        "INSERT INTO TABLE(col1) VALUES (1) ON DUPLICATE KEY UPDATE col2=2;INSERT INTO TABLE(col1) VALUES (3) ON "
        "DUPLICATE KEY UPDATE col2=4\n"
        "\n"
        "when active, the useServerPrepStmts option is set to false",
        false,
        false}},
      {
        "tcpKeepAlive", {"tcpKeepAlive",
        "0.9.1",
        "Sets corresponding option on the connection socket.",
        false,
        true}},
      {
        "tcpRcvBuf", {"tcpRcvBuf",
        "0.9.1",
        "set buffer size for TCP buffer (SO_RCVBUF).",
        false,
        (int32_t)0,
        int32_t(0) }},
      {
        "tcpSndBuf", {"tcpSndBuf",
        "0.9.1",
        "set buffer size for TCP buffer (SO_SNDBUF).",
        false,
        (int32_t)0,
        int32_t(0) } },
      {
        "socketFactory", {"socketFactory",
        "0.9.1",
        "to use a custom socket factory, set it to the full name of the class that"
        " implements javax.net.SocketFactory.",
        false}},
      {
        "pinGlobalTxToPhysicalConnection", {"pinGlobalTxToPhysicalConnection", "0.9.1", "", false, false}},
      {
        "trustServerCertificate", {"trustServerCertificate",
        "0.9.2",
        "When using SSL, do not check server's certificate.",
        false,
        true}
      },
      {
        "serverSslCert", {"serverSslCert",
        "0.9.2",
        "Permits providing server's certificate in DER form, or server's CA"
        " certificate. This permits a self-signed certificate to be trusted.\n"
        "Can be used in one of 3 forms : \n"
        "* serverSslCert=/path/to/cert.pem (full path to certificate)\n"
        "* serverSslCert=classpath:relative/cert.pem (relative to current classpath)\n"
        "* or as verbatim DER-encoded certificate string \"------BEGIN CERTIFICATE-----\" .",
        false}},
      {
        "useFractionalSeconds", {"useFractionalSeconds",
        "0.9.1",
        "Correctly handle subsecond precision in"
        " timestamps (feature available with MariaDB 5.3 and later).\n"
        "May confuse 3rd party components (Hibernate).",
        false,
        true}},
      {
        "autoReconnect", {"autoReconnect",
        "0.9.1",
        "Driver must recreateConnection after a failover.",
        false,
        false}},
      {
        "failOnReadOnly", {"failOnReadOnly",
        "0.9.1",
        "After a master failover and no other master found,"
        " back on a read-only host ( throw exception if not).",
        false,
        false}},
      {
        "retriesAllDown", {"retriesAllDown",
        "0.9.1",
        "When using loadbalancing, the number of times the driver should"
        " cycle through available hosts, attempting to connect.\n"
        "     * Between cycles, the driver will pause for 250ms if no servers are available.",
        false,
        (int32_t)120,
        int32_t(0) }},
      {
        "failoverLoopRetries", {"failoverLoopRetries",
        "0.9.1",
        "When using failover, the number of times the driver"
        " should cycle silently through available hosts, attempting to connect.\n"
        "     * Between cycles, the driver will pause for 250ms if no servers are available.\n"
        "     * if set to 0, there will be no silent reconnection",
        false,
        (int32_t)120,
        int32_t(0) }},
      {
        "validConnectionTimeout", {"validConnectionTimeout",
        "0.9.1",
        "When in multiple hosts, after this time in"
        " second without used, verification that the connections haven't been lost.\n"
        "     * When 0, no verification will be done. Defaults to 0 (120 before 1.5.8 version)",
        false,
        (int32_t)0,
        int32_t(0)}},
      {
        "loadBalanceBlacklistTimeout", {"loadBalanceBlacklistTimeout",
        "0.9.1",
        "time in second a server is blacklisted after a connection failure.",
        false,
        (int32_t)50,
        int32_t(0)}},
      {
        "cachePrepStmts", {"cachePrepStmts",
        "0.9.1",
        "enable/disable prepare Statement cache, default true.",
        false,
        false}},
      {
        "prepStmtCacheSize", {"prepStmtCacheSize",
        "0.9.1",
        "This sets the number of prepared statements that the "
        "driver will cache per connection if \"cachePrepStmts\" is enabled.",
        false,
        (int32_t)250,
        int32_t(0)}},
      {
        "prepStmtCacheSqlLimit", {"prepStmtCacheSqlLimit",
        "0.9.1",
        "This is the maximum length of a prepared SQL"
        " statement that the driver will cache  if \"cachePrepStmts\" is enabled.",
        false,
        (int32_t)2048,
        int32_t(0)}},
      {
        "assureReadOnly", {"assureReadOnly",
        "0.9.1",
        "If true, in high availability, and switching to a "
        "read-only host, assure that this host is in read-only mode by setting the session to read-only.",
        false,
        false}},
      {
        "useLegacyDatetimeCode", {"useLegacyDatetimeCode",
        "0.9.1",
        "if true (default) store date/timestamps "
        "according to client time zone.\n"
        "if false, store all date/timestamps in DB according to server time zone, and time information (that is a"
        " time difference), doesn't take\n"
        "timezone in account.",
        false,
        true}},
      {
        "maximizeMysqlCompatibility", {"maximizeMysqlCompatibility",
        "0.9.1",
        "maximize MySQL compatibility.\n"
        "when using jdbc setDate(), will store date in client timezone, not in server timezone when "
        "useLegacyDatetimeCode = false.\n"
        "default to false.",
        false,
        false}},
      {
        "useServerPrepStmts", {"useServerPrepStmts",
        "0.9.1",
        "useServerPrepStmts must prepared statements be"
        " prepared on server side, or just faked on client side.\n"
        "     * if rewriteBatchedStatements is set to true, this options will be set to false.",
        false,
        false}},
/******************************* Tls parameters *******************************/
      {
        "useTls", {"useTls",
        "0.9.1",
        "Force SSL on connection. (legacy aliases \"useSsl\", \"useSSL\")",
        false,
        false} },
      {
        "tlsKey", {"tlsKey",
        "0.9.2",
        "File path to a private key file to use for TLS. (alias sslKey is also supported)\n"
        "It is required to use the absolute path, not a relative path.",
        false}},
      {
        "tlsCert", {"tlsCert",
        "0.9.2",
        "Path to the X509 certificate file"
        ".\n"
        "(alias sslCert).",
        false}},
      {
        "tlsCA", {"tlsCA",
        "0.9.2",
        "A path to a PEM file that should contain one or more X509 certificates for trusted Certificate Authorities (CAs)."
        "",
        false}},
      {
        "tlsCAPath", {"tlsCAPath",
        "0.9.2",
        "A path to a directory that contains one or more PEM files that should each contain one X509 certificate for "
        "a trusted Certificate Authority (CA) to use "
        "",
        false}},
      {
        "keyPassword", {"keyPassword",
        "0.9.2",
        "Password for the private key in client certificate tlsCA. (only "
        "needed if private key password differ from tlsCA password).",
        false}},
      {
        "enabledTlsProtocolSuites", {"enabledTlsProtocolSuites",
        "0.9.2",
        "Force TLS/SSL protocol to a specific set of TLS "
        "versions (comma separated list). \n"
        "Example : \"TLSv1, TLSv1.1, TLSv1.2\"\n"
        "(Aliases: \"enabledSSLProtocolSuites\" \"enabledSslProtocolSuites\")",
        false}},
      {
        "enabledTlsCipherSuites", {"enabledTlsCipherSuites",
        "0.9.2",
        "Force TLS/SSL cipher (comma separated list).\n"
        "Example : \"TLS_DHE_RSA_WITH_AES_256_GCM_SHA384, TLS_DHE_DSS_WITH_AES_256_GCM_SHA384\"",
        false}
      },
      {
        "tlsCRL", {"tlsCRL",
        "0.9.2",
        "path to a PEM file that should contain one or more revoked X509 certificates",
        false,
        ""}
      },
      {
        "tlsCRLPath", {"tlsCRLPath",
        "0.9.2",
        "A path to a directory that contains one or more PEM files that should each contain one revoked X509 certificate.\n"
        "The directory specified by this option needs to be run through the openssl rehash command.\n"
        " ",
        false,
        ""}
      },
      {
        "serverRsaPublicKeyFile", {"serverRsaPublicKeyFile",
        "0.9.2",
        "the name of the file which contains the RSA public key of the database server. The format of this file must be in PEM format.\n"
        "This option is used by the caching_sha2_password client authentication plugin.",
        false,
        ""} },
      {
        "tlsPeerFP", {"tlsPeerFP",
        "0.9.2",
        "The SHA1 fingerprint of a server certificate for validation during the TLS handshake.",
        false,
        ""}
      },
      {
        "tlsPeerFPList", {"tlsPeerFPList", "0.9.2",
        "A file containing one or more SHA1 fingerprints of server certificates for validation during the TLS handshake.", false, ""}
      },
/**************************** Tls parameters - end ******************************/
      {
        "continueBatchOnError", {"continueBatchOnError",
        "0.9.1",
        "When executing batch queries, must batch continue on error.",
        false,
        true}},
      {
        "jdbcCompliantTruncation", {"jdbcCompliantTruncation",
        "0.9.1",
        "Truncation error (\"Data truncated for"
        " column '%' at row %\", \"Out of range value for column '%' at row %\") will be thrown as error, and not as warning.",
        false,
        true}},
      {
        "cacheCallableStmts", {"cacheCallableStmts",
        "0.9.1",
        "enable/disable callable Statement cache, default true.",
        false,
        false}},
      {
        "callableStmtCacheSize", {"callableStmtCacheSize",
        "0.9.1",
        "This sets the number of callable statements "
        "that the driver will cache per VM if \"cacheCallableStmts\" is enabled.",
        false,
        (int32_t)150,
        int32_t(0)}},
      {
        "connectionAttributes", {"connectionAttributes",
        "0.9.1",
        "When performance_schema is active, permit to send server "
        "some client information in a key;value pair format "
        "(example: connectionAttributes=key1:value1,key2,value2).\n"
        "Those informations can be retrieved on server within tables performance_schema.session_connect_attrs "
        "and performance_schema.session_account_connect_attrs.\n"
        "This can permit from server an identification of client/application",
        false}},
      {
        "useBatchMultiSend", {"useBatchMultiSend",
        "0.9.1",
        "*Not compatible with aurora*\n"
        "Driver will can send queries by batch. \n"
        "If set to false, queries are sent one by one, waiting for the result before sending the next one. \n"
        "If set to true, queries will be sent by batch corresponding to the useBatchMultiSendNumber option value"
        " (default 100) or according to the max_allowed_packet server variable if the packet size does not permit"
        " sending as many queries. Results will be read later, avoiding a lot of network latency when the client"
        " and server aren't on the same host. \n"
        "\n"
        "This option is mainly effective when the client is distant from the server.",
        false,
        false}},
      {
        "useBatchMultiSendNumber", {"useBatchMultiSendNumber",
        "0.9.1",
        "When option useBatchMultiSend is active,"
        " indicate the maximum query send in a row before reading results.",
        false,
        (int32_t)100,
        int32_t(1)}},
      {
        "log", {"log",
        "0.9.1",
        "Enable log information. \n"
        "require Slf4j version > 1.4 dependency.\n"
        "Log level correspond to Slf4j logging implementation",
        false,
        false}},
      {"profileSql", {"profileSql", "0.9.1", "log query execution time.", false, false}},
      {"maxQuerySizeToLog", {"maxQuerySizeToLog", "0.9.1", "Max query log size.", false, 1024, 0}},
      {
        "slowQueryThresholdNanos", {"slowQueryThresholdNanos",
        "0.9.1",
        "Will log query with execution time superior to this value (if defined )",
        0LL,
        false,
        0LL}},
      {
        "passwordCharacterEncoding", {"passwordCharacterEncoding",
        "0.9.1",
        "Indicate password encoding charset. If not set, driver use platform's default charset.",
        false}
      },
      {
        "useCharacterEncoding", {"useCharacterEncoding",
        "1.0.1",
        "String data encoding charset. The driver will assume, that all input strings are encoded using this"
        "character set, and will use it for communication with the server",
        false,
        "utf8mb4"}
      },
      {
        "usePipelineAuth", {"usePipelineAuth",
        "0.9.1",
        "*Not compatible with aurora*\n"
        "During connection, different queries are executed. When option is active those queries are send using"
        " pipeline (all queries are send, then only all results are reads), permitting faster connection "
        "creation",
        false,
        false}},
      {
        "enablePacketDebug", {"enablePacketDebug",
        "0.9.1",
        "Driver will save the last 16 MariaDB packet "
        "exchanges (limited to first 1000 bytes). Hexadecimal value of those packets will be added to stacktrace"
        " when an IOException occur.\n"
        "This option has no impact on performance but driver will then take 16kb more memory.",
        false,
        false}},
      {
        "disableSslHostnameVerification", {"disableSslHostnameVerification",
        "0.9.1",
        "When using ssl, the driver "
        "checks the hostname against the server's identity as presented in the server's certificate (checking "
        "alternative names or the certificate CN) to prevent man-in-the-middle attacks. This option permits "
        "deactivating this validation. Hostname verification is disabled when the trustServerCertificate "
        "option is set",
        false,
        false}},
      {
        "useBulkStmts", {"useBulkStmts",
        "1.0.2",
        "Use dedicated COM_STMT_BULK_EXECUTE protocol for executeBatch if "
        "possible(batch without Statement::RETURN_GENERATED_KEYS and streams). Can be significanlty faster."
        "(works only with server MariaDB >= 10.2.7)",
        false,
        false}},
      {
        "autocommit", {"autocommit",
        "0.9.1",
        "Set default autocommit value on connection initialization",
        false,
        true}},
      {
        "pool", {"pool",
        "1.1.1",
        "Use pool.",
        false,
        false}},
      {
        "poolName", {"poolName",
        "0.9.1",
        "Pool name that permits identifying threads. default: auto-generated as "
        "MariaDb-pool-<pool-index>",
        false}},
      {
        "maxPoolSize", {"maxPoolSize",
        "1.1.1",
        "The maximum number of physical connections that the pool should contain.",
        false,
        (int32_t)8,
        int32_t(1) }},
      {
        "minPoolSize", {"minPoolSize",
        "1.1.1",
        "When connections are removed due to not being used for "
        "longer than than \"maxIdleTime\", connections are closed and removed from the pool. \"minPoolSize\" "
        "indicates the number of physical connections the pool should keep available at all times. Should be less"
        " or equal to maxPoolSize.",
        false,
        (int32_t)0,
        int32_t(0)}},
      {
        "maxIdleTime", {"maxIdleTime",
        "1.1.1",
        "The maximum amount of time in seconds"
        " that a connection can stay in the pool when not used. This value must always be below @wait_timeout"
        " value - 45s \n"
        "Default: 600 in seconds (=10 minutes), minimum value is 60 seconds",
        false,
        (int32_t)600,
        Options::MIN_VALUE__MAX_IDLE_TIME}},
      {
        "poolValidMinDelay", {"poolValidMinDelay",
        "1.1.1",
        "When the pool is requested for a connection, it will validate the connection state. "
        "\"poolValidMinDelay\" allows to disable this validation if the connection has been "
        "used recently, avoiding useless verifications in case of frequent reuse of connections. "
        "0 means validation is done each time the connection is requested.",
        false,
        (int32_t)1000,
        int32_t(0)}},
      {
        "staticGlobal", {"staticGlobal",
        "0.9.1",
        "Indicates the values of the global variables "
        "max_allowed_packet, wait_timeout, autocommit, auto_increment_increment, time_zone, system_time_zone and"
        " tx_isolation) won't be changed, permitting the pool to create new connections faster.",
        false,
        false}},
      {
        "useResetConnection", {"useResetConnection",
        "1.1.1",
        "When a connection is closed() "
        "(given back to pool), the pool resets the connection state. Setting this option, the prepare command "
        "will be deleted, session variables changed will be reset, and user variables will be destroyed when the"
        " server permits it (>= MariaDB 10.2.4, >= MySQL 5.7.3), permitting saving memory on the server if the "
        "application make extensive use of variables. Must not be used with the useServerPrepStmts option",
        false,
        true }},
      {
        "allowMasterDownConnection", {"allowMasterDownConnection",
        "0.9.1",
        "When using master/slave configuration, "
        "permit to create connection when master is down. If no master is up, default connection is then a slave "
        "and Connection.isReadOnly() will then return true.",
        false,
        false}},
      {
        "galeraAllowedState", {"galeraAllowedState",
        "0.9.1",
        "Usually, Connection.isValid just send an empty packet to "
        "server, and server send a small response to ensure connectivity. When this option is set, connector will"
        " ensure Galera server state \"wsrep_local_state\" correspond to allowed values (separated by comma). "
        "Example \"4,5\", recommended is \"4\". see galera state to know more.",
        false}},
      {
        "useAffectedRows", {"useAffectedRows",
        "0.9.1",
        "If false (default), use \"found rows\" for the row "
        "count of statements. This corresponds to the JDBC standard.\n"
        "If true, use \"affected rows\" for the row count.\n"
        "This changes the behavior of, for example, UPDATE... ON DUPLICATE KEY statements.",
        false,
        false}},
      {
        "includeInnodbStatusInDeadlockExceptions", {"includeInnodbStatusInDeadlockExceptions",
        "0.9.1",
        "add \"SHOW ENGINE INNODB STATUS\" result to exception trace when having a deadlock exception",
        false,
        false}},
      {
        "includeThreadDumpInDeadlockExceptions", {"includeThreadDumpInDeadlockExceptions",
        "0.9.1",
        "add thread dump to exception trace when having a deadlock exception",
        false,
        false}},
      {
        "useReadAheadInput", {"useReadAheadInput",
        "0.9.1",
        "use a buffered inputSteam that read socket available data",
        false,
        true}
      },
      {
        "servicePrincipalName", {"servicePrincipalName",
        "0.9.1",
        "when using GSSAPI authentication, SPN (Service Principal Name) use the server SPN information. When set, "
        "connector will use this value, ignoring server information",
        false,
        ""}
      },

      {
        "defaultFetchSize", {"defaultFetchSize",
        "0.9.1",
        "The driver will call setFetchSize(n) with this value on all newly-created Statements",
        false,
        int32_t(0),
        int32_t(0) }},
      {
        "useMysqlMetadata", {"useMysqlMetadata",
        "0.9.1",
        "force DatabaseMetadata.getDatabaseProductName() "
        "to return \"MySQL\" as database, not real database type",
        false,
        false}
      },
      {
        "blankTableNameMeta", {"blankTableNameMeta",
        "0.9.1",
        "Resultset metadata getTableName always return blank. "
        "This option is mainly for ORACLE db compatibility",
        false,
        false}},
      {
        "credentialType", {"credentialType",
        "0.9.1",
        "Default authentication client-side plugin to use",
        false,
        ""}
      }
    };

//---------------------------------------- Aliases ------------------------------------------------------------------------------------
    bool addAliases(std::map<std::string, DefaultOptions*>& completeOptionsMap) {
      for (auto& defaultOption : OptionsMap) {
        completeOptionsMap.emplace(defaultOption.first, &defaultOption.second);
      }

      completeOptionsMap.emplace("userName",                   &OptionsMap["user"]);
      completeOptionsMap.emplace("socket",                     &OptionsMap["localSocket"]);
      completeOptionsMap.emplace("createDB",                   &OptionsMap["createDatabaseIfNotExist"]);
      completeOptionsMap.emplace("useSSL",                     &OptionsMap["useTls"]);
      completeOptionsMap.emplace("useSsl",                     &OptionsMap["useTls"]);
      completeOptionsMap.emplace("profileSQL",                 &OptionsMap["profileSql"]);
      completeOptionsMap.emplace("enabledSSLCipherSuites",     &OptionsMap["enabledTlsCipherSuites"]);
      completeOptionsMap.emplace("enabledSslCipherSuites",     &OptionsMap["enabledTlsCipherSuites"]);
      completeOptionsMap.emplace("MARIADB_OPT_TLS_PASSPHRASE", &OptionsMap["keyPassword"]);
      completeOptionsMap.emplace("sslCert",                    &OptionsMap["tlsCert"]);
      completeOptionsMap.emplace("sslCA",                      &OptionsMap["tlsCA"]);
      completeOptionsMap.emplace("tlsCa",                      &OptionsMap["tlsCA"]);
      completeOptionsMap.emplace("tlsCaPath",                  &OptionsMap["tlsCAPath"]);
      completeOptionsMap.emplace("sslCAPath",                  &OptionsMap["tlsCAPath"]);
      completeOptionsMap.emplace("enabledSslProtocolSuites",   &OptionsMap["enabledTlsProtocolSuites"]);
      completeOptionsMap.emplace("enabledSSLProtocolSuites",   &OptionsMap["enabledTlsProtocolSuites"]);
      completeOptionsMap.emplace("sslCRL",                     &OptionsMap["tlsCRL"]);
      completeOptionsMap.emplace("tlsCrl",                     &OptionsMap["tlsCRL"]);
      completeOptionsMap.emplace("sslCRLPath",                 &OptionsMap["tlsCRLPath"]);
      completeOptionsMap.emplace("tlsCrlPath",                 &OptionsMap["tlsCRLPath"]);
      completeOptionsMap.emplace("tlsPeerFp",                  &OptionsMap["tlsPeerFP"]);
      completeOptionsMap.emplace("tlsPeerFpList",              &OptionsMap["tlsPeerFPList"]);
      completeOptionsMap.emplace("MARIADB_OPT_TLS_PEER_FP",    &OptionsMap["tlsPeerFP"]);
      completeOptionsMap.emplace("MARIADB_OPT_SSL_FP_LIST",    &OptionsMap["tlsPeerFPList"]);
      completeOptionsMap.emplace("rsaKey",                     &OptionsMap["serverRsaPublicKeyFile"]);
      completeOptionsMap.emplace("OPT_READ_TIMEOUT",           &OptionsMap["socketTimeout"]);
      completeOptionsMap.emplace("OPT_RECONNECT",              &OptionsMap["autoReconnect"]);
      completeOptionsMap.emplace("CLIENT_COMPRESS",            &OptionsMap["useCompression"]);
      completeOptionsMap.emplace("OPT_SET_CHARSET_NAME",       &OptionsMap["useCharacterEncoding"]);
      completeOptionsMap.emplace("useCharset",                 &OptionsMap["useCharacterEncoding"]);
      completeOptionsMap.emplace("defaultAuth",                &OptionsMap["credentialType"]);
      return true;
    }

    std::map<std::string, DefaultOptions*> DefaultOptions::OPTIONS_MAP;
    static bool aliasesAdded= addAliases(DefaultOptions::OPTIONS_MAP);
//-------------------------------------------------------------------------------------------------------------------------------------
    DefaultOptions::DefaultOptions(const char * optionName, const char * implementationVersion, const char* description, bool required)
      : optionName(optionName),  description(description), required(required), defaultValue(""), minValue (), maxValue()
    {
    }

    DefaultOptions::DefaultOptions(const char * optionName, const char * implementationVersion, const char * description,
        bool required, const char * defaultValue):
      optionName(optionName),
      description(description),
      required(required),
      defaultValue(defaultValue),
      minValue(),
      maxValue()
    {
    }

    DefaultOptions::DefaultOptions(const char * optionName, const char * implementationVersion, const char * description,
        bool required, bool defaultValue) :
      optionName(optionName),
      description(description),
      required(required),
      defaultValue(defaultValue),
      minValue(),
      maxValue()
    {
    }

    DefaultOptions::DefaultOptions(const char* optionName, const char* implementationVersion, const char* description,
        bool required, int32_t defaultValue, int32_t minValue) :
      optionName(optionName),
      description(description),
      required(required),
      defaultValue(defaultValue),
      minValue(minValue),
      maxValue(INT32_MAX)
    {
    }

    DefaultOptions::DefaultOptions(const char* optionName, const char* implementationVersion, const char* description,
        int64_t defaultValue, bool required, int64_t minValue) :
      optionName(optionName),
      description(description),
      required(required),
      defaultValue(defaultValue),
      minValue(minValue),
      maxValue(INT64_MAX)
    {
    }

#ifdef WE_NEED_INT_ARRAY_DEFAULT_VALUE
    DefaultOptions::DefaultOptions(const SQLString& optionName, int32_t* defaultValue, int32_t minValue,
      const SQLString& implementationVersion, SQLString& description, bool required) :
      optionName(optionName),
      defaultValue(defaultValue),
      minValue(minValue),
      maxValue(INT32_MAX),
      description(description),
      required(required)
    {
    }
#endif

    Shared::Options DefaultOptions::defaultValues(HaMode haMode)
    {
      Properties properties;
      return parse(haMode, emptyStr, properties);
    }
    /**
     * Generate an Options object with default value corresponding to High Availability mode.
     *
     * @param haMode current high Availability mode
     * @param pool is for pool
     * @return Options object initialized
     */
    Shared::Options DefaultOptions::defaultValues(HaMode haMode, bool pool)
    {
      Properties properties;
      properties.insert({ "pool", pool ? "true" : "false" });
      Shared::Options options= parse(haMode, emptyStr, properties);
      postOptionProcess(options, nullptr);
      return options;
    }

    /**
     * Parse additional properties.
     *
     * @param haMode current haMode.
     * @param urlParameters options defined in url
     * @param options initial options
     */
    void DefaultOptions::parse(const enum HaMode haMode, const SQLString& urlParameters, Shared::Options options)
    {
      Properties prop;
      parse(haMode, urlParameters, prop, options);
      postOptionProcess(options, nullptr);
    }


    Shared::Options DefaultOptions::parse( const enum HaMode haMode,const SQLString& urlParameters, Properties& properties) {
      Shared::Options options= parse(haMode, urlParameters, properties, nullptr);
      postOptionProcess(options, nullptr);
      return options;
    }

    /**
     * Parse additional properties .
     *
     * @param haMode current haMode.
     * @param urlParameters options defined in url
     * @param properties options defined by properties
     * @param options initial options
     * @return options
     */
    Shared::Options DefaultOptions::parse(enum HaMode haMode, const SQLString& urlParameters, Properties& properties,
      Shared::Options options)
    {
      if (/*urlParameters != nullptr &&*/ !urlParameters.empty())
      {
        Tokens parameters= split(urlParameters, "&");

        for (SQLString& parameter : *parameters)
        {
          size_t pos= parameter.find_first_of('=');
          if (pos == std::string::npos){
            if (properties.find(parameter) == properties.end()){
              properties.insert({ parameter, emptyStr });
            }
          }else {
            if (properties.find(parameter.substr(0,pos)) == properties.end()){
              properties.insert({ parameter.substr(0,pos),parameter.substr(pos +1) });
            }
          }
        }
      }
      return parse(haMode, properties, options);
    }


    Shared::Options DefaultOptions::parse(enum HaMode haMode, const Properties& properties, Shared::Options paramOptions)
    {
      Options &options= *paramOptions;

      try
      {
        for (auto& it : properties)
        {
          const std::string& key= StringImp::get(it.first);
          SQLString propertyValue(it.second);

          auto cit= OPTIONS_MAP.find(key);

          if (cit != OPTIONS_MAP.end()/* && !propertyValue.empty()*/)
          {
            DefaultOptions *o= cit->second;
            const ClassField<Options> field= Options::getField(o->optionName);

            if (o->objType() == Value::VSTRING ){
              field.set(options, propertyValue);
            }
            else if (o->objType() == Value::VBOOL)
            {
              if (propertyValue.toLowerCase().length() == 0
                || propertyValue.compare("1") == 0
                || propertyValue.compare("true") == 0)
              {
                field.set(options, true);
              }
              else if (propertyValue.compare("0") == 0
                || propertyValue.compare("false") == 0)
              {
                field.set(options, false);
              }
              else
              {
                throw IllegalArgumentException(
                    "Optional parameter "
                    +o->optionName
                    +" must be bool (true/false or 0/1) was \""
                    +propertyValue
                    +"\"");
              }
            }
            else if (o->objType() == Value::VINT32)
            {
              try {
                int32_t value= std::stoi(StringImp::get(propertyValue));

                assert(!o->minValue.empty());
                assert(!o->maxValue.empty());
                int32_t minValue= o->minValue;
                int32_t maxValue= o->maxValue;

                if (value < minValue
                    || (maxValue != INT32_MAX && value > maxValue))
                {
                  throw IllegalArgumentException(
                      "Optional parameter "
                      +o->optionName
                      +" must be greater or equal to "
                      + o->minValue.toString()
                      + (maxValue != INT32_MAX
                        ?" and smaller than " + o->maxValue.toString()
                        :" ")
                      +", was \""
                      +propertyValue
                      +"\"");
                }

                field.set(options, value);
              }
              catch (std::invalid_argument&)
              {
                throw IllegalArgumentException(
                    "Optional parameter "
                    + o->optionName
                    + " must be Integer, was \""
                    + propertyValue
                    + "\"");
              }
            }
            else if (o->objType() == Value::VINT64)
            {
              try {
                int64_t value= std::stoll(StringImp::get(propertyValue));

                assert(!o->minValue.empty());
                assert(!o->maxValue.empty());

                int64_t minValue= o->minValue;
                int64_t maxValue= o->maxValue;

                if (value < minValue
                    || (maxValue != INT64_MAX && value > maxValue))
                {
                  throw IllegalArgumentException(
                      "Optional parameter "
                      + o->optionName
                      + " must be greater or equal to "
                      + o->minValue.toString()
                      + (maxValue !=INT64_MAX
                        ? " and smaller than " + o->maxValue.toString()
                        : SQLString(" "))
                      + ", was \""
                      + propertyValue
                      + "\"");
                }
                field.set(options,value);
              }
              catch (std::invalid_argument&)
              {
                throw IllegalArgumentException(
                    "Optional parameter "
                    +o->optionName
                    +" must be Long, was \""
                    +propertyValue
                    +"\"");
              }
            }
          }
          else
          {
            options.nonMappedOptions.emplace(it.first, it.second);
          }
        }
        if (options.socketTimeout == 0)
        {
          options.socketTimeout= SocketTimeoutDefault[haMode];//OptionsMap["socketTimeout"].defaultValue;
        }
      }
      catch (std::exception &e)
      {
        throw IllegalArgumentException(SQLString("An exception occurred : ") + e.what());
      }

      return paramOptions;
    }

    /**
     * Option initialisation end : set option value to a coherent state.
     *
     * @param options options
     * @param credentialPlugin credential plugin
     */
    void DefaultOptions::postOptionProcess(const Shared::Options options, CredentialPlugin* credentialPlugin)
    {
      if (options->rewriteBatchedStatements){
        options->useServerPrepStmts= false;
      }
      if (!options->pipe.empty()){
        options->useBatchMultiSend= false;
        options->usePipelineAuth= false;
      }

      if (options->pool) {
        options->minPoolSize=
          options->minPoolSize == 0
          ? options->maxPoolSize
          : std::min(options->minPoolSize, options->maxPoolSize);

        throw SQLFeatureNotImplementedException("This connector version does not have pool support");
      }

      if (options->cacheCallableStmts || options->cachePrepStmts) {
        throw SQLFeatureNotImplementedException("Callable/Prepared statement caches are not supported yet");
      }

      if (options->defaultFetchSize != 0){
        throw SQLFeatureNotImplementedException("ResultSet streaming is not supported in this version");
      }

      if (credentialPlugin && credentialPlugin->mustUseSsl())
      {
        options->useTls= true;
      }

      if (options->usePipelineAuth) {
        throw SQLFeatureNotSupportedException("Pipe identification is not supported yet");
      }

      if (options->useCharacterEncoding.compare("utf8") == 0) {
        options->useCharacterEncoding = "utf8mb4";
      }
    }

    /**
     * Generate parameter String equivalent to options.
     *
     * @param options options
     * @param haMode high availability Mode
     * @param sb String builder
     */
    void DefaultOptions::propertyString(const Shared::Options options,const enum HaMode haMode, SQLString& sb)
    {
      try
      {
        bool first= true;
        for (auto  it : OptionsMap)
        {
          DefaultOptions& o= it.second;
          const ClassField<Options> field= Options::getField(o.optionName);
          const Value& value= field.get(*options);

          if (!value.empty() && !value.equals(o.defaultValue))
          {
            if (first)
            {
              first= false;
              sb.append('?');
            }
            else
            {
              sb.append('&');
            }
            sb.append(o.optionName).append('=');

            /* Basically, we could just cast the value to SQLString for all types */
            if (o.objType() == Value::VSTRING)
            {
              sb.append(static_cast<const char*>(value));
            }
            else if (o.objType() == Value::VBOOL)
            {
              sb.append(value.toString());
            }
            else if (o.objType() == Value::VINT32 || o.objType() == Value::VINT64)
            {
              sb.append(static_cast<const char*>(value));
            }
          }
        }
      }
      catch (std::exception& n)
      {
        throw n;
      }
    }


  SQLString DefaultOptions::getOptionName(){
    return optionName;
  }


  SQLString DefaultOptions::getDescription(){
    return description;
  }


  bool DefaultOptions::isRequired(){
    return required;
  }
  Value::valueType DefaultOptions::objType() const
  {
    return defaultValue.objType();
  }
}
}
