# Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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


SET(bugs_sources
  ${test_common_sources}
  bugs.cpp)

IF(WIN32)
  SET(bugs_sources
      ${bugs_sources}
      bugs.h)

ENDIF(WIN32)

ADD_EXECUTABLE(unsorted_bugs ${bugs_sources})
SET_TARGET_PROPERTIES(unsorted_bugs PROPERTIES
          OUTPUT_NAME "unsorted_bugs"
	        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")
TARGET_LINK_LIBRARIES(unsorted_bugs ${PLATFORM_DEPENDENCIES} test_framework ${LIBRARY_NAME} ${MY_GCOV_LINK_LIBRARIES})

MESSAGE(STATUS "Configuring bugs test cases - unsorted")
