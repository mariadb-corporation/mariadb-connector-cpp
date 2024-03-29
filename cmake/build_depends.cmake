IF(RPM)
  MACRO(FIND_DEP V)
    SET(out ${V}_DEP)
    IF (NOT DEFINED ${out})
      IF(EXISTS ${${V}} AND NOT IS_DIRECTORY ${${V}})
        EXECUTE_PROCESS(COMMAND ${ARGN} RESULT_VARIABLE res OUTPUT_VARIABLE O OUTPUT_STRIP_TRAILING_WHITESPACE)
      ELSE()
        SET(res 1)
      ENDIF()
      IF (res)
        SET(O)
      ELSE()
        MESSAGE(STATUS "Need ${O} for ${${V}}")
      ENDIF()
      SET(${out} ${O} CACHE INTERNAL "Package that contains ${${V}}" FORCE)
    ENDIF()
  ENDMACRO()

  # FindBoost.cmake doesn't leave any trace, do it here
  IF (Boost_INCLUDE_DIR)
    FIND_FILE(Boost_config_hpp boost/config.hpp PATHS ${Boost_INCLUDE_DIR})
  ENDIF()

  GET_CMAKE_PROPERTY(ALL_VARS CACHE_VARIABLES)
  FOREACH (V ${ALL_VARS})
    GET_PROPERTY(H CACHE ${V} PROPERTY HELPSTRING)
    IF (H MATCHES "^Have library [^/]" AND ${V})
      STRING(REGEX REPLACE "^Have library " "" L ${H})
      SET(V ${L}_LIBRARY)
      FIND_LIBRARY(${V} ${L})
    ENDIF()
    GET_PROPERTY(T CACHE ${V} PROPERTY TYPE)
    IF ((T STREQUAL FILEPATH OR V MATCHES "^CMAKE_COMMAND$") AND ${V} MATCHES "^/")
      IF (RPM)
        FIND_DEP(${V} rpm -q --qf "%{NAME}" -f ${${V}})
      ELSE() # must be DEB
        MESSAGE(FATAL_ERROR "Not implemented")
      ENDIF ()
      SET(BUILD_DEPS ${BUILD_DEPS} ${${V}_DEP})
    ENDIF()
  ENDFOREACH()
  IF (BUILD_DEPS)
    SET(BUILD_DEPS "${CPACK_RPM_BUILDREQUIRES}" "${BUILD_DEPS}")
    LIST(REMOVE_DUPLICATES BUILD_DEPS)
    STRING(REPLACE ";" " " CPACK_RPM_BUILDREQUIRES "${BUILD_DEPS}")
  ENDIF()
ENDIF(RPM)
