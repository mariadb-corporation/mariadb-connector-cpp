# ************************************************************************************
#   Copyright (C) 2013,2023 MariaDB Corporation AB
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Library General Public License for more details.
#
#   You should have received a copy of the GNU Library General Public
#   License along with this library; if not see <http://www.gnu.org/licenses>
#   or write to the Free Software Foundation, Inc.,
#   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
# *************************************************************************************/

IF(APPLE)
  # productbuild packaging stuff 
  CMAKE_MINIMUM_REQUIRED(VERSION 3.23)
ELSE()
  CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
ENDIF()
CMAKE_POLICY(SET CMP0048 NEW)

PROJECT(mariadb_connector_cpp
        VERSION 1.1.4
#        DESCRIPTION "MariaDB Connector/C++"
        LANGUAGES CXX C)

IF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
  MESSAGE(FATAL_ERROR "GCC version >=4.8 is required")
ENDIF()

SET(MACPP_VERSION_QUALITY "ga") #Empty also means GA
SET(MACPP_VERSION "1.01.0004")
SET(MARIADB_DEFAULT_PLUGINS_SUBDIR "plugin")

# For C/C
IF(CMAKE_VERSION VERSION_LESS "3.1")
  IF(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-std=gnu99 ${CMAKE_C_FLAGS}")
  ENDIF()
ELSE()
  SET(CMAKE_C_STANDARD 99)
ENDIF()

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(PRODUCT_NAME "MariaDB C++ Connector")
SET(PRODUCT_MANUFACTURER "MariaDB")
SET(PRODUCT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
SET(PRODUCT_SERIES "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

GET_PROPERTY(MAODBC_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
# We don't need RC for what we need MAODBC_LANGUAGES for
LIST(REMOVE_ITEM MAODBC_LANGUAGES "RC")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

SET(LIBRARY_NAME "mariadbcpp")
SET(STATIC_LIBRARY_NAME "${LIBRARY_NAME}-static")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/Version.h.in
               ${CMAKE_SOURCE_DIR}/src/Version.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/maconncpp.rc.in
               ${CMAKE_SOURCE_DIR}/src/maconncpp.rc)
INCLUDE(SearchLibrary)
INCLUDE(SetValueMacro)


SET (MACPP_SOURCES src/MariaDbDriver.cpp
                   src/DriverManager.cpp
                   src/UrlParser.cpp
                   src/MariaDbDatabaseMetaData.cpp
                   src/HostAddress.cpp
                   src/Consts.cpp
                   src/SQLString.cpp
                   src/MariaDbConnection.cpp
                   src/MariaDbStatement.cpp
                   src/MariaDBException.cpp
                   src/MariaDBWarning.cpp
                   src/Identifier.cpp
                   src/MariaDbSavepoint.cpp
                   src/SqlStates.cpp
                   src/Results.cpp

                   src/ColumnType.cpp

                   src/ColumnDefinition.cpp
                   src/protocol/MasterProtocol.cpp

                   src/protocol/capi/QueryProtocol.cpp
                   src/protocol/capi/ConnectProtocol.cpp
                   src/com/capi/ColumnDefinitionCapi.cpp

                   src/cache/CallableStatementCache.cpp
                   src/cache/CallableStatementCacheKey.cpp

                   src/util/Value.cpp
                   src/util/Utils.cpp
                   src/util/String.cpp

                   src/logger/NoLogger.cpp
                   src/logger/SimpleLogger.cpp
                   src/logger/LoggerFactory.cpp
                   src/logger/ProtocolLoggingProxy.cpp

                   src/parameters/ParameterHolder.cpp

                   src/options/Options.cpp
                   src/options/DefaultOptions.cpp

                   src/pool/GlobalStateInfo.cpp
                   src/pool/Pools.cpp
                   src/pool/Pool.cpp
                   src/pool/MariaDbThreadFactory.cpp
                   src/pool/MariaDbInnerPoolConnection.cpp
                   src/pool/ConnectionEventListener.cpp
                   src/pool/ThreadPoolExecutor.cpp

                   src/failover/FailoverProxy.cpp

                   src/credential/CredentialPluginLoader.cpp

                   src/SelectResultSet.cpp
                   src/com/capi/SelectResultSetCapi.cpp
                   src/com/capi/SelectResultSetBin.cpp
                   #src/com/SelectResultSetPacket.cpp
                   #src/com/ColumnDefinitionPacket.cpp

                   src/com/ColumnNameMap.cpp

                   src/io/StandardPacketInputStream.cpp

                   src/Charset.cpp
                   src/ClientSidePreparedStatement.cpp
                   src/BasePrepareStatement.cpp
                   src/MariaDbFunctionStatement.cpp
                   src/MariaDbProcedureStatement.cpp
                   src/ServerSidePreparedStatement.cpp

                   src/parameters/BigDecimalParameter.cpp
                   src/parameters/BooleanParameter.cpp
                   src/parameters/ByteArrayParameter.cpp
                   src/parameters/ByteParameter.cpp
                   src/parameters/DateParameter.cpp
                   src/parameters/DefaultParameter.cpp
                   src/parameters/DoubleParameter.cpp
                   src/parameters/FloatParameter.cpp
                   src/parameters/IntParameter.cpp
                   #src/parameters/LocalTimeParameter.cpp
                   src/parameters/LongParameter.cpp
                   src/parameters/ULongParameter.cpp
                   src/parameters/NullParameter.cpp
                   #src/parameters/OffsetTimeParameter.cpp
                   src/parameters/ParameterHolder.cpp
                   src/parameters/ReaderParameter.cpp
                   #src/parameters/SerializableParameter.cpp
                   src/parameters/ShortParameter.cpp
                   src/parameters/StreamParameter.cpp
                   src/parameters/TimeParameter.cpp
                   src/parameters/TimestampParameter.cpp
                   #src/parameters/ZonedDateTimeParameter.cpp
                   src/parameters/StringParameter.cpp
                   src/MariaDbPoolConnection.cpp
                   src/MariaDbParameterMetaData.cpp
                   src/MariaDbResultSetMetaData.cpp
                   src/CallParameter.cpp
                   src/CallableParameterMetaData.cpp
                   src/com/Packet.cpp
                   src/credential/Credential.cpp
                   src/util/LogQueryTool.cpp
                   src/util/ClientPrepareResult.cpp
                   src/util/ServerPrepareResult.cpp
                   src/util/ServerPrepareStatementCache.cpp
                   src/com/CmdInformationSingle.cpp
                   src/com/CmdInformationBatch.cpp
                   src/com/CmdInformationMultiple.cpp
                   src/com/RowProtocol.cpp
                   src/protocol/capi/BinRowProtocolCapi.cpp
                   src/protocol/capi/TextRowProtocolCapi.cpp
                   src/ExceptionFactory.cpp
                   src/StringImp.cpp
                   src/CArray.cpp
                   src/SimpleParameterMetaData.cpp
                   src/Properties.cpp
                   "src/List.cpp"
                   src/MariaDbDataSource.cpp
                   ####CPP####
                   )

IF(WIN32)
  SET(MACPP_SOURCES ${MACPP_SOURCES}
                   src/Dll.c

                   src/MariaDBConnCpp.h
                   src/MariaDbDriver.h
                   src/UrlParser.h
                   src/MariaDbDatabaseMetaData.h
                   src/HostAddress.h
                   src/Version.h
                   src/Consts.h
                   src/MariaDbConnection.h
                   src/MariaDbStatement.h
                   src/MariaDBWarning.h
                   src/Protocol.h
                   src/Identifier.h
                   src/MariaDbSavepoint.h
                   src/SqlStates.h
                   src/Results.h
                   src/ColumnDefinition.h
                   src/MariaDbServerCapabilities.h

                   src/protocol/MasterProtocol.h

                   src/protocol/capi/QueryProtocol.h
                   src/protocol/capi/ConnectProtocol.h
                   src/com/capi/ColumnDefinitionCapi.h

                   src/cache/CallableStatementCache.h
                   src/cache/CallableStatementCacheKey.h

                   src/util/Value.h
                   src/util/ClassField.h
                   src/util/Utils.h
                   src/util/StateChange.h
                   src/util/String.h

                   src/logger/NoLogger.h
                   src/logger/SimpleLogger.h
                   src/logger/LoggerFactory.h
                   src/logger/Logger.h
                   src/logger/ProtocolLoggingProxy.h

                   src/parameters/ParameterHolder.h

                   src/options/Options.h
                   src/options/DefaultOptions.h
                   src/options/DriverPropertyInfo.h

                   src/io/PacketOutputStream.h
                   src/io/PacketInputStream.h
                   src/io/StandardPacketInputStream.h

                   src/credential/CredentialPlugin.h
                   src/credential/CredentialPluginLoader.h

                   src/pool/Pool.h
                   src/pool/ThreadPoolExecutor.h
                   src/pool/MariaDbThreadFactory.h
                   src/pool/MariaDbInnerPoolConnection.h
                   src/pool/ConnectionEventListener.h

                   src/failover/FailoverProxy.h

                   src/Listener.h

                   src/com/CmdInformation.h

                   "include/conncpp.hpp"
                   "include/conncpp/Connection.hpp"
                   "include/conncpp/Statement.hpp"
                   "include/conncpp/ResultSet.hpp"
                   "include/conncpp/PreparedStatement.hpp"
                   "include/conncpp/ParameterMetaData.hpp"
                   "include/conncpp/ResultSetMetaData.hpp"
                   "include/conncpp/DatabaseMetaData.hpp"
                   "include/conncpp/CallableStatement.hpp"
                   "include/conncpp/SQLString.hpp"
                   "include/conncpp/Warning.hpp"
                   "include/conncpp/Exception.hpp"
                   "include/conncpp/jdbccompat.hpp"
                   "include/conncpp/Driver.hpp"
                   "include/conncpp/DriverManager.hpp"
                   "include/conncpp/Types.hpp"
                   "include/conncpp/buildconf.hpp"
                   "include/conncpp/CArray.hpp"
                   "include/conncpp/Properties.hpp"
                   "include/conncpp/List.hpp"
                   "include/conncpp/PooledConnection.hpp"
                   "include/MariaDbDataSource.hpp"

                   src/SelectResultSet.h
                   src/com/capi/SelectResultSetCapi.h
                   src/com/capi/SelectResultSetBin.h
                   src/com/SelectResultSetPacket.h
                   src/ColumnType.h
                   src/com/ColumnDefinitionPacket.h
                   src/com/ColumnNameMap.h
                   src/Charset.h
                   src/ClientSidePreparedStatement.h
                   src/BasePrepareStatement.h
                   src/MariaDbFunctionStatement.h
                   src/MariaDbProcedureStatement.h
                   src/ServerSidePreparedStatement.h

                   src/parameters/BigDecimalParameter.h
                   src/parameters/BooleanParameter.h
                   src/parameters/ByteArrayParameter.h
                   src/parameters/ByteParameter.h
                   src/parameters/DateParameter.h
                   src/parameters/DefaultParameter.h
                   src/parameters/DoubleParameter.h
                   src/parameters/FloatParameter.h
                   src/parameters/IntParameter.h
                   #src/parameters/LocalTimeParameter.h
                   src/parameters/LongParameter.h
                   src/parameters/ULongParameter.h
                   src/parameters/NullParameter.h
                   #src/parameters/OffsetTimeParameter.h
                   src/parameters/ReaderParameter.h
                   #src/parameters/SerializableParameter.h
                   src/parameters/ShortParameter.h
                   src/parameters/StreamParameter.h
                   src/parameters/TimeParameter.h
                   src/parameters/TimestampParameter.h
                   #src/parameters/ZonedDateTimeParameter.h
                   src/parameters/StringParameter.h

                   src/Parameters.h
                   src/MariaDbPoolConnection.h
                   src/MariaDbParameterMetaData.h
                   src/CallParameter.h
                   src/CallableParameterMetaData.h
                   src/MariaDbResultSetMetaData.h
                   src/com/Packet.h
                   src/util/ServerStatus.h
                   src/credential/Credential.h
                   src/util/LogQueryTool.h
                   src/PrepareResult.h
                   src/util/ClientPrepareResult.h
                   src/util/ServerPrepareResult.h
                   src/util/ServerPrepareStatementCache.h
                   src/util/BlockingQueue.h
                   src/com/CmdInformationSingle.h
                   src/com/CmdInformationBatch.h
                   src/com/CmdInformationMultiple.h
                   src/com/RowProtocol.h
                   src/protocol/capi/BinRowProtocolCapi.h
                   src/protocol/capi/TextRowProtocolCapi.h
                   src/ExceptionFactory.h
                   src/StringImp.h
                   src/CArrayImp.h
                   src/SimpleParameterMetaData.h
                   src/PropertiesImp.h
                   src/ListImp.h
                   src/MariaDbDataSourceInternal.h
                   ####H####
                       )
  SOURCE_GROUP(Logging REGULAR_EXPRESSION "/logger/")
  SOURCE_GROUP(PublicAPI REGULAR_EXPRESSION "include/")
  SOURCE_GROUP(Parameters REGULAR_EXPRESSION "src/parameters")
  SOURCE_GROUP(Options REGULAR_EXPRESSION "src/options")
  SOURCE_GROUP(IO REGULAR_EXPRESSION "src/io")
  SOURCE_GROUP(Credential REGULAR_EXPRESSION "src/credential")
  SOURCE_GROUP(Pool REGULAR_EXPRESSION "src/pool")
  SOURCE_GROUP(Utils REGULAR_EXPRESSION "src/util")
  SOURCE_GROUP(Caches REGULAR_EXPRESSION "src/cache")
  SOURCE_GROUP(Failover REGULAR_EXPRESSION "src/failover")
  SOURCE_GROUP(Commands REGULAR_EXPRESSION "src/com/")
  SOURCE_GROUP(Protocol REGULAR_EXPRESSION "src/protocol/")
  SOURCE_GROUP(C_API REGULAR_EXPRESSION "/capi/")

  SET(PLATFORM_DEPENDENCIES ws2_32)
ELSE()
  SEARCH_LIBRARY(LIB_MATH floor m)
  MESSAGE(STATUS "Found math lib: ${LIB_MATH}")
  SET(PLATFORM_DEPENDENCIES ${LIB_MATH})
ENDIF()

INCLUDE(check_compiler_flag)
IF(WITH_ASAN)
  IF(MSVC)
    MA_SET_COMPILER_FLAG("-fsanitize=address" DEBUG RELWITHDEBINFO)
    SET(MAODBC_LINKER_FLAGS ${MAODBC_LINKER_FLAGS} /INCREMENTAL:NO)
    MA_SET_LINKER_FLAG("${MAODBC_LINKER_FLAGS}" DEBUG RELWITHDEBINFO)
  ELSE()
    MY_CHECK_AND_SET_COMPILER_FLAG("-U_FORTIFY_SOURCE" DEBUG RELWITHDEBINFO)
    MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=address -fPIC" DEBUG RELWITHDEBINFO)

    SET(WITH_ASAN_OK 1)
    FOREACH(lang ${MAODBC_LANGUAGES})
      IF(NOT ${have_${lang}__fsanitize_address__fPIC})
        SET(WITH_ASAN_OK 0)
      ENDIF()
    ENDFOREACH()
    IF(WITH_ASAN_OK)
      OPTION(WITH_ASAN_SCOPE "Enable -fsanitize-address-use-after-scope" OFF)
      IF(WITH_ASAN_SCOPE)
        MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=address -fsanitize-address-use-after-scope" DEBUG RELWITHDEBINFO)
      ENDIF()
    ELSE()
      MESSAGE(FATAL_ERROR "Do not know how to enable address sanitizer")
    ENDIF()
  ENDIF()
ENDIF()

IF (WITH_UBSAN)
  MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=undefined -fno-sanitize=alignment -U_FORTIFY_SOURCE -DWITH_UBSAN" DEBUG RELWITHDEBINFO)
ENDIF()

IF (WITH_MSAN)
  MY_CHECK_AND_SET_COMPILER_FLAG("-fsanitize=memory -fsanitize-memory-track-origins -U_FORTIFY_SOURCE" DEBUG RELWITHDEBINFO)
ENDIF()

# This has to be before C/C's cmake run, or it will build with /MD
IF(WIN32)
  IF (MSVC)
    SET(CONFIG_TYPES "DEBUG" "RELEASE" "RELWITHDEBINFO" "MINSIZEREL")
    FOREACH(BUILD_TYPE ${CONFIG_TYPES})
      FOREACH(COMPILER ${MAODBC_LANGUAGES})
        SET(COMPILER_FLAGS "${CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}}")
        IF (NOT COMPILER_FLAGS STREQUAL "")
          IF(NOT WITH_ASAN)
            STRING(REPLACE "/MD" "/MT" COMPILER_FLAGS ${COMPILER_FLAGS})
            STRING(REPLACE "/Zi" "/ZI" COMPILER_FLAGS ${COMPILER_FLAGS})
          ENDIF(NOT WITH_ASAN)
          MESSAGE (STATUS "CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}= ${COMPILER_FLAGS}")
          SET(CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE} ${COMPILER_FLAGS} CACHE
               STRING "Overwritten by mariadb-cpp" FORCE)
        ENDIF()
      ENDFOREACH()
    ENDFOREACH()
  ENDIF()
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN)
  SET(INSTALL_PLUGINDIR "${MARIADB_DEFAULT_PLUGINS_SUBDIR}")
  SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} version.lib)
