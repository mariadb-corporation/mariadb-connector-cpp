# ************************************************************************************
#   Copyright (C) 2013,2021 MariaDB Corporation AB
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
  cmake_minimum_required(VERSION 3.23)
ELSE()
  cmake_minimum_required(VERSION 3.1)
ENDIF()
CMAKE_POLICY(SET CMP0048 NEW)

PROJECT(mariadb_connector_cpp
        VERSION 1.0.3
#        DESCRIPTION "MariaDB Connector/C++"
        LANGUAGES CXX C)

IF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
  MESSAGE(FATAL_ERROR "GCC version >=4.9 is required")
ENDIF()

SET(MACPP_VERSION_QUALITY "ga") #Empty also means GA
SET(MACPP_VERSION "1.00.0003")
SET(MARIADB_DEFAULT_PLUGINS_SUBDIR "plugin")

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(LIBRARY_NAME "mariadbcpp")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/Version.h.in
               ${CMAKE_SOURCE_DIR}/src/Version.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/maconncpp.rc.in
               ${CMAKE_SOURCE_DIR}/src/maconncpp.rc)
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/SearchLibrary.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/SetValueMacro.cmake)


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
                   src/logger/LoggerFactory.cpp
                   src/logger/ProtocolLoggingProxy.cpp

                   src/parameters/ParameterHolder.cpp

                   src/options/Options.cpp
                   src/options/DefaultOptions.cpp

                   src/pool/GlobalStateInfo.cpp
                   src/pool/Pools.cpp

                   src/failover/FailoverProxy.cpp

                   src/credential/CredentialPluginLoader.cpp

                   src/SelectResultSet.cpp
                   src/com/capi/SelectResultSetCapi.cpp
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
                   src/MariaDbPooledConnection.cpp
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
                   ####CPP####
                   )

# Raw class implementations of classes we are not gonna use atm
#                   src/pool/Pool.cpp

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

                   src/pool/GlobalStateInfo.h
                   src/pool/Pools.h
                   src/pool/Pool.h

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

                   src/SelectResultSet.h
                   src/com/capi/SelectResultSetCapi.h
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
                   src/MariaDbPooledConnection.h
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

  SET(PLATFORM_DEPENDENCIES ws2_32)# Shlwapi)
ELSE()
  SEARCH_LIBRARY(LIB_MATH floor m)
  MESSAGE(STATUS "Found math lib: ${LIB_MATH}")
  SET(PLATFORM_DEPENDENCIES ${LIB_MATH})
ENDIF()

# This has to be before C/C's cmake run, or it will build with /MD
IF(WIN32)
  IF (MSVC)
    SET(CONFIG_TYPES "DEBUG" "RELEASE" "RELWITHDEBINFO" "MINSIZEREL")
    FOREACH(BUILD_TYPE ${CONFIG_TYPES})
      FOREACH(COMPILER CXX C)
        SET(COMPILER_FLAGS "${CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}}")
        IF (NOT COMPILER_FLAGS STREQUAL "")
          STRING(REPLACE "/MD" "/MT" COMPILER_FLAGS ${COMPILER_FLAGS})
          IF (BUILD_TYPE STREQUAL "Debug")
            SET(COMPILER_FLAGS "${COMPILER_FLAGS} /RTC1 /RTCc")
            STRING(REPLACE "/Zi" "/ZI" COMPILER_FLAGS ${COMPILER_FLAGS})
          ENDIF()
          MESSAGE (STATUS "CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE}= ${COMPILER_FLAGS}")
          SET(CMAKE_${COMPILER}_FLAGS_${BUILD_TYPE} ${COMPILER_FLAGS} CACHE
               STRING "overwritten by mariadb-cpp" FORCE)
        ENDIF()
      ENDFOREACH()
    ENDFOREACH()
  ENDIF()
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN)
  ADD_DEFINITIONS("-DMARIADB_EXPORTED=__declspec(dllexport)")
  SET(INSTALL_PLUGINDIR "${MARIADB_DEFAULT_PLUGINS_SUBDIR}")
  SET(PLATFORM_DEPENDENCIES ${PLATFORM_DEPENDENCIES} version.lib)
