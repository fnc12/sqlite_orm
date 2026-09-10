#pragma once

#include <sqlite3.h>

/*
 *  JSON functions are built into SQLite as of 3.38.0, unless it was built with `SQLITE_OMIT_JSON`.
 *  Before that they had to be requested with `SQLITE_ENABLE_JSON1`, which is a no-op since - which is
 *  why a distribution that has JSON need not define it. vcpkg, for one, reports the feature the modern
 *  way: its `sqlite3-vcpkg-config.h` leaves `SQLITE_OMIT_JSON` undefined and never mentions JSON1.
 *
 *  A distribution that omits JSON therefore has to say so in a header the consumer sees, as vcpkg does
 *  for the features it selects; otherwise sqlite_orm cannot tell and the functions fail to link.
 */
#if defined(SQLITE_ENABLE_JSON1) || (SQLITE_VERSION_NUMBER >= 3038000 && !defined(SQLITE_OMIT_JSON))
#define SQLITE_ORM_JSON_SUPPORTED
#endif

/*
 *  The extension loading API is compiled in unless SQLite was built with `SQLITE_OMIT_LOAD_EXTENSION`.
 *  Apple's system SQLite is such a build, and it also strips `sqlite3_load_extension` from its header
 *  without defining the omit macro, leaving nothing for the preprocessor to test for. Hence extension
 *  loading is off on Apple platforms by default; when building against an unrestricted SQLite there
 *  (e.g. from Homebrew or vcpkg), request it by defining `SQLITE_ORM_ENABLE_LOAD_EXTENSION`.
 */
#if !defined(SQLITE_OMIT_LOAD_EXTENSION) && (!defined(__APPLE__) || defined(SQLITE_ORM_ENABLE_LOAD_EXTENSION))
#define SQLITE_ORM_LOAD_EXTENSION_SUPPORTED
#endif
