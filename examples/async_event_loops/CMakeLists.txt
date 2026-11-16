# Examples of embedding the asynchronous storage into the event loop an application already has.
# Those for libhv and Asio are built where the library is found.

function(add_async_event_loop_example name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE sqlite_orm ${ARGN})
endfunction()

add_async_event_loop_example(async_epoll epoll.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_STANDARD GREATER_EQUAL 20)
    # libhv's package configuration links OpenSSL without looking for it when built with it
    find_package(OpenSSL QUIET)
    find_package(libhv CONFIG QUIET)
    if(TARGET hv)
        add_async_event_loop_example(async_libhv libhv.cpp hv)
    elseif(TARGET hv_static)
        add_async_event_loop_example(async_libhv libhv.cpp hv_static)
    endif()

    find_package(Threads REQUIRED)
    find_package(Boost CONFIG QUIET)
    find_path(ASIO_INCLUDE_DIR asio.hpp)
    if(Boost_FOUND)
        add_async_event_loop_example(async_asio asio.cpp Boost::headers Threads::Threads)
    elseif(ASIO_INCLUDE_DIR)
        add_async_event_loop_example(async_asio asio.cpp Threads::Threads)
        target_include_directories(async_asio PRIVATE ${ASIO_INCLUDE_DIR})
    endif()
endif()
