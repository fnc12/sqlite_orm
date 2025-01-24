cmake_minimum_required(VERSION 3.16)

# note: find_package(SQLite3 REQUIRED) already done in top-level CMakeLists

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB files "*.cpp")

set(run_example_targets)

foreach(file ${files})
    get_filename_component(file_basename ${file} NAME_WE)

    add_executable(${file_basename} ${file})
    # note: sqlite3 already linked in top-level CMakeLists

    target_link_libraries(${file_basename} PRIVATE sqlite_orm)

    add_custom_target(run_${file_basename}
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${file_basename}
        DEPENDS ${file_basename}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running example: ${file_basename}"
        VERBATIM
    )

    list(APPEND run_example_targets run_${file_basename})
endforeach()

add_custom_target(run_all_examples
    DEPENDS ${run_example_targets}
)