ENDIF()

### Build options, initial settings and platform defaults
INCLUDE("options_defaults")

### Setting installation paths - should go before C/C subproject sets its own. We need to have control over those
INCLUDE("install")
MESSAGE(STATUS "Libraries installation dir: ${INSTALL_LIBDIR}")
MESSAGE(STATUS "Authentication Plugins installation dir: ${INSTALL_PLUGINDIR}")

IF(NOT MINGW AND WIN32)
  MESSAGE(STATUS "Generate MSI package: ${WITH_MSI}")
  MESSAGE(STATUS "Sign MSI package: ${WITH_SIGNCODE}")
ENDIF()

# By now we have everything needed by tests. If we need to build them only - firing config now and exit
# There is "normal" tests config below
IF(BUILD_TESTS_ONLY)
  IF(WIN32)
    LINK_DIRECTORIES("$ENV{ProgramFiles}/MariaDB/MariaDB C++ Connector 64-bit")
  ELSE()
    MESSAGE(STATUS "Configurig Tests: tcp://${TEST_UID}@${TEST_SERVER}:${TEST_PORT}/${TEST_SCHEMA} socket: ${TEST_SOCKET}")
    SET(DRIVER_LIB_LOCATION "${libmariadb_prefix}/${INSTALL_LIBDIR}")
  ENDIF()
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/conncpp)
  # At some point use of String.cpp from the driver was added to tests. Need to verify if it's still needed/a good idea.
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
  ADD_SUBDIRECTORY(test)
  # Only means only
  RETURN()
