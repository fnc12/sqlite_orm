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
