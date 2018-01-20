cmake_minimum_required (VERSION 3.2)

option(SqliteOrm_SysSqlite "Use system version of sqlite library" OFF)

if(SqliteOrm_SysSqlite)
    message(FATAL_ERROR "WIP: please, disable the SqliteOrm_SysSqlite option.")
else()
    add_subdirectory(third_party/sqlite)
endif()

set(SqliteOrm_UnitTarget unit_tests)

add_executable(${SqliteOrm_UnitTarget} tests.cpp)

target_include_directories(${SqliteOrm_UnitTarget} PRIVATE ${SqliteOrm_INCLUDE})

target_link_libraries(${SqliteOrm_UnitTarget} PRIVATE dl sqlite3 pthread)

set_target_properties(${SqliteOrm_UnitTarget} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON)

add_test(NAME "All_in_one_unit_test"
    COMMAND ${SqliteOrm_UnitTarget}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
