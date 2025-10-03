#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
#include <type_traits>  //  std::false_type, std::true_type
#include <tuple>  //  std::make_tuple
#include <utility>  //  std::move
#endif
#endif

#include "../functional/gsl.h"
#include "../schema/virtual_table.h"
#include "../schema/column.h"
#include "../column_pointer.h"

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
namespace sqlite_orm::internal {
    struct dbstat_module_tag {
        // simplify conceptual/meta programming
        using module_type = dbstat_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "dbstat";
        }
    };

    template<>
    struct virtual_table_module_traits<dbstat_module_tag> {
        using module_type = dbstat_module_tag;
        using is_eponymous = std::true_type;
        using is_without_rowid = std::false_type;
        using omit_column_type = std::true_type;
    };

    template<class... Cs>
    struct virtual_table_traits<dbstat_module_tag, Cs...> : virtual_table_module_traits<dbstat_module_tag> {
        using definition_type = table_definition<Cs...>;
        using elements_type = typename definition_type::elements_type;
    };

    template<class... Cs>
    inline virtual_table_definition<dbstat_module_tag, Cs...> using_dbstat(Cs... columns) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {{std::make_tuple(std::move(columns)...)}});
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    inline constexpr orm_table_reference auto dbstat_table = c<dbstat>();
#endif

    /**
     *  Factory function for a DBSTAT virtual table definition.
     *  If no schema is specified then the main schema is used.
     *  
     *  Though the DBSTAT virtual table is an eponymous table SQLite allows to create a virtual table instance with a different name.
     *  This is mostly useful with binding input arguments, e.g. a different schema than "main", which is yet unimplemented.
     */
    inline auto using_dbstat() {
        return internal::using_dbstat(make_column("name", &dbstat::name),
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

    /**
     *  Factory function for the DBSTAT default eponymous virtual table.
     */
    inline auto make_dbstat_table() {
        return make_virtual_table<dbstat>(internal::dbstat_module_tag::name(), using_dbstat());
    }
}
#endif  //  SQLITE_ENABLE_DBSTAT_VTAB
