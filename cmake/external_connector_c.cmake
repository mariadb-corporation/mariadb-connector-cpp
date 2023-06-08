include(FindPackageHandleStandardArgs)

find_path(MARIADB_CLIENT_INCLUDE_DIR
	NAMES mysql.h mariadb_version.h
	PATH_SUFFIXES mariadb
	NO_SYSTEM_ENVIRONMENT_PATH
	NO_CMAKE_SYSTEM_PATH)
find_library(MARIADB_CLIENT_LIBRARIES
	NAMES mariadb
	PATH_SUFFIXES lib/mariadb
	NO_SYSTEM_ENVIRONMENT_PATH
	NO_CMAKE_SYSTEM_PATH)

find_package_handle_standard_args(MARIADB_CLIENT
	REQUIRED_VARS MARIADB_CLIENT_INCLUDE_DIR MARIADB_CLIENT_LIBRARIES)

if (MARIADB_CLIENT_FOUND)
	message("# find mariadb include dir: ${MARIADB_CLIENT_INCLUDE_DIR}")
	message("# find mariadb libraries: ${MARIADB_CLIENT_LIBRARIES}")
else()
	message("${MARIADB_CLIENT_INCLUDE_DIR}")
	message("${MARIADB_CLIENT_LIBRARIES}")
	message(FATAL_ERROR "Failed found mariadb")
endif()

