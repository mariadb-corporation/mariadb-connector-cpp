# Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
#               2023 MariaDB Corportation AB
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


SET(perf_statement_sources
    ${test_common_sources}
    perf_statement.cpp)

IF(WIN32)
  SET(perf_statement_sources
      ${perf_statement_sources}
      perf_statement.h)
ENDIF(WIN32)

ADD_EXECUTABLE(perf_statement ${perf_statement_sources})
SET_TARGET_PROPERTIES(perf_statement PROPERTIES
          OUTPUT_NAME "perf_statement"
          LINK_FLAGS "${MYSQLCPPCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}"
          COMPILE_FLAGS "${MYSQLCPPCONN_COMPILE_FLAGS_ENV}")
TARGET_LINK_LIBRARIES(perf_statement ${PLATFORM_DEPENDENCIES} ${MY_GCOV_LINK_LIBRARIES} test_framework ${LIBRARY_NAME})

MESSAGE(STATUS "Configuring performance test - statement")
