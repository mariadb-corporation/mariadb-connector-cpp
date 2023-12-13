# Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
#               2020 MariaDB Corporation AB
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0, as
# published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an
# additional permission to link the program and your derivative works
# with the separately licensed software that they have included with
# MySQL.
#
# Without limiting anything contained in the foregoing, this file,
# which is part of MySQL Connector/C++, is also subject to the
# Universal FOSS Exception, version 1.0, a copy of which can be found at
# http://oss.oracle.com/licenses/universal-foss-exception.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

SET(test_connection_sources
  ${test_common_sources}
  connection.cpp)

IF(WIN32)
  SET(test_connection_sources
        ${test_connection_sources}
    connectiontest.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_connection ${test_connection_sources})
SET_TARGET_PROPERTIES(test_connection PROPERTIES
          OUTPUT_NAME "connection"
	        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_connection ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})
IF(NOT WIN32)
  TARGET_LINK_LIBRARIES(test_connection pthread)
ENDIF()

MESSAGE(STATUS "Configuring unit tests - connection")

SET(test_databasemetadata_sources
  ${test_common_sources}
  connectionmetadata.cpp)

IF(WIN32)
  SET(test_databasemetadata_sources
      ${test_databasemetadata_sources}
      connectionmetadata.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_databasemetadata ${test_databasemetadata_sources})
SET_TARGET_PROPERTIES(test_databasemetadata PROPERTIES
          OUTPUT_NAME "databasemetadata"
	  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_databasemetadata ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring unit tests - databasemetadata")


SET(test_resultsetmetadata_sources
  ${test_common_sources}
  resultsetmetadata.cpp)

IF(WIN32)
  SET(test_resultsetmetadata_sources
      ${test_resultsetmetadata_sources}
      resultsetmetadatatest.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_resultsetmetadata ${test_resultsetmetadata_sources})
SET_TARGET_PROPERTIES(test_resultsetmetadata PROPERTIES
          OUTPUT_NAME "resultsetmetadata"
	  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_resultsetmetadata ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring unit tests - resultsetmetadata")

SET(test_resultset_sources
  ${test_common_sources}
  resultset.cpp)

IF(WIN32)
  SET(test_resultset_sources
      ${test_resultset_sources}
      resultsettest.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_resultset ${test_resultset_sources})
SET_TARGET_PROPERTIES(test_resultset PROPERTIES
          OUTPUT_NAME "resultset"
	  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_resultset ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring unit tests - resultset")

SET(test_savepoint_sources
  ${test_common_sources}
  savepoint.cpp)

IF(WIN32)
  SET(test_savepoint_sources
      ${test_savepoint_sources}
      savepointtest.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_savepoint ${test_savepoint_sources})
SET_TARGET_PROPERTIES(test_savepoint PROPERTIES
          OUTPUT_NAME "savepoint"
	        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_savepoint ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring unit tests - savepoint")

SET(test_preparedstatement_sources
  ${test_common_sources}
  preparedstatement.cpp)

IF(WIN32)
  SET(test_preparedstatement_sources
      ${test_preparedstatement_sources}
      preparedstatementtest.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_preparedstatement ${test_preparedstatement_sources})
SET_TARGET_PROPERTIES(test_preparedstatement PROPERTIES
          OUTPUT_NAME "preparedstatement"
	        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_preparedstatement ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring unit tests - preparedstatement")

SET(test_parametermetadata_sources
  ${test_common_sources}
  parametermetadata.cpp)

IF(WIN32)
  SET(test_parametermetadata_sources
      ${test_parametermetadata_sources}
      parametermetadatatest.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_parametermetadata ${test_parametermetadata_sources})
SET_TARGET_PROPERTIES(test_parametermetadata PROPERTIES
          OUTPUT_NAME "parametermetadata"
	        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_parametermetadata ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring unit tests - parametermetadata")

SET(test_art_resultset_sources
  ${test_common_sources}
  art_resultset.cpp)

IF(WIN32)
  SET(test_art_resultset_sources
      ${test_art_resultset_sources}
      art_resultset.h)

  ADD_DEFINITIONS("-DCPPCONN_PUBLIC_FUNC=")
ENDIF(WIN32)

#ADD_EXECUTABLE(test_art_resultset ${test_art_resultset_sources})
#SET_TARGET_PROPERTIES(test_art_resultset PROPERTIES
#          OUTPUT_NAME "art_resultset"
#          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
#          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
#TARGET_LINK_LIBRARIES(test_art_resultset ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})
#
#MESSAGE(STATUS "Configuring unit tests - art_resultset")

SET(test_statement_sources
  ${test_common_sources}
  statement.cpp)

IF(WIN32)
  SET(test_statement_sources
      ${test_statement_sources}
      statementtest.h)
ENDIF(WIN32)

ADD_EXECUTABLE(test_statement ${test_statement_sources})
SET_TARGET_PROPERTIES(test_statement PROPERTIES
          OUTPUT_NAME "statement"
	        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(test_statement ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring unit tests - statement")

SET(test_uri_sources
  ${test_common_sources}
  "${CMAKE_SOURCE_DIR}/driver/mysql_uri.cpp"
  "${CMAKE_SOURCE_DIR}/driver/mysql_util.cpp"
  uri.cpp)

IF(WIN32)
  SET(test_uri_sources
        ${test_uri_sources}
    "${CMAKE_SOURCE_DIR}/driver/mysql_uri.h"
    uri.h)
ENDIF(WIN32)

#ADD_EXECUTABLE(test_uri ${test_uri_sources})
#SET_TARGET_PROPERTIES(test_uri PROPERTIES
#          OUTPUT_NAME "uri"
#          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
#          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
#TARGET_LINK_LIBRARIES(test_uri ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})

#MESSAGE(STATUS "Configuring unit tests - uri")


SET(test_variant_sources
  ${test_common_sources}
  variant.cpp)

IF(WIN32)
  SET(test_variant_sources
    ${test_variant_sources}
    variant.h)
ENDIF(WIN32)

#ADD_EXECUTABLE(test_variant ${test_variant_sources})
#SET_TARGET_PROPERTIES(test_variant PROPERTIES
#          OUTPUT_NAME "variant"
#          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
#          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
#TARGET_LINK_LIBRARIES(test_variant ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} test_framework ${MY_GCOV_LINK_LIBRARIES})
#
#MESSAGE(STATUS "Configuring unit tests - variant")