ENDIF()

### Build options, initial settings and platform defaults
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/options_defaults.cmake")

### Setting installation paths - should go before C/C subproject sets its own. We need to have control over those
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/install.cmake")
MESSAGE(STATUS "Libraries installation dir: ${INSTALL_LIB_SUFFIX}")
MESSAGE(STATUS "Authentication Plugins installation dir: ${INSTALL_PLUGINDIR}")

IF(WIN32)
  MESSAGE(STATUS "Generate MSI package: ${WITH_MSI}")
  MESSAGE(STATUS "Sign MSI package: ${WITH_SIGNCODE}")
ENDIF()

ADD_CUSTOM_TARGET(DEPENDENCIES_FOR_PACKAGE)
IF(NOT WITH_SSL)
  MESSAGE(STATUS "Setting default value for WITH_SSL for libmariadb build to ON")
  SET(CONC_WITH_SSL ON)
ENDIF()
### Including C/C subproject
IF(EXISTS ${CMAKE_SOURCE_DIR}/libmariadb)
  IF(GIT_BUILD_SRCPKG)
    # We don't want conn/c (wrong) src pkg to be built.
    SET(GIT_BUILD_SRCPKG FALSE)
    SET(CONNCPP_GIT_BUILD_SRCPKG TRUE)
  ENDIF()
  MESSAGE(STATUS "Running C/C cmake scripts")
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/connector_c.cmake)
  INSTALL(FILES
          $<TARGET_FILE:libmariadb>
          DESTINATION ${INSTALL_LIBDIR}
 	        COMPONENT ConCLib)
  MESSAGE(STATUS "Configuring to install libmariadb to ${INSTALL_LIBDIR}")

  SET(OWN_PLUGINS_LIST mysql_clear_password dialog client_ed25519 sha256_password caching_sha2_password)
  IF (PLUGINS_DYNAMIC)
    # The list from CC is visible for us
    SET(PLUGINS_LIST ${PLUGINS_DYNAMIC})
  ELSE()
    SET(PLUGINS_LIST ${OWN_PLUGINS_LIST})
  ENDIF()
  FOREACH(CC_PLUGIN ${PLUGINS_LIST})
    IF(NOT PLUGINS_DYNAMIC OR "${PLUGIN_${CC_PLUGIN}_TYPE}" STREQUAL "MARIADB_CLIENT_PLUGIN_AUTH")
      MESSAGE(STATUS "Configuring to install ${CC_PLUGIN} to ${INSTALL_PLUGINDIR}")
      ADD_DEPENDENCIES(DEPENDENCIES_FOR_PACKAGE ${CC_PLUGIN})
      INSTALL(FILES
              $<TARGET_FILE:${CC_PLUGIN}>
              DESTINATION ${INSTALL_PLUGINDIR}
 	            COMPONENT ConCPlugins)
    ENDIF()
  ENDFOREACH()
#  ADD_DEPENDENCIES(libmariadb DEPENDENCIES_FOR_PACKAGE)
ELSE()
  SET(USE_SYSTEM_INSTALLED_LIB TRUE)
  MESSAGE(STATUS "There is no Connector/C sub-project folder, linking against libmariadb installed on the system")
ENDIF()

IF(WITH_MSI AND WITH_SIGNCODE)
  IF(WIN32 AND NOT SIGN_OPTIONS)
    SET(SIGN_OPTIONS /a /t http://timestamp.verisign.com/scripts/timstamp.dll)
  ELSE()
    SEPARATE_ARGUMENTS(SIGN_OPTIONS)
  ENDIF()
  MARK_AS_ADVANCED(SIGN_OPTIONS)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libmariadb/include)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/libmariadb/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/conncpp)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
ENDIF()
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/maconncpp.def.in
               ${CMAKE_SOURCE_DIR}/src/maconncpp.def)