ENDIF()

ADD_CUSTOM_TARGET(DEPENDENCIES_FOR_PACKAGE)
IF(NOT WITH_SSL)
  MESSAGE(STATUS "Setting default value for WITH_SSL for libmariadb build to ON")
  SET(CONC_WITH_SSL ON)
ENDIF()

### Including C/C subproject
IF(NOT USE_SYSTEM_INSTALLED_LIB)
  IF(EXISTS ${CMAKE_SOURCE_DIR}/libmariadb)
    IF(GIT_BUILD_SRCPKG)
      # We don't want conn/c (wrong) src pkg to be built.
      SET(GIT_BUILD_SRCPKG FALSE)
      SET(CONNCPP_GIT_BUILD_SRCPKG TRUE)
    ENDIF()
    MESSAGE(STATUS "Running C/C cmake scripts")
    INCLUDE(connector_c)
    INSTALL(FILES $<TARGET_FILE:libmariadb>
            DESTINATION ${INSTALL_LIBDIR}
 	          COMPONENT ConnectorC)
    MESSAGE(STATUS "Configuring to install libmariadb to ${INSTALL_LIBDIR}")

    SET(OWN_PLUGINS_LIST mysql_clear_password dialog client_ed25519 sha256_password caching_sha2_password)
    IF (PLUGINS_DYNAMIC)
      # The list from CC is visible for us
      SET(PLUGINS_LIST ${PLUGINS_DYNAMIC})
    ELSE()
      SET(PLUGINS_LIST ${OWN_PLUGINS_LIST})
    ENDIF()
    # Why is it here?
    IF(APPLE)
      SET(PRODUCT_IDENTIFIER "com.mariadb.connector.cpp")
      SET_TARGET_PROPERTIES(${PLUGINS_LIST}
                            PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                                       XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp -f"
                           )
      IF(WITH_SIGNCODE)
        SET_TARGET_PROPERTIES(${PLUGINS_LIST} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: ${DEVELOPER_ID}")
      ELSE()
        SET_TARGET_PROPERTIES(${PLUGINS_LIST} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
      ENDIF()
    ENDIF()

    FOREACH(CC_PLUGIN ${PLUGINS_LIST})
      IF(NOT PLUGINS_DYNAMIC OR "${PLUGIN_${CC_PLUGIN}_TYPE}" STREQUAL "MARIADB_CLIENT_PLUGIN_AUTH")
        MESSAGE(STATUS "Configuring to install ${CC_PLUGIN} to ${INSTALL_PLUGINDIR}")
        ADD_DEPENDENCIES(DEPENDENCIES_FOR_PACKAGE ${CC_PLUGIN})
        INSTALL(FILES $<TARGET_FILE:${CC_PLUGIN}>
                DESTINATION ${INSTALL_PLUGINDIR}
                COMPONENT Plugins)
      ENDIF()
    ENDFOREACH()
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libmariadb/include)
    INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/libmariadb/include)
  ELSE()
    SET(USE_SYSTEM_INSTALLED_LIB TRUE)
    IF (NOT WIN32)
      INCLUDE_DIRECTORIES(/usr/include/mariadb)
      # /usr/${INSTALL_LIBDIR}
    ENDIF()
    MESSAGE(STATUS "There is no Connector/C sub-project folder, linking against libmariadb installed on the system")
  ENDIF()
