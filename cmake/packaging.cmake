INCLUDE(cmake/ConnectorName.cmake)
SET(CPACK_PACKAGE_NAME "mariadb-connector-cpp")
SET(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
IF(NOT CPACK_PACKAGE_RELEASE)
  SET(CPACK_PACKAGE_RELEASE 1)
ENDIF()
SET(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)
SET(CPACK_PACKAGE_VENDOR "MariaDB Corporation plc")
SET(CPACK_PACKAGE_CONTACT "info@mariadb.com")
SET(CPACK_PACKAGE_DESCRIPTION "MariaDB Connector/C++. C++ driver library for connecting to MariaDB and MySQL servers")
SET(CPACK_PACKAGE_LICENSE "LGPLv2.1")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_PACKAGE_API_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/")

IF(INSTALL_LAYOUT STREQUAL "DEFAULT")
  SET(CPACK_COMPONENTS_ALL Plugins SharedLibraries ConnectorC Documentation Development)
ELSEIF(INSTALL_LAYOUT STREQUAL "PKG")
  SET(CPACK_COMPONENTS_ALL Plugins SharedLibraries Documentation Development)
ELSE()
  SET(CPACK_COMPONENTS_ALL SharedLibraries Documentation Development)
ENDIF()

IF(NOT SYSTEM_NAME)
  STRING(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)
ENDIF()

SET(QUALITY_SUFFIX "")
IF (MACPP_VERSION_QUALITY AND NOT "${MACPP_VERSION_QUALITY}" STREQUAL "ga" AND NOT "${MACPP_VERSION_QUALITY}" STREQUAL "GA")
  SET(QUALITY_SUFFIX "-${MACPP_VERSION_QUALITY}")
ENDIF()
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "mariadb-connector-cpp-${CPACK_PACKAGE_VERSION}${QUALITY_SUFFIX}-src")

GET_CONNECTOR_PACKAGE_NAME(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}")

SET(CPACK_SOURCE_IGNORE_FILES
/test/
/.git/
.gitignore
.gitmodules
.gitattributes
CMakeCache.txt
cmake_dist.cmake
CPackSourceConfig.cmake
CPackConfig.cmake
/.build/
cmake_install.cmake
CTestTestfile.cmake
/CMakeFiles/
/version_resources/
.*vcxproj
.*gz$
.*zip$
.*so$
.*so.2
.*so.3
.*dll$
.*a$
.*pdb$
.*sln$
.*sdf$
install_manifest_*txt
Makefile$
tests_config.h
/autom4te.cache/
/.travis/
.travis.yml
/libmariadb/
/_CPack_Packages/
)

