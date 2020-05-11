SET(MARIADBCPP_PUBLIC_API   ${CMAKE_SOURCE_DIR}/include/Driver.h
                            ${CMAKE_SOURCE_DIR}/include/Connection.h
                            ${CMAKE_SOURCE_DIR}/include/Statement.h
                            ${CMAKE_SOURCE_DIR}/include/PreparedStatement.h
                            ${CMAKE_SOURCE_DIR}/include/ResultSet.h
                            ${CMAKE_SOURCE_DIR}/include/DatabaseMetaData.h
                            ${CMAKE_SOURCE_DIR}/include/ResultSetMetaData.h
                            ${CMAKE_SOURCE_DIR}/include/CallableStatement.h
                            ${CMAKE_SOURCE_DIR}/include/Exception.h
                            ${CMAKE_SOURCE_DIR}/include/SQLString.h
                            ${CMAKE_SOURCE_DIR}/include/Warning.h
                            ${CMAKE_SOURCE_DIR}/include/ParameterMetaData.h
                            ${CMAKE_SOURCE_DIR}/include/Savepoint.h
                            ${CMAKE_SOURCE_DIR}/include/Types.h
                            ${CMAKE_SOURCE_DIR}/include/jdbccompat.h
                            ${CMAKE_SOURCE_DIR}/include/ConnCpp.h
                            )
SET(MARIADBCPP_COMPAT_STUBS ${CMAKE_SOURCE_DIR}/include/compat/Array.h
                            ${CMAKE_SOURCE_DIR}/include/compat/Executor.h
                            ${CMAKE_SOURCE_DIR}/include/compat/Object.h
                            ${CMAKE_SOURCE_DIR}/include/compat/SQLType.h
                            ${CMAKE_SOURCE_DIR}/include/compat/Struct.h
   )
IF(WIN32)
  SET(WIX_INCLUDES ${MARIADBCPP_PUBLIC_API} {MARIADBCPP_COMPAT_STUBS} PARENT_SCOPE)
ENDIF()

INSTALL(FILES
   ${MARIADBCPP_PUBLIC_API}
   DESTINATION ./${INSTALL_INCLUDE_DIR}
   COMPONENT PublicAPI)
INSTALL(FILES
   ${MARIADBCPP_COMPAT_STUBS}
   DESTINATION ./${INSTALL_INCLUDE_DIR}/compat
   COMPONENT PublicAPI)