ENDIF()

IF(WITH_MSI AND WITH_SIGNCODE)
  IF(WIN32 AND NOT SIGN_OPTIONS)
    SET(SIGN_OPTIONS /a /t http://timestamp.verisign.com/scripts/timstamp.dll)
  ELSE()
    SEPARATE_ARGUMENTS(SIGN_OPTIONS)
  ENDIF()
  MARK_AS_ADVANCED(SIGN_OPTIONS)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/conncpp)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/class)
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
ENDIF()
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/maconncpp.def.in
               ${CMAKE_SOURCE_DIR}/src/maconncpp.def)

# Dynamic linking is default on non-Windows
IF(MARIADB_LINK_DYNAMIC)
  IF(USE_SYSTEM_INSTALLED_LIB)
    IF(MINGW)
      # I guess -l can be removed here. Also, for build with c/c as submodule this will have to me moved on top level out of this IF's 
      SET(PLATFORM_DEPENDENCIES "${PLATFORM_DEPENDENCIES}" "-lsecur32 -lcrypt32")
      # Not sure if mingw really needs this separate mysql.h search, but let it be
      FIND_PATH(MARIADB_CONNECTORC_INCLUDE NAMES "mysql.h"
                DOC "Path to MariaDB Connector/C Include Directory")

      FIND_PATH(MARIADB_CONNECTORC_LIB NAMES libmariadbclient.a
                                             liblibmariadb.dll.a 
                                             libmariadb.dll 
                                             mariadbclient
                                             mariadb
                DOC "Path to MariaDB Connector/C Lib Directory")
      # Not sure why is shlwapi is needed
      FIND_LIBRARY(MARIADB_SHLWAPI_LIB NAMES ShLwApi.Lib 
                                             ShLwApi
                DOC "Path to ShLwApi.Lib. Usually found in $ENV{programfiles\(x86\)}\\Windows Kits\\10\\Lib")
    ELSE()
      FIND_LIBRARY(CCLIB libmariadb.so)
      IF (${CCLIB} STREQUAL "CCLIB-NOTFOUND")
        # It won't be found by linker either unless user does some magic before build, so we actually could stop here with error
        MESSAGE(STATUS "C/C library has not been found")
        SET(MARIADB_CLIENT_TARGET_NAME mariadb)
      ELSE()
        MESSAGE(STATUS "C/C library found here ${CCLIB}")
        SET(MARIADB_CLIENT_TARGET_NAME ${CCLIB})
      ENDIF()
      FIND_FILE(CCHEADER NAMES "mysql.h"
                PATHS /usr/include/mariadb
                      /usr/include/mysql
                      /usr/local/include/mariadb
                      /usr/local/include/mysql)
                #PATH_SUFFIXES mariadb mysql)
      IF (${CCHEADER} STREQUAL "CCHEADER-NOTFOUND")
        # Again should probably stop
        MESSAGE(STATUS "C/C headers have not been found")
        INCLUDE_DIRECTORIES(/usr/include/mariadb /usr/include/mysql)
      ELSE()
        GET_FILENAME_COMPONENT(CCHEADERDIR ${CCHEADER} DIRECTORY)
        MESSAGE(STATUS "C/C headers have been found here ${CCHEADERDIR}")
        INCLUDE_DIRECTORIES(${CCHEADERDIR})
      ENDIF()
    ENDIF()
  ELSE()
    SET(MARIADB_CLIENT_TARGET_NAME libmariadb)
  ENDIF()
  MESSAGE(STATUS "Linking Connector/C library dynamically(${MARIADB_CLIENT_TARGET_NAME})")
