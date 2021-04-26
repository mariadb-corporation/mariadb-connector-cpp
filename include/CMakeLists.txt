SET(MARIADBCPP_PUBLIC_API   ${CMAKE_SOURCE_DIR}/include/conncpp.hpp)

SET(MARIADBCPP_PUBLIC_CLASSES ${CMAKE_SOURCE_DIR}/include/conncpp/Driver.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/DriverManager.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/Connection.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/Statement.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/PreparedStatement.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/ResultSet.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/DatabaseMetaData.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/ResultSetMetaData.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/CallableStatement.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/Exception.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/SQLString.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/Warning.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/ParameterMetaData.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/Savepoint.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/Types.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/jdbccompat.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/buildconf.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/CArray.hpp
                            )

SET(MARIADBCPP_COMPAT_STUBS ${CMAKE_SOURCE_DIR}/include/conncpp/compat/Array.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/compat/Executor.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/compat/Object.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/compat/SQLType.hpp
                            ${CMAKE_SOURCE_DIR}/include/conncpp/compat/Struct.hpp
   )
IF(WIN32)
  SET(WIX_INCLUDES ${MARIADBCPP_PUBLIC_API} MARIADBCPP_PUBLIC_CLASSES {MARIADBCPP_COMPAT_STUBS} PARENT_SCOPE)
ENDIF()

INSTALL(FILES
   ${MARIADBCPP_PUBLIC_API}
   DESTINATION ${INSTALL_INCLUDEDIR}
   COMPONENT PublicAPI)
INSTALL(FILES
   ${MARIADBCPP_PUBLIC_CLASSES}
   DESTINATION ${INSTALL_INCLUDEDIR}/conncpp
   COMPONENT PublicAPI)
INSTALL(FILES
   ${MARIADBCPP_COMPAT_STUBS}
   DESTINATION ${INSTALL_INCLUDEDIR}/conncpp/compat
   COMPONENT PublicAPI)

