include(FetchContent)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    # FIND_PACKAGE_ARGS is available since 3.24
    if(CMAKE_VERSION VERSION_LESS 3.24)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.2.1
        )
    else()
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.2.1
            # prefer find_package() over building from source
            FIND_PACKAGE_ARGS 3 CONFIG
        )
    endif()
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    # CMake <3.24: exposes targets only locally, but caches them. So call FetchContent_MakeAvailable again in the directory of usage
    FetchContent_MakeAvailable(Catch2)
endif()

# CMake <3.24: exposes targets only locally, but caches them. So call find_package again in the directory of usage
find_package(SQLite3 REQUIRED)
