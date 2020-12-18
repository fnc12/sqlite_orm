include(FetchContent)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.3
    )
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(catch2)
endif()

if(SqliteOrm_SysSqlite)
    find_package(SQLite3 REQUIRED)
else()
    add_subdirectory(sqlite3)
endif()