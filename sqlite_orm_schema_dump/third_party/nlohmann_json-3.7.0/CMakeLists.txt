cmake_minimum_required(VERSION 3.1)

project(nlohmann_json VERSION 3.7.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

