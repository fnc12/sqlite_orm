cmake_minimum_required (VERSION 3.2)

file(GLOB files "*.cpp")
foreach(file ${files})
    get_filename_component(file_basename ${file} NAME_WE)
    add_executable(${file_basename} ${file})
    set_target_properties(${file_basename} PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON
    )
    target_include_directories(${file_basename} PRIVATE ${SqliteOrm_INCLUDE})
    target_link_libraries(${file_basename} PRIVATE dl sqlite3 pthread)
endforeach()
