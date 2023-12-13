include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

# We need some extra FAIL_REGEX patterns
# Note that CHECK_C_SOURCE_COMPILES is a misnomer, it will also link.
SET(fail_patterns
    FAIL_REGEX "argument unused during compilation"
    FAIL_REGEX "unsupported .*option"
    FAIL_REGEX "unknown .*option"
    FAIL_REGEX "unrecognized .*option"
    FAIL_REGEX "ignoring unknown option"
    FAIL_REGEX "warning:.*ignored"
    FAIL_REGEX "warning:.*is valid for.*but not for"
    FAIL_REGEX "warning:.*redefined"
    FAIL_REGEX "[Ww]arning: [Oo]ption"
    )
#The regex patterns above are not localized, thus LANG=C
SET(ENV{LANG} C)
MACRO (MY_CHECK_C_COMPILER_FLAG flag)
  STRING(REGEX REPLACE "[-,= +]" "_" result "have_C_${flag}")
  SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${flag}")
  CHECK_C_SOURCE_COMPILES("int main(void) { return 0; }" ${result}
    ${fail_patterns})
  SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
ENDMACRO()

MACRO (MY_CHECK_CXX_COMPILER_FLAG flag)
  STRING(REGEX REPLACE "[-,= +]" "_" result "have_CXX_${flag}")
  SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${flag}")
  CHECK_CXX_SOURCE_COMPILES("int main(void) { return 0; }" ${result}
    ${fail_patterns})
  SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
ENDMACRO()

FUNCTION(MY_CHECK_AND_SET_COMPILER_FLAG flag_to_set)
  # At the moment this is gcc-only.
  # Let's avoid expensive compiler tests on Windows:
  IF(WIN32)
    RETURN()
  ENDIF()
  STRING(REGEX REPLACE "^-Wno-" "-W" flag_to_check ${flag_to_set})

  LIST(FIND MAODBC_LANGUAGES "C" LANGIDX)
  IF(NOT LANGIDX EQUAL -1)
    MY_CHECK_C_COMPILER_FLAG(${flag_to_check})
  ENDIF()
  LIST(FIND MAODBC_LANGUAGES "CXX" LANGIDX)
  IF(NOT LANGIDX EQUAL -1)
    MY_CHECK_CXX_COMPILER_FLAG(${flag_to_check})
  ENDIF()

  STRING(REGEX REPLACE "[-,= +]" "_" result "${flag_to_check}")
  MESSAGE(STATUS "Langs: ${MAODBC_LANGUAGES}")
  FOREACH(lang ${MAODBC_LANGUAGES})
    IF (have_${lang}_${result})
      IF(ARGN)
        FOREACH(type ${ARGN})
          SET(CMAKE_${lang}_FLAGS_${type} "${CMAKE_${lang}_FLAGS_${type}} ${flag_to_set}" PARENT_SCOPE)
        ENDFOREACH()
      ELSE()
        SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} ${flag_to_set}" PARENT_SCOPE)
      ENDIF()
    ENDIF()
  ENDFOREACH()
ENDFUNCTION()

FUNCTION(MA_SET_COMPILER_FLAG flag_to_set)
  IF(MSVC)
    STRING(REGEX REPLACE "-" "/" result_flag "${flag_to_set}")
  ENDIF()
  FOREACH(lang ${MAODBC_LANGUAGES})
    IF(ARGN)
      FOREACH(type ${ARGN})
        STRING(REGEX REPLACE "${result_flag}" "" CMAKE_${lang}_FLAGS_${type} "${CMAKE_${lang}_FLAGS_${type}}")
        SET(CMAKE_${lang}_FLAGS_${type} "${CMAKE_${lang}_FLAGS_${type}} ${result_flag}" PARENT_SCOPE)
      ENDFOREACH()
    ELSE()
      STRING(REGEX REPLACE "${result_flag}" "" CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS}")
      SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} ${result_flag}" PARENT_SCOPE)
    ENDIF()
  ENDFOREACH()
ENDFUNCTION()

FUNCTION(MA_SET_LINKER_FLAG flag_to_set)
  IF(MSVC)
    STRING(REGEX REPLACE ":NO" "" flag_to_check ${flag_to_set})
    STRING(REGEX REPLACE "-" "/" result_flag "${flag_to_set}")
  ELSE()
    STRING(REGEX REPLACE "^-Wno-" "-W" flag_to_check ${flag_to_set})
    SET(result_flag "${flag_to_set}")
  ENDIF()
  IF(ARGN)
    FOREACH(type ${ARGN})
      # LIST(REMOVE_ITEM could be better here and in similar places above
      STRING(REGEX REPLACE "${flag_to_set}" "" CMAKE_EXE_LINKER_FLAGS_${type} "${CMAKE_EXE_LINKER_FLAGS_${type}}")
      STRING(REGEX REPLACE "${flag_to_set}" "" CMAKE_SHARED_LINKER_FLAGS_${type} "${CMAKE_SHARED_LINKER_FLAGS_${type}}")
      STRING(REGEX REPLACE "${flag_to_set}" "" CMAKE_MODULE_LINKER_FLAGS_${type} "${CMAKE_MODULE_LINKER_FLAGS_${type}}")
      STRING(REGEX REPLACE "${flag_to_check}" "" CMAKE_EXE_LINKER_FLAGS_${type} "${CMAKE_EXE_LINKER_FLAGS_${type}}")
      STRING(REGEX REPLACE "${flag_to_check}" "" CMAKE_SHARED_LINKER_FLAGS_${type} "${CMAKE_SHARED_LINKER_FLAGS_${type}}")
      STRING(REGEX REPLACE "${flag_to_check}" "" CMAKE_MODULE_LINKER_FLAGS_${type} "${CMAKE_MODULE_LINKER_FLAGS_${type}}")
      #MESSAGE(STATUS "Before: ${CMAKE_EXE_LINKER_FLAGS_${type}} ${CMAKE_SHARED_LINKER_FLAGS_${type}} ${CMAKE_MODULE_LINKER_FLAGS_${type}}")
      SET(CMAKE_EXE_LINKER_FLAGS_${type} "${CMAKE_EXE_LINKER_FLAGS_${type}} ${result_flag}" PARENT_SCOPE)
      SET(CMAKE_SHARED_LINKER_FLAGS_${type} "${CMAKE_SHARED_LINKER_FLAGS_${type}} ${result_flag}" PARENT_SCOPE)
      SET(CMAKE_MODULE_LINKER_FLAGS_${type} "${CMAKE_MODULE_LINKER_FLAGS_${type}} ${result_flag}" PARENT_SCOPE)
      #MESSAGE(STATUS "After: ${CMAKE_EXE_LINKER_FLAGS_${type}} ${CMAKE_SHARED_LINKER_FLAGS_${type}} ${CMAKE_MODULE_LINKER_FLAGS_${type}}, Expected: ${CMAKE_SHARED_LINKER_FLAGS_${type}} ${result_flag}")
    ENDFOREACH()
 ELSE()
   SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${result_flag}" PARENT_SCOPE)
 ENDIF()
ENDFUNCTION()
