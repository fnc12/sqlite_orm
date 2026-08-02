cmake_minimum_required (VERSION 3.16)

# Note: find_package(SQLite3 REQUIRED) already done in top-level CMakeLists
FetchContent_MakeAvailable(Catch2)

option(SQLITE_ORM_OMITS_CODECVT "Omits codec testing" OFF)

include(ucm)


# Consuming the standard library as C++ named module is a C++23 standard library feature and additionally requires support from build systems.
# At the time of adding support for sqlite_orm as a C++ named module, CMake has still only experimental features to
# effect the provision of the the C++ Standard Library Modules [see `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD`].
# 
# However, Visual C++ has an option that instructs MSBuild to automatically provision the standard modules,
# and this option is ON by default when using the `/std:c++latest` compiler switch:
# "Build ISO C++23 Standard Library Modules", MSBuild property `BuildStlModules`.
if(CMAKE_CXX_STANDARD GREATER_EQUAL 23 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.28.2 AND MSVC AND MSVC_VERSION GREATER_EQUAL 1930)
    add_executable(module_tests
        named_module/named_module.cpp
    )

    target_sources(module_tests
        PRIVATE FILE_SET all_modules TYPE CXX_MODULES
        BASE_DIRS ../modules
        FILES ../modules/sqlite_orm.ixx
    )

    target_compile_features(module_tests PUBLIC cxx_std_23)

    if(SQLITE_ORM_OMITS_CODECVT)
        message(STATUS "SQLITE_ORM_OMITS_CODECVT is enabled")
        target_compile_definitions(module_tests PRIVATE SQLITE_ORM_OMITS_CODECVT=1)
    endif()

    target_precompile_headers(module_tests PRIVATE
        <sqlite3.h>)

    # note: sqlite3 already linked in top-level CMakeLists
    target_link_libraries(module_tests PRIVATE sqlite_orm)

    add_test(NAME "module_tests"
        COMMAND module_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Glob all .cpp files recursively in the current directory
file(GLOB_RECURSE UNIT_TEST_SOURCES "*.cpp")
ucm_remove_directories(named_module FROM UNIT_TEST_SOURCES)
# Download .c file for the generate_series module
file(DOWNLOAD "https://raw.githubusercontent.com/sqlite/sqlite/refs/tags/version-3.50.4/ext/misc/series.c" "${CMAKE_CURRENT_SOURCE_DIR}/.sqlite3/ext/misc/series.c")
# Build series.c as a static module, which will be directly initialized and used by the unit tests target
set_source_files_properties(.sqlite3/ext/misc/series.c PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
    COMPILE_DEFINITIONS "SQLITE_CORE")

add_executable(unit_tests ${UNIT_TEST_SOURCES} .sqlite3/ext/misc/series.c)

if(SQLITE_ORM_OMITS_CODECVT)
    message(STATUS "SQLITE_ORM_OMITS_CODECVT is enabled")
    target_compile_definitions(unit_tests PRIVATE SQLITE_ORM_OMITS_CODECVT=1)
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
        # disabled due to unit tests testing overaligned structs
        -Wno-interference-size
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
