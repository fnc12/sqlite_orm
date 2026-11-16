# Examples of embedding the asynchronous storage into the event loop an application already has.

function(add_async_event_loop_example name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE sqlite_orm ${ARGN})
endfunction()

add_async_event_loop_example(async_epoll epoll.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_STANDARD GREATER_EQUAL 20)
    # libhv is built from source as a static library, and only when an example needs it.
    # Its options are plain variables here, which its older CMake policy settings would otherwise ignore.
    include(FetchContent)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(BUILD_SHARED OFF)
    set(BUILD_STATIC ON)
    set(BUILD_EXAMPLES OFF)
    set(WITH_HTTP_CLIENT OFF)
    set(libhv_content
        GIT_REPOSITORY https://github.com/ithewei/libhv.git
        GIT_TAG v1.3.4
        GIT_SHALLOW ON
    )
    if(NOT CMAKE_VERSION VERSION_LESS 3.28)
        # keeps libhv out of the `all` and `install` targets
        list(APPEND libhv_content EXCLUDE_FROM_ALL)
    endif()
    FetchContent_Declare(libhv ${libhv_content})
    FetchContent_MakeAvailable(libhv)
    add_async_event_loop_example(async_libhv libhv.cpp hv_static)

    # Asio is header-only: the example is built where Boost.Asio or standalone Asio is installed
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
