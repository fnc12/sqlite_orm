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

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/sqlite_orm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# install C++ named module interface unit file and possible various accompanying files produced by CMake.
# note that there is no convention for the install location of module source files at the time of writing.
# the module source file will get installed to subfolder "${SQLITE_ORM_INSTALL_CMAKEDIR}/src" following Craig Scott's [writeup](https://crascit.com/2024/04/04/cxx-modules-cmake-shared-libraries/#h-installing-shared-libraries-with-c-20-modules).
# also see @starball's detailed stackoverflow [answer](https://stackoverflow.com/a/78209338/279251).
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28.2)
    install(TARGETS sqlite_orm
        FILE_SET CXX_MODULES
            DESTINATION ${SQLITE_ORM_INSTALL_CMAKEDIR}/src
    )
    install(EXPORT SqliteOrmTargets
        DESTINATION ${SQLITE_ORM_INSTALL_CMAKEDIR}
        CXX_MODULES_DIRECTORY .
    )
endif()

install(EXPORT SqliteOrmTargets
	DESTINATION ${SQLITE_ORM_INSTALL_CMAKEDIR}
	NAMESPACE sqlite_orm::
	COMPONENT   sqlite_orm_development
)