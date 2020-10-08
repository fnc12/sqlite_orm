# find_package exposes targets only globally, but caches them. So call find_package again in the directory of usage
find_package(SQLite3 REQUIRED)