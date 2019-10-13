cmake_minimum_required (VERSION 3.2)

project(sqlite_orm_schema_dump)

find_package(mstch REQUIRED)

add_executable(${PROJECT_NAME} main.cpp database_inspector.cpp database_converter.cpp
    init_code.mustache implementation.mustache header.mustache)
target_link_libraries(${PROJECT_NAME} mstch::mstch sqlite3)

