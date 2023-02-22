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
            FIND_PACKAGE_ARGS
        )
    endif()
    add_subdirectory(catch2)
endif()

add_subdirectory(sqlite3)
