#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
#include <type_traits>  //  std::false_type, std::true_type, std::is_convertible
#include <tuple>  //  std::tuple_size, std::make_tuple, std::tuple
#include <utility>  //  std::move, std::index_sequence_for
#include <string>  //  std::string
#endif
#endif

#include "../functional/gsl.h"
#include "../tuple_helper/tuple_filter.h"
#include "../member_traits/member_traits.h"
#include "../schema/virtual_table.h"
#include "../schema/column.h"
#include "../literal.h"
#include "../table_reference.h"

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
    inline virtual_table_definition<dbstat_module_tag, Cs...> make_dbstat_definition(Cs... columns) {
        return {{std::make_tuple(std::move(columns)...)}};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  Data structure for the `dbstat` eponymous virtual table.
     */
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

        /** 
         *  Hidden columns of the `dbstat` virtual table, which can be referred to using a 'column pointer'.
         *  See `hidden_columns`.
         */
        struct hidden {
            std::string schema;
#if SQLITE_VERSION_NUMBER >= 3031000
            bool aggregate = false;
#endif

            using enclosing_type = dbstat;
        };
    };

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    inline constexpr orm_table_reference auto dbstat_table = c<dbstat>();
#else
    inline constexpr auto dbstat_table = c<dbstat>();
#endif

    /**
     *  Factory function for a DBSTAT virtual table definition.
     *  If no schema is specified then the main schema is used.
     *  
     *  The hidden DBSTAT columns 'schema' and 'aggregate' are mapped into each table definition.
     *  
     *  Though the DBSTAT virtual table is an eponymous table SQLite allows to create a virtual table instance with a different name.
     *  This is mostly useful with binding input arguments (so-called table values), e.g. a different schema than "main" or whether to query aggregated results.
     */
    template<class... Value>
    auto using_dbstat(Value... tableValues) {
        using namespace ::sqlite_orm::internal;
        using expected_hidden_types = std::tuple<member_field_type_t<decltype(&dbstat::hidden::schema)>
#if SQLITE_VERSION_NUMBER >= 3031000
                                                 ,
                                                 member_field_type_t<decltype(&dbstat::hidden::aggregate)>
#endif
                                                 >;
        static_assert(sizeof...(Value) <= std::tuple_size<expected_hidden_types>::value,
                      "You may only pass the schema name and the aggregation flag");
        using input_value_types = std::tuple<Value...>;
        // make a tuple of types from expected types limited to the number of passed in table values
        using final_expected_types =
            tuple_from_index_sequence_t<expected_hidden_types, std::index_sequence_for<Value...>>;
        static_assert(std::is_convertible<input_value_types, final_expected_types>::value,
                      "The schema name must be a string value, the aggregate flag a boolean value");

        return make_dbstat_definition(make_column("name", &dbstat::name),
                                      make_column("path", &dbstat::path),
                                      make_column("pageno", &dbstat::pageno),
                                      make_column("pagetype", &dbstat::pagetype),
                                      make_column("ncell", &dbstat::ncell),
                                      make_column("payload", &dbstat::payload),
                                      make_column("unused", &dbstat::unused),
                                      make_column("mx_payload", &dbstat::mx_payload),
                                      make_column("pgoffset", &dbstat::pgoffset),
                                      make_column("pgsize", &dbstat::pgsize),
                                      make_hidden_column("schema", &dbstat::hidden::schema),
#if SQLITE_VERSION_NUMBER >= 3031000
                                      make_hidden_column("aggregate", &dbstat::hidden::aggregate),
#endif
                                      literal_holder<Value>{std::move(tableValues)}...);
    }

    /**
     *  Factory function for the DBSTAT default eponymous virtual table.
     */
    inline auto make_dbstat_table() {
        return make_virtual_table<dbstat>(internal::dbstat_module_tag::name(), using_dbstat());
    }
}
#endif  //  SQLITE_ENABLE_DBSTAT_VTAB