# Dynamic linking is default on non-Windows
IF(MARIADB_LINK_DYNAMIC OR NOT WIN32)# OR USE_SYSTEM_INSTALLED_LIB)
  IF(USE_SYSTEM_INSTALLED_LIB)
    SET(MARIADB_CLIENT_TARGET_NAME mariadb)
  ELSE()
    SET(MARIADB_CLIENT_TARGET_NAME libmariadb)
  ENDIF()
  MESSAGE(STATUS "Linking Connector/C library dynamically(${MARIADB_CLIENT_TARGET_NAME})")
ELSE()
  SET(MARIADB_CLIENT_TARGET_NAME mariadbclient)
  MESSAGE(STATUS "Linking Connector/C library statically(${MARIADB_CLIENT_TARGET_NAME})")
ENDIF()

IF(WIN32)
  ADD_LIBRARY(${LIBRARY_NAME} SHARED ${MACPP_SOURCES} ${CMAKE_SOURCE_DIR}/src/maconncpp.def ${CMAKE_SOURCE_DIR}/src/maconncpp.rc)
ELSE()
#  MESSAGE(STATUS "Version script: ${CMAKE_SOURCE_DIR}/src/maconncpp.def")
  ADD_LIBRARY(${LIBRARY_NAME} SHARED ${MACPP_SOURCES})
  IF(APPLE)
    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl"
                                     INSTALL_RPATH_USE_LINK_PATH 0
                                     BUILD_WITH_INSTALL_RPATH 1
                                     XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                                     XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp -f"
                                     INSTALL_RPATH "${MAODBC_INSTALL_RPATH}")
    IF(WITH_SIGNCODE)
      SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: ${DEVELOPER_ID}")
    ELSE()
      SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
    ENDIF()
  ELSE()
#    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/src/maconncpp.def")
  ENDIF()
ENDIF()

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${MARIADB_CLIENT_TARGET_NAME} ${PLATFORM_DEPENDENCIES})
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
  INSTALL(TARGETS
          ${LIBRARY_NAME}
          LIBRARY DESTINATION ${INSTALL_LIB_SUFFIX}/mariadb
          COMPONENT CppLibs)

  MESSAGE(STATUS "Documentation installed to ${INSTALL_DOCDIR}")
  MESSAGE(STATUS "License file installed to ${INSTALL_LICENSEDIR}")
  MESSAGE(STATUS "Public API header files installed to ${INSTALL_INCLUDEDIR}")
  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/README
          DESTINATION
          ${INSTALL_DOCDIR}
          COMPONENT Documentation)
  INSTALL(FILES
          ${CMAKE_SOURCE_DIR}/COPYING
          DESTINATION
          ${INSTALL_LICENSEDIR}
          COMPONENT Documentation)
  ADD_SUBDIRECTORY(include)
ENDIF()
# Tests. Checking if we have them. May be not the case if we are building from source package
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  ADD_SUBDIRECTORY(test)
  ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
	  COMMAND ${CMAKE_COMMAND} ARGS -E copy $<TARGET_FILE:${LIBRARY_NAME}> test)
  ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
	  COMMAND ${CMAKE_COMMAND} ARGS -E copy $<TARGET_FILE:libmariadb> test)
ENDIF()

# Packaging
INCLUDE(cmake/packaging.cmake)
MESSAGE(STATUS "License File: ${CPACK_RESOURCE_FILE_LICENSE}")
MESSAGE(STATUS "ReadMe File: ${CPACK_PACKAGE_DESCRIPTION_FILE}")
MESSAGE(STATUS "Source Package Filename: ${CPACK_SOURCE_PACKAGE_FILE_NAME}.${CPACK_SOURCE_GENERATOR}")

# install_test/CMakeLists.txt fuke configuration should go after packaging.cmake but before include(CPack). Since CPack included in packaging cmake,
# that configuration has been moved there. For the same reason there also moved the message with package name
