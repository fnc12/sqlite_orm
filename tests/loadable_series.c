/*
 *  Compiles the downloaded `ext/misc/series.c` as a run-time loadable extension for the
 *  `load extension` tests - that is, without `SQLITE_CORE`, unlike the statically linked copy
 *  built into the unit tests, which the `SQLITE_CORE` source file property applies to.
 */
#include ".sqlite3/ext/misc/series.c"