ELSE()
  SET(MARIADB_CLIENT_TARGET_NAME mariadbclient)
  MESSAGE(STATUS "Linking Connector/C library statically(${MARIADB_CLIENT_TARGET_NAME})")
ENDIF()


ADD_LIBRARY(${LIBRARY_NAME}_obj OBJECT ${MACPP_SOURCES})
IF(UNIX)
  SET_TARGET_PROPERTIES(${LIBRARY_NAME}_obj PROPERTIES COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
ENDIF()
SET(${LIBRARY_NAME}_OBJECTS $<TARGET_OBJECTS:${LIBRARY_NAME}_obj>)

IF(WIN32 AND NOT MINGW)
  ADD_LIBRARY(${LIBRARY_NAME} SHARED ${CMAKE_SOURCE_DIR}/src/maconncpp.rc ${${LIBRARY_NAME}_OBJECTS} ${CMAKE_SOURCE_DIR}/src/maconncpp.def)
  ADD_LIBRARY(${STATIC_LIBRARY_NAME} STATIC ${${LIBRARY_NAME}_OBJECTS} ${CMAKE_SOURCE_DIR}/src/maconncpp.rc)

  TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME}_obj PRIVATE "MARIADB_EXPORTED=__declspec(dllexport)")

  TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE "MARIADB_EXPORTED=__declspec(dllexport)")
  TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} INTERFACE "MARIADB_EXPORTED=__declspec(dllimport)")

  TARGET_COMPILE_DEFINITIONS(${STATIC_LIBRARY_NAME} PUBLIC MARIADB_STATIC_LINK)
