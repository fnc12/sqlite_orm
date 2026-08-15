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
    
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${file_basename} PUBLIC
            # disabled due to examples potentially containing deprecated features
            -Wno-deprecated-declarations
            # disabled due to unit tests testing overaligned structs
            -Wno-interference-size)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 16)
        target_compile_options(${file_basename} PUBLIC
            -freflection)
    endif()

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
