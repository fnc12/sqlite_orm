#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#endif

#include "schema/column.h"
#include "schema/table.h"
#include "column_pointer.h"
#include "alias.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  SQLite's "schema table" that stores the schema for a database.
     *  
     *  @note Despite the fact that the schema table was renamed from "sqlite_master" to "sqlite_schema" in SQLite 3.33.0
     *  the renaming process was more like keeping the previous name "sqlite_master" and attaching an internal alias "sqlite_schema".
     *  One can infer this fact from the following SQL statement:
     *  It qualifies the set of columns, but bails out with error "no such table: sqlite_schema": `SELECT sqlite_schema.* from sqlite_schema`.
     *  Hence we keep its previous table name `sqlite_master`, and provide `sqlite_schema` as a table alias in sqlite_orm.
     */
    struct sqlite_master {
        std::string type;
        std::string name;
        std::string tbl_name;
        int rootpage = 0;
        std::string sql;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        friend bool operator==(const sqlite_master&, const sqlite_master&) = default;
#endif
    };

    inline auto make_sqlite_schema_table() {
        return make_table("sqlite_master",
                          make_column("type", &sqlite_master::type),
                          make_column("name", &sqlite_master::name),
                          make_column("tbl_name", &sqlite_master::tbl_name),
                          make_column("rootpage", &sqlite_master::rootpage),
                          make_column("sql", &sqlite_master::sql));
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    inline constexpr orm_table_reference auto sqlite_master_table = c<sqlite_master>();
    inline constexpr orm_table_alias auto sqlite_schema = "sqlite_schema"_alias.for_<sqlite_master>();
#endif
}
