# Macro that checks if variable is defined, otherwise checks env for same name, otherwise uses default value
MACRO(SET_VALUE _variable _default_value _descr)
  IF (NOT ${_variable})
    IF(DEFINED ENV{${_variable}})
      SET(${_variable} $ENV{${_variable}} CACHE STRING "${_descr}")
    ELSE()
      SET(${_variable} ${_default_value} CACHE STRING "${_descr}")
    ENDIF()
  ENDIF()
ENDMACRO(SET_VALUE)

