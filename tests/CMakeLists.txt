cmake_minimum_required (VERSION 3.16)

# Note: find_package(SQLite3 REQUIRED) already done in top-level CMakeLists
FetchContent_MakeAvailable(Catch2)

option(SQLITE_ORM_OMITS_CODECVT "Omits codec testing" OFF)


# Consuming the standard library as C++ named module is a C++23 standard library feature and additionally requires support from build systems.
# At the time of adding support for sqlite_orm as a C++ named module, CMake has still only experimental features to
# effect the provision of the the C++ Standard Library Modules [see `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD`].
# 
# However, Visual C++ has an option that instructs MSBuild to automatically provision the standard modules,
# and this option is ON by default when using the `/std:c++latest` compiler switch:
# "Build ISO C++23 Standard Library Modules", MSBuild property `BuildStlModules`.
if(SQLITE_ORM_ENABLE_CXX_23 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.28.2 AND MSVC AND MSVC_VERSION GREATER_EQUAL 1930)
    add_executable(module_tests
        named_module.cpp
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

add_executable(unit_tests ${UNIT_TEST_SOURCES})

if(SQLITE_ORM_OMITS_CODECVT)
    message(STATUS "SQLITE_ORM_OMITS_CODECVT is enabled")
    target_compile_definitions(unit_tests PRIVATE SQLITE_ORM_OMITS_CODECVT=1)
endif()

if (MSVC)
    target_compile_options(unit_tests PUBLIC
        # multi-processor compilation
        /MP)
    if (MSVC_VERSION LESS_EQUAL 1900)
        target_compile_options(unit_tests PUBLIC
            # C4503: decorated name length exceeded
            /wd4503
            # C4800: forcing value to bool (performance warning)
            /wd4800)
    else()
        target_compile_options(unit_tests PUBLIC
            # warning-level 4
            /W4
            # C4127: conditional expression is constant
            /wd4127
            # C4456: declaration of 'symbol' hides previous local declaration
            /wd4456
            # C4458: declaration of 'symbol' hides class member
            /wd4458)
    endif()
    if (CMAKE_CXX_FLAGS MATCHES "/D_UNICODE")
        # explicitly set the entry point of the executable file,
        # otherwise for some reason the linker will not pick up `wmain`, which is provided by the static Catch2 library
        target_link_options(unit_tests PRIVATE "/ENTRY:wmainCRTStartup")
    endif()
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