# Build source packages
IF(GIT_BUILD_SRCPKG OR CONNCPP_GIT_BUILD_SRCPKG)
  IF(WIN32)
    EXECUTE_PROCESS(COMMAND git archive --format=zip --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.zip --worktree-attributes -v HEAD)
  ELSE()
    EXECUTE_PROCESS(COMMAND git archive ${GIT_BRANCH} --format=zip --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.zip -v HEAD)
    EXECUTE_PROCESS(COMMAND git archive ${GIT_BRANCH} --format=tar --prefix=${CPACK_SOURCE_PACKAGE_FILE_NAME}/ --output=${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar -v HEAD)
    EXECUTE_PROCESS(COMMAND gzip -9 -f ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar)
  ENDIF()
ENDIF()
IF(WIN32)
  SET(DEFAULT_GENERATOR "ZIP")
ELSE()
  SET(DEFAULT_GENERATOR "TGZ")
ENDIF()
IF(NOT CPACK_GENERATOR)
  SET(CPACK_GENERATOR "${DEFAULT_GENERATOR}")
ENDIF()
IF(NOT CPACK_SOURCE_GENERATOR)
  SET(CPACK_SOURCE_GENERATOR "${DEFAULT_GENERATOR}")
ENDIF()

IF(PKG)
  SET(CPACK_GENERATOR "productbuild")
  CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/osxinstall/resources/WELCOME.html.in
                 ${CMAKE_BINARY_DIR}/osxinstall/resources/WELCOME.html @ONLY)
  SET(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_BINARY_DIR}/osxinstall/resources/WELCOME.html")
  SET(CPACK_RESOURCE_FILE_README  "${CMAKE_SOURCE_DIR}/osxinstall/resources/README.html")
  SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/osxinstall/resources/LICENSE.html")

  SET(CPACK_PRODUCTBUILD_IDENTIFIER "com.mariadb.connector.cpp")
  SET(CPACK_PRODUCTBUILD_DOMAINS ON)
  SET(CPACK_PRODUCTBUILD_DOMAINS_USER ON)
  SET(CPACK_POSTFLIGHT_CONNECTORCPP_SCRIPT "${CMAKE_SOURCE_DIR}/osxinstall/postinstall")
  #SET(CPACK_POSTFLIGHT_PUBLICAPI_SCRIPT "${CMAKE_SOURCE_DIR}/osxinstall/papipostinstall")
  SET(CPACK_PRODUCTBUILD_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/osxinstall/resources")
  SET(CPACK_PRODUCTBUILD_BACKGROUND  "mdb-dialog-popup.png")
  SET(CPACK_PRODUCTBUILD_BACKGROUND_ALIGNMENT "left")
  CPACK_ADD_COMPONENT(ConnectorCpp DISPLAY_NAME "Connector Library" DESCRIPTION "Connector Dynamic Library.")
  CPACK_ADD_COMPONENT(Plugins DISPLAY_NAME "Authentication Plugins" DESCRIPTION "Set of Connector/C Authentication Plugin Libraries.")
  CPACK_ADD_COMPONENT(PublicApi DISPLAY_NAME "Public API" DESCRIPTION "Connector's Public API Headers.")
  CPACK_ADD_COMPONENT(Documentation DISPLAY_NAME "Documentation" DESCRIPTION "Connector License and Readme files.")

  #SET(PRODUCT_SERIES "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
  IF(WITH_SIGNCODE)
  #  SET(SIGN_WITH_DEVID "--sign \"Developer ID Installer: ${DEVELOPER_ID}\"")
    SET(CPACK_PRODUCTBUILD_IDENTITY_NAME "Developer ID Installer: ${DEVELOPER_ID}")
  ELSE()
  #  SET(SIGN_WITH_DEVID "")
  ENDIF()

  #CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/distribution.plist.in
  #               ${CMAKE_CURRENT_BINARY_DIR}/distribution.plist @ONLY)
ENDIF()

# "set/append array" - append a set of strings, separated by a space
MACRO(SETA var)
  FOREACH(v ${ARGN})
    SET(${var} "${${var}} ${v}")
  ENDFOREACH()
ENDMACRO(SETA)

#########################
# DEB and RPM packaging #
#########################
IF(DEB)
  SET(CPACK_GENERATOR "DEB")
  SET(CPACK_DEBIAN_PACKAGE_SECTION "devel")
  SET(CPACK_DEBIAN_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
  SET(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
  SET(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
  SET(CPACK_DEBIAN_PACKAGE_DEBUG ON)
  SET(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
  SET(CPACK_DEB_COMPONENT_INSTALL    ON)
  SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  SET(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
  EXECUTE_PROCESS(COMMAND lsb_release -sc OUTPUT_VARIABLE DIST OUTPUT_STRIP_TRAILING_WHITESPACE)
  IF(NOT DIST)
    SET(DIST ${DEB})
  ENDIF()
  SET(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_PACKAGE_RELEASE}+maria~${DIST}")
ENDIF()
#
#
IF(RPM)
  SET(CPACK_GENERATOR "RPM")
  SET(CPACK_RPM_PACKAGE_DEBUG ON)
  SET(CPACK_RPM_PACKAGE_GROUP "Development/Libraries") # deprecated
  SET(CPACK_RPM_COMPONENT_INSTALL ON)
  IF(CMAKE_VERSION VERSION_LESS "3.6.0")
    SET(CPACK_RPM_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
    EXECUTE_PROCESS(COMMAND rpm --eval %dist
                    OUTPUT_VARIABLE DIST OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(CPACK_RPM_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_RPM_PACKAGE_NAME}-${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}${DIST}-${CMAKE_SYSTEM_PROCESSOR}")
  ELSE()
    SET(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
    SET(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
    OPTION(CPACK_RPM_DEBUGINFO_PACKAGE "" ON)
    SET(CPACK_RPM_BUILD_SOURCE_DIRS_PREFIX "/usr/src/debug/${CPACK_RPM_PACKAGE_NAME}-${CPACK_RPM_PACKAGE_VERSION}")
  ENDIF()
  IF(CMAKE_VERSION VERSION_GREATER "3.9.99")

    SET(CPACK_SOURCE_GENERATOR "RPM")
    #SET(CPACK_RPM_BUILDREQUIRES "cmake;mariadb-connector-c")
    SETA(CPACK_RPM_SOURCE_PKG_BUILD_PARAMS "-DRPM=${RPM}")

    MACRO(ADDIF var)
      IF(DEFINED ${var})
        SETA(CPACK_RPM_SOURCE_PKG_BUILD_PARAMS "-D${var}=${${var}}")
      ENDIF()
    ENDMACRO()

    ADDIF(CMAKE_BUILD_TYPE)
    ADDIF(BUILD_CONFIG)
    ADDIF(MARIADB_LINK_DYNAMIC)
    #ADDIF(CMAKE_C_FLAGS_RELWITHDEBINFO)
    #ADDIF(DCMAKE_CXX_FLAGS_RELWITHDEBINFO)
    #ADDIF(WITH_SSL)

    INCLUDE(build_depends)
    MESSAGE(STATUS "Build dependencies of the source RPM are: ${CPACK_RPM_BUILDREQUIRES}")
    MESSAGE(STATUS "Cmake params for build from source RPM: ${CPACK_RPM_SOURCE_PKG_BUILD_PARAMS}")

  ENDIF()
ENDIF()
IF("${CPACK_GENERATOR}" STREQUAL "TGZ" OR "${CPACK_GENERATOR}" STREQUAL "ZIP")
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/install_test/CMakeLists.txt.in
                 ${CMAKE_CURRENT_SOURCE_DIR}/install_test/CMakeLists.txt @ONLY)
ENDIF()
MESSAGE(STATUS "Package Name: ${CPACK_PACKAGE_FILE_NAME} Generator: ${CPACK_GENERATOR}")
#
INCLUDE(CPack)
