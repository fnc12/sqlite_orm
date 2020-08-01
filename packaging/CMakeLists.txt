include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(SQLITE_ORM_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/SqliteOrm CACHE STRING "Path to SqliteOrm cmake files")

configure_file(${PROJECT_SOURCE_DIR}/cmake/SqliteOrmConfig.cmake.in SqliteOrmConfig.cmake @ONLY)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/SqliteOrmConfigVersion.cmake
				VERSION ${sqlite_orm_VERSION}
				COMPATIBILITY SameMajorVersion
				)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SqliteOrmConfig.cmake
				${CMAKE_CURRENT_BINARY_DIR}/SqliteOrmConfigVersion.cmake
				DESTINATION ${SQLITE_ORM_INSTALL_CMAKEDIR})

install(TARGETS sqlite_orm 
	EXPORT SqliteOrmTargets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} 
        COMPONENT sqlite_orm_runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}             
        COMPONENT          sqlite_orm_runtime           
    	NAMELINK_COMPONENT sqlite_orm_development   
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}           
        COMPONENT   sqlite_orm_development 
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/sqlite_orm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT SqliteOrmTargets
	DESTINATION ${SQLITE_ORM_INSTALL_CMAKEDIR}
	NAMESPACE sqlite_orm::
	COMPONENT   sqlite_orm_development
)