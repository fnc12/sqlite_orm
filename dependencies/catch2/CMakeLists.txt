# CMake <3.24: exposes targets only locally, but caches them. So call FetchContent_MakeAvailable again in the directory of usage
FetchContent_MakeAvailable(Catch2)
