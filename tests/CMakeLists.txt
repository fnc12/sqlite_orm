cmake_minimum_required (VERSION 3.16)

# Note: find_package(SQLite3 REQUIRED) already done in top-level CMakeLists
FetchContent_MakeAvailable(Catch2)

option(SQLITE_ORM_OMITS_CODECVT "Omits codec testing" OFF)

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
