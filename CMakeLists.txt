cmake_minimum_required (VERSION 3.2)

project(sqlite_orm)

option(SqliteOrm_BuildTests "Build sqlite_orm unit tests" ON)

set(SqliteOrm_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Tests
include(CTest)
if(SqliteOrm_BuildTests AND BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(examples)

