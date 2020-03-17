# Macro that checks if variable is defined, otherwise checks env for same name, otherwise uses default value
MACRO(SET_VALUE _variable _default_value)
  IF (NOT ${_variable})
    IF(DEFINED ENV{${_variable}})
      SET(${_variable} $ENV{${_variable}})
    ELSE()
      SET(${_variable} ${_default_value})
    ENDIF()
  ENDIF()
ENDMACRO(SET_VALUE)

