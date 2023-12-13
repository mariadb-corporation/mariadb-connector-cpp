# Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
#               2020, 2022 MariaDB Corporation AB
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

#default test server params configured in options_defaults.cmake 

configure_file(tests_config.h.in
               ${CMAKE_BINARY_DIR}/test/tests_config.h)

INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/test")

IF(WIN32)
  IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    #LINK_DIRECTORIES(${}/lib/debug)
  ELSEIF(CMAKE_BUILD_TYPE STREQUAL "")
    #LINK_DIRECTORIES(${}/lib/opt)
  ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

  ADD_DEFINITIONS("-D_SCL_SECURE_NO_WARNINGS")
  ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS")
  ADD_DEFINITIONS("-DMARIADB_EXPORTED=__declspec(dllimport)")

ELSEIF(NOT WIN32)
  #ADD_DEFINITIONS("-DMARIADB_EXPORTED")

ENDIF(WIN32)

ENABLE_TESTING()

IF(CMAKE_COMPILER_IS_GNUCC AND MYSQLCPPCONN_GCOV_ENABLE)
  SET(MY_GCOV_LINK_LIBRARIES gcov)
ENDIF(CMAKE_COMPILER_IS_GNUCC AND MYSQLCPPCONN_GCOV_ENABLE)

ADD_EXECUTABLE(static_test static_test.cpp)
TARGET_LINK_LIBRARIES(static_test ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} ${MY_GCOV_LINK_LIBRARIES})

ADD_TEST(static_test ${EXECUTABLE_OUTPUT_PATH}/static_test)
SET_TESTS_PROPERTIES(static_test PROPERTIES TIMEOUT 120)

ADD_EXECUTABLE(driver_test driver_test.cpp)
TARGET_LINK_LIBRARIES(driver_test ${LIBRARY_NAME} ${PLATFORM_DEPENDENCIES} ${MY_GCOV_LINK_LIBRARIES})

ADD_TEST(driver_test ${EXECUTABLE_OUTPUT_PATH}/driver_test)

ADD_SUBDIRECTORY(framework)
ADD_SUBDIRECTORY(CJUnitTestsPort)
ADD_SUBDIRECTORY(unit)

ADD_TEST(compliance cjportedtests)
ADD_TEST(test_parametermetadata parametermetadata)
ADD_TEST(test_preparedstatement preparedstatement)
ADD_TEST(test_resultsetmetadata resultsetmetadata)
ADD_TEST(test_connection connection)
ADD_TEST(test_databasemetadata databasemetadata)
ADD_TEST(test_resultset resultset)
ADD_TEST(test_statement statement)
ADD_TEST(unsorted_bugs unsorted_bugs)


SET_TESTS_PROPERTIES(test_parametermetadata test_resultsetmetadata test_connection PROPERTIES TIMEOUT 120 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
# With remote SkySQL server, compliance tend to timeout
SET_TESTS_PROPERTIES(static_test driver_test PROPERTIES TIMEOUT 150 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
SET_TESTS_PROPERTIES(compliance test_databasemetadata test_preparedstatement test_resultset unsorted_bugs PROPERTIES TIMEOUT 1200 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

MESSAGE(STATUS "Configuring test cases")