ELSE()
  # Xcode doesn't support targets that have only object files,
  # so let's add an empty file to keep Xcode happy
  IF(CMAKE_GENERATOR MATCHES Xcode)
    FILE(WRITE ${CMAKE_BINARY_DIR}/empty.cpp "")
    SET(EMPTY_FILE ${CMAKE_BINARY_DIR}/empty.cpp)
  ENDIF()
  #  MESSAGE(STATUS "Version script: ${CMAKE_SOURCE_DIR}/src/maconncpp.def")
  ADD_LIBRARY(${LIBRARY_NAME} SHARED ${${LIBRARY_NAME}_OBJECTS} ${EMPTY_FILE})
  ADD_LIBRARY(${STATIC_LIBRARY_NAME} STATIC ${${LIBRARY_NAME}_OBJECTS} ${EMPTY_FILE})

  IF(APPLE)
    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl"
                                     INSTALL_RPATH_USE_LINK_PATH 0
                                     BUILD_WITH_INSTALL_RPATH 1
                                     XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                                     XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp -f"
                                     INSTALL_RPATH "${MAODBC_INSTALL_RPATH}")
    SET_TARGET_PROPERTIES(${STATIC_LIBRARY_NAME} PROPERTIES 
                                     INSTALL_RPATH_USE_LINK_PATH 0
                                     BUILD_WITH_INSTALL_RPATH 1
                                     XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                                     XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp -f"
                                     INSTALL_RPATH "${MAODBC_INSTALL_RPATH}")
    IF(WITH_SIGNCODE)
      SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: ${DEVELOPER_ID}")
      SET_TARGET_PROPERTIES(${STATIC_LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: ${DEVELOPER_ID}")
    ELSE()
      SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
      SET_TARGET_PROPERTIES(${STATIC_LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
    ENDIF()
  ELSE()
#    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/src/maconncpp.def")
  ENDIF()
ENDIF()

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${MARIADB_CLIENT_TARGET_NAME} ${PLATFORM_DEPENDENCIES})
TARGET_LINK_LIBRARIES(${STATIC_LIBRARY_NAME} mariadbclient)

# MINGW Specific includes and linking
IF(MINGW)
  
  IF(MARIADB_SHLWAPI_LIB)
    TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${MARIADB_SHLWAPI_LIB})
    MESSAGE(STATUS "Found shlwapi lib - ${MARIADB_SHLWAPI_LIB}")
  ELSE()
    # Should here and below be just warnings?
    MESSAGE(WARNING "Did not find shlwapi lib")
  ENDIF()

  IF(MARIADB_CONNECTORC_INCLUDE)
    # Probably has to be private, and not PUBLIC here
    TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME}_obj PUBLIC  ${MARIADB_CONNECTORC_INCLUDE})
    MESSAGE(STATUS "Found MariaDB Connector/C includes - ${MARIADB_CONNECTORC_INCLUDE}")
  ELSE()
    MESSAGE(WARNING "Did not find MariaDB Connector/C includes")
  ENDIF()
  
  IF(MARIADB_CONNECTORC_LIB)
    TARGET_LINK_DIRECTORIES(${LIBRARY_NAME} PUBLIC ${MARIADB_CONNECTORC_LIB})
    TARGET_LINK_DIRECTORIES(${STATIC_LIBRARY_NAME} PUBLIC ${MARIADB_CONNECTORC_LIB})
    MESSAGE(STATUS "Found MariaDB Connector/C libraries - ${MARIADB_CONNECTORC_LIB}")
  ELSE()
    MESSAGE(WARNING "Did not find MariaDB Connector/C libraries")
  ENDIF()
  
