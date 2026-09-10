cmake_minimum_required(VERSION 3.16)

# Builds the downloaded series.c a second time as a run-time loadable extension for the
# `load extension` tests - in a directory of its own, so that the `SQLITE_CORE` source file
# property of the statically linked copy in the parent directory does not apply.
add_library(series_extension MODULE ${CMAKE_CURRENT_SOURCE_DIR}/../.sqlite3/ext/misc/series.c)
set_target_properties(series_extension PROPERTIES
    PREFIX ""
    OUTPUT_NAME "series"
    # the `$<1:>` wrapper suppresses the per-configuration subdirectory of multi-config generators,
    # placing the extension in the working directory of the test executable
    LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}/..>")
# note: only for the include directories; a loadable extension calls SQLite through a thunk
target_link_libraries(series_extension PRIVATE SQLite3::SQLite3)
