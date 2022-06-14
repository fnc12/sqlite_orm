cmake_minimum_required (VERSION 3.2)

# added by JDH
set(sqlite_orm_examples_VERSION "1.8.0")
set(PACKAGE_VERSION ${sqlite_orm_examples_VERSION})

project("sqlite_orm_examples" VERSION ${PACKAGE_VERSION})

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

message(STATUS "Configuring ${CMAKE_PROJECT_NAME} ${sqlite_orm_examples_VERSION}")

find_package(SQLite3 REQUIRED)


if (MSVC)
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")    
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")    
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")    
    add_compile_options(/EHsc)
    if (MSVC_VERSION GREATER_EQUAL 1914)
        add_compile_options(/Zc:__cplusplus)
	message(STATUS "Add /Zc:__cplusplus flag to compiler")
    endif()
    add_compile_options(/MP) # multi-processor compilation
    if (CMAKE_CXX_STANDARD GREATER 14)
        add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    endif()

    if (("${CMAKE_GENERATOR}" MATCHES "(Win64|x64)") OR ("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64"))
        message(STATUS "Add /bigobj flag to compiler")
        add_compile_options(/bigobj)
    endif()
endif()

message(STATUS "Generator ${CMAKE_GENERATOR}   ${CMAKE_GENERATOR_PLATFORM}")
# end jdh

file(GLOB files "*.cpp")
foreach(file ${files})
    get_filename_component(file_basename ${file} NAME_WE)
    add_executable(${file_basename} ${file})
    find_package(SQLite3 REQUIRED)
    target_link_libraries(${file_basename} PRIVATE ${SQLite3_LIBRARY})
    target_include_directories( ${file_basename} PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../include>)
endforeach()
