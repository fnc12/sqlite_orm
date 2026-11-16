#pragma once

/** @file The types of the SQLite C API that sqlite_orm republishes, under the names it publishes them by.
 */

#include <sqlite3.h>

SQLITE_ORM_EXPORT namespace sqlite_orm {
    using int64 = sqlite_int64;
    using uint64 = sqlite_uint64;
}
