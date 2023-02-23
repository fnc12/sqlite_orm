# CMake <3.24: exposes targets only locally, but caches them. So call find_package again in the directory of usage
find_package(SQLite3 CONFIG REQUIRED)
