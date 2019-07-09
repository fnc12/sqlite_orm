cmake_minimum_required (VERSION 3.2)

option(SQLITE_ORM_OMITS_CODECVT "Omits codec testing" OFF)

option(SqliteOrm_SysSqlite "Use system version of sqlite library" OFF)

if(SqliteOrm_SysSqlite)
    message(FATAL_ERROR "WIP: please, disable the SqliteOrm_SysSqlite option.")
else()
    add_subdirectory(third_party/sqlite)
endif()

add_executable(unit_tests tests.cpp)
add_executable(unit_tests tests2.cpp)
add_executable(unit_tests tests3.cpp)
add_executable(unit_tests tests4.cpp)
add_executable(unit_tests tests4.cpp)
add_executable(unit_tests private_getters_tests.cpp)
add_executable(unit_tests pragma_tests.cpp)
add_executable(unit_tests explicit_columns.cpp)
add_executable(unit_tests core_functions_tests.cpp)


if(SQLITE_ORM_OMITS_CODECVT)
	message(STATUS "SQLITE_ORM_OMITS_CODECVT is enabled")
	target_compile_definitions(unit_tests PRIVATE SQLITE_ORM_OMITS_CODECVT=1)
endif()

find_package(Catch2 REQUIRED)
target_link_libraries(unit_tests PRIVATE sqlite_orm sqlite3 Catch2::Catch2)

enable_testing()

add_test(NAME "All_in_one_unit_test"
    COMMAND unit_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
