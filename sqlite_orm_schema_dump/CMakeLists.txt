cmake_minimum_required (VERSION 3.2)

project(sqlite_orm_schema_dump)

add_subdirectory(third_party/nlohmann_json-3.7.0)
add_subdirectory(third_party/inja-2.1)

add_executable(${PROJECT_NAME} main.cpp database_inspector.cpp database_converter.cpp
    init_code.mustache implementation.mustache header.mustache)
target_link_libraries(${PROJECT_NAME} sqlite3 inja)

