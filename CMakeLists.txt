# note: the minimum required version needs to go hand in hand with appveyor builds,
# which is CMake 3.16 for the Visual Studio 2017 build worker image
cmake_minimum_required (VERSION 3.16)

# PACKAGE_VERSION is used by cpack scripts currently
# Both sqlite_orm_VERSION and PACKAGE_VERSION should be the same for now

set(sqlite_orm_VERSION "1.8.0")
set(PACKAGE_VERSION ${sqlite_orm_VERSION})

project("sqlite_orm" VERSION ${PACKAGE_VERSION})

# Handling C++ standard version to use
option(SQLITE_ORM_ENABLE_CXX_20 "Enable C++ 20" OFF)
option(SQLITE_ORM_ENABLE_CXX_17 "Enable C++ 17" OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(SQLITE_ORM_ENABLE_CXX_20)
    set(CMAKE_CXX_STANDARD 20)
    message(STATUS "SQLITE_ORM: Build with C++20 features")
elseif(SQLITE_ORM_ENABLE_CXX_17)
    set(CMAKE_CXX_STANDARD 17)
    message(STATUS "SQLITE_ORM: Build with C++17 features")
else()
    # fallback to C++14 if there is no special instruction
    set(CMAKE_CXX_STANDARD 14)
    message(STATUS "SQLITE_ORM: Build with C++14 features")
endif()
set(CMAKE_CXX_EXTENSIONS OFF)


set(CMAKE_VERBOSE_MAKEFILE ON)

message(STATUS "Configuring ${PROJECT_NAME} ${sqlite_orm_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(CTest)

### Dependencies
add_subdirectory(dependencies)

### Main Build Targets
set(SqliteOrm_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(sqlite_orm INTERFACE)
add_library(sqlite_orm::sqlite_orm ALIAS sqlite_orm)

find_package(SQLite3 REQUIRED)
target_link_libraries(sqlite_orm INTERFACE SQLite::SQLite3)

target_sources(sqlite_orm INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/sqlite_orm/sqlite_orm.h>)

target_include_directories(sqlite_orm INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

include(ucm)

if (MSVC)
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")    
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")    
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")    
    add_compile_options(/EHsc)
    if (MSVC_VERSION GREATER_EQUAL 1914)
        add_compile_options(/Zc:__cplusplus)
    endif()
    if (MSVC_VERSION GREATER_EQUAL 1910)
        # VC 2017 issues a deprecation warning for `strncpy`
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    endif()
    add_compile_options(/MP) # multi-processor compilation
    if (CMAKE_CXX_STANDARD GREATER 14)
        add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    endif()

    if ("${CMAKE_GENERATOR}" MATCHES "(Win64|x64)")
        message(STATUS "Add /bigobj flag to compiler")
        add_compile_options(/bigobj)
    endif()
endif()

ucm_print_flags()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Tests
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLES "Build code examples" OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

### Packaging
add_subdirectory(packaging)