ENDIF()

INCLUDE(symlink)
CREATE_SYMLINK(lib${LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX} ${STATIC_LIBRARY_NAME} ${INSTALL_LIBDIR})

ADD_DEPENDENCIES(${LIBRARY_NAME} DEPENDENCIES_FOR_PACKAGE)

########## Packaging ##########

# MSI
IF(WIN32)
  IF(WITH_MSI)
    ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} ARGS -DDRIVER_LIB_DIR=$<TARGET_FILE_DIR:${LIBRARY_NAME}>
                                    -DPLUGINS_LIB_DIR=$<TARGET_FILE_DIR:dialog>
                                    -DPLUGINS_SUBDIR_NAME=${MARIADB_DEFAULT_PLUGINS_SUBDIR}
                                    -DFILE_IN=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml.in
                                    -DFILE_OUT=${CMAKE_SOURCE_DIR}/wininstall/binaries_dir.xml
                                    -P ${CMAKE_SOURCE_DIR}/cmake/ConfigureFile.cmake
                       )
    ADD_SUBDIRECTORY(wininstall)
  ENDIF()
ELSE()
  IF(APPLE)
    #MESSAGE(STATUS "Configuring to generate PKG package")
    #ADD_SUBDIRECTORY(osxinstall)
  ENDIF()
  INSTALL(TARGETS ${LIBRARY_NAME}
          LIBRARY DESTINATION ${INSTALL_LIBDIR}
          COMPONENT SharedLibraries)
  INSTALL(TARGETS
          ${STATIC_LIBRARY_NAME}
          ARCHIVE DESTINATION ${INSTALL_LIBDIR}
          COMPONENT Development)

  MESSAGE(STATUS "Documentation installed to ${INSTALL_DOCDIR}")
  MESSAGE(STATUS "License file installed to ${INSTALL_LICENSEDIR}")
  MESSAGE(STATUS "Public API header files installed to ${INSTALL_INCLUDEDIR}")
  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/README
          DESTINATION
          "${INSTALL_DOCDIR}"
          COMPONENT Documentation)
  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/COPYING
          DESTINATION
          "${INSTALL_LICENSEDIR}"
          COMPONENT Documentation)
  ADD_SUBDIRECTORY(include)
ENDIF()
# Tests. Checking if we have them. May be not the case if we are building from source package
IF(WITH_UNIT_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  ADD_SUBDIRECTORY(test)
  ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
	  COMMAND ${CMAKE_COMMAND} ARGS -E copy $<TARGET_FILE:${LIBRARY_NAME}> test)
  IF(NOT MINGW AND NOT USE_SYSTEM_INSTALLED_LIB)
    ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
	  COMMAND ${CMAKE_COMMAND} ARGS -E copy $<TARGET_FILE:libmariadb> test)
  ENDIF()
ENDIF()

# Packaging
INCLUDE(packaging)
MESSAGE(STATUS "License File: ${CPACK_RESOURCE_FILE_LICENSE}")
MESSAGE(STATUS "ReadMe File: ${CPACK_PACKAGE_DESCRIPTION_FILE}")
MESSAGE(STATUS "Source Package Filename: ${CPACK_SOURCE_PACKAGE_FILE_NAME}.${CPACK_SOURCE_GENERATOR}")

# install_test/CMakeLists.txt configuration should go after packaging.cmake but before include(CPack). Since CPack included in packaging cmake,
# that configuration has been moved there. For the same reason there also moved the message with package name
