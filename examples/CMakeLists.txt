cmake_minimum_required (VERSION 3.16)

# note: find_package(SQLite3 REQUIRED) already done in top-level CMakeLists

file(GLOB files "*.cpp")
foreach(file ${files})
    get_filename_component(file_basename ${file} NAME_WE)
    add_executable(${file_basename} ${file})
    # note: sqlite3 already linked in top-level CMakeLists
    target_link_libraries(${file_basename} PRIVATE sqlite_orm)
endforeach()
