#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
#include <string>  //  std::string
#endif
#endif

#include "../schema/column.h"
#include "../schema/table.h"
#include "../column_pointer.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
    struct dbstat {
        std::string name;
        std::string path;
        int pageno = 0;
        std::string pagetype;
        int ncell = 0;
        int payload = 0;
        int unused = 0;
        int mx_payload = 0;
        int pgoffset = 0;
        int pgsize = 0;
    };

    inline auto make_dbstat_table() {
        return make_table("dbstat",
                          make_column("name", &dbstat::name),
                          make_column("path", &dbstat::path),
                          make_column("pageno", &dbstat::pageno),
                          make_column("pagetype", &dbstat::pagetype),
                          make_column("ncell", &dbstat::ncell),
                          make_column("payload", &dbstat::payload),
                          make_column("unused", &dbstat::unused),
                          make_column("mx_payload", &dbstat::mx_payload),
                          make_column("pgoffset", &dbstat::pgoffset),
                          make_column("pgsize", &dbstat::pgsize));
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    inline constexpr orm_table_reference auto dbstat_table = c<dbstat>();
#endif
#endif  //  SQLITE_ENABLE_DBSTAT_VTAB
}
