cmake_minimum_required (VERSION 3.16)

# Note: find_package(SQLite3 REQUIRED) already done in top-level CMakeLists
FetchContent_MakeAvailable(Catch2)

option(SQLITE_ORM_BUILD_MODULE_TESTS "Build the named module tests where the toolchain provides the standard library modules" ON)

include(ucm)

if(SQLITE_ORM_BUILD_MODULE_TESTS AND TARGET sqlite_orm_module)
    message(STATUS "SQLITE_ORM: Build module tests")

    add_executable(module_tests
        named_module/named_module.cpp
    )

    # note: the sources outside of a module file set are scanned for imports only with policy CMP0155,
    # which the minimum required CMake version leaves unset
    set_target_properties(module_tests PROPERTIES
        CXX_SCAN_FOR_MODULES ON)

    target_precompile_headers(module_tests PRIVATE
        <sqlite3.h>)

    target_link_libraries(module_tests PRIVATE sqlite_orm_module)

    add_test(NAME "module_tests"
        COMMAND module_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
elseif(SQLITE_ORM_BUILD_MODULE_TESTS AND CMAKE_CXX_STANDARD GREATER_EQUAL 23)
    message(STATUS "SQLITE_ORM: No module tests, the toolchain does not provide the standard library modules")
endif()

# Glob all .cpp files recursively in the current directory
file(GLOB_RECURSE UNIT_TEST_SOURCES "*.cpp")
ucm_remove_directories(named_module FROM UNIT_TEST_SOURCES)
# Download .c file for the generate_series module.
# The tag should roughly match the version of the SQLite library being linked,
# since series.c uses API constants of the SQLite version it is shipped with.
set(SQLITE_ORM_SERIES_C_TAG "version-3.50.4" CACHE STRING "sqlite/sqlite repository tag to download ext/misc/series.c from")
file(DOWNLOAD "https://raw.githubusercontent.com/sqlite/sqlite/refs/tags/${SQLITE_ORM_SERIES_C_TAG}/ext/misc/series.c" "${CMAKE_CURRENT_SOURCE_DIR}/.sqlite3/ext/misc/series.c")
# Build series.c as a static module, which will be directly initialized and used by the unit tests target
set_source_files_properties(.sqlite3/ext/misc/series.c PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
    COMPILE_DEFINITIONS "SQLITE_CORE")

add_executable(unit_tests ${UNIT_TEST_SOURCES} .sqlite3/ext/misc/series.c)

# Build series.c a second time as a run-time loadable extension for the `load extension` tests.
# Not on Apple platforms, whose system SQLite has the extension loading API omitted
# [see `SQLITE_ORM_LOAD_EXTENSION_SUPPORTED` in sqlite3/sqlite3_config.h].
if(NOT APPLE)
    add_subdirectory(loadable_extension)
    add_dependencies(unit_tests series_extension)
    target_compile_definitions(unit_tests PRIVATE
        SQLITE_ORM_SERIES_EXTENSION_FILE="$<TARGET_FILE_NAME:series_extension>")
endif()

if(MSVC)
    target_compile_options(unit_tests PUBLIC
        # multi-processor compilation
        /MP
        # warning-level 4
        /W4
        # C4456: declaration of 'symbol' hides previous local declaration
        /wd4456
        # C4458: declaration of 'symbol' hides class member
        /wd4458
        # C4458: 'symbol': deprecated
        # disabled due to unit tests testing deprecated features
        /wd4996)
    if (CMAKE_CXX_FLAGS MATCHES "/D_UNICODE")
        # explicitly set the entry point of the executable file,
        # otherwise for some reason the linker will not pick up `wmain`, which is provided by the static Catch2 library
        target_link_options(unit_tests PRIVATE "/ENTRY:wmainCRTStartup")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(unit_tests PUBLIC
        # disabled due to unit tests testing deprecated features
        -Wno-deprecated-declarations
        # perfectly valid to have missing field initializers in library's test code
        -Wno-missing-field-initializers
        # suppress warnings about unused variables in test code
        -Wno-unused-variable)
endif()
    
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 16)
    target_compile_options(unit_tests PUBLIC
        -freflection)
endif()

target_precompile_headers(unit_tests PRIVATE
    <sqlite3.h>
    <sqlite_orm/sqlite_orm.h>
    <catch2/catch_all.hpp>)

# note: sqlite3 already linked in top-level CMakeLists
target_link_libraries(unit_tests PRIVATE sqlite_orm Catch2::Catch2WithMain)

add_test(NAME "All_in_one_unit_test"
    COMMAND unit_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
