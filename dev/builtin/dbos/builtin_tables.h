#pragma once

/** @file The tuple of the database objects SQLite provides itself, `make_builtin_tables()`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple, std::tuple_cat
#endif

#include "sqlite_schema.h"
#include "dbstat.h"
#include "pragma_table_info.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for the tables SQLite provides itself, as far as available:
     *  the schema table, the DBSTAT virtual table (`SQLITE_ENABLE_DBSTAT_VTAB`)
     *  and the eponymous virtual tables of the PRAGMA functions (see `make_pragma_tables()`).
     *
     *  Eponymous virtual tables of extensions, like `generate_series`, are not included.
     *
     *  E.g.
     *  auto storage = make_storage("", make_builtin_tables(), make_table<User>());
     */
    inline auto make_builtin_tables() {
        return std::tuple_cat(std::tuple{make_sqlite_schema_table()},
#ifdef SQLITE_ORM_DBSTAT_SUPPORTED
                              std::tuple{make_dbstat_table()},
#endif
#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ORM_REFLECTION_SUPPORTED)
                              make_pragma_tables(),
#endif
                              std::tuple<>{});
    }
}
