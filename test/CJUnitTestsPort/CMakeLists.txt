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

IF(WIN32)
  LINK_DIRECTORIES(../framework/$(ConfigurationName))
ENDIF()
SET(MY_TARGET_LINK_LIBRARIES test_framework ${LIBRARY_NAME})

IF(CMAKE_COMPILER_IS_GNUCC AND MYSQLCPPCONN_GCOV_ENABLE)
  SET(MY_GCOV_LINK_LIBRARIES gcov)
ENDIF(CMAKE_COMPILER_IS_GNUCC AND MYSQLCPPCONN_GCOV_ENABLE)

ADD_DEFINITIONS("-DWE_HAVE_DATATYPE_AGAIN")

IF(MYSQLCPPCONN_TEST_NOT_IMPLEMENTED)
  ADD_DEFINITIONS("-DINCLUDE_NOT_IMPLEMENTED_METHODS=1")
ENDIF(MYSQLCPPCONN_TEST_NOT_IMPLEMENTED)

SET(jdbctests_sources
  ccpptests.cpp
  BaseTestFixture.cpp
  ../common/stringutils.cpp
  resources.cpp
  regression/EscapeProcessorRegressionTest.cpp
  compliance/ConnectionTest.cpp
  compliance/DatabaseMetaDataTest.cpp
  compliance/StatementTest.cpp
  compliance/ResultSetMetadataTest.cpp
  simple/BlobTest.cpp
  regression/BlobRegressionTest.cpp
  regression/PreparedStatementRegressionTest.cpp
  simple/TransactionTest.cpp
  regression/SubqueriesRegressionTest.cpp
  compliance/UnbufferedRsStmtTest.cpp
  )
IF (NOT BUILD_TESTS_ONLY)
  SET(jdbctests_sources ${jdbctests_sources} 
    compliance/PreparedStatementTest.cpp
    )
ENDIF()

# It's convinient for VS users to have headers as part of project
IF(WIN32)
  SET(jdbctests_sources
        ${jdbctests_sources}
    BaseTestFixture.h
    ../common/stringutils.h
    resources.h
    regression/EscapeProcessorRegressionTest.h
    compliance/ConnectionTest.h
    compliance/DatabaseMetaDataTest.h
    compliance/StatementTest.h
    compliance/ResultSetMetadataTest.h
    compliance/PreparedStatementTest.h
    simple/BlobTest.h
    regression/BlobRegressionTest.h
    regression/PreparedStatementRegressionTest.h
    simple/TransactionTest.h
    regression/SubqueriesRegressionTest.h
    compliance/UnbufferedRsStmtTest.h
    )
ENDIF(WIN32)

ADD_EXECUTABLE(CJUnitTestsPort ${jdbctests_sources})
TARGET_LINK_LIBRARIES(CJUnitTestsPort ${MY_TARGET_LINK_LIBRARIES} ${MY_GCOV_LINK_LIBRARIES})
SET_TARGET_PROPERTIES(CJUnitTestsPort PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")
SET_TARGET_PROPERTIES(CJUnitTestsPort PROPERTIES OUTPUT_NAME cjportedtests)
#Copy sql.properties file to build dir
FILE(COPY sql.properties DESTINATION ${CMAKE_BINARY_DIR}/test/)
CONFIGURE_FILE(cts.sql.in
	${CMAKE_BINARY_DIR}/test/cts.sql)


MESSAGE(STATUS "Configuring C/J junit tests port")
