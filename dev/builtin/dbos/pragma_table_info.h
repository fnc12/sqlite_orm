#pragma once

/** @file The rows of `PRAGMA table_info` and `PRAGMA table_xinfo`, and the eponymous virtual tables
 *        `pragma_table_info` and `pragma_table_xinfo` of their PRAGMA functions.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED)
#include <type_traits>  //  std::true_type, std::false_type
#include <tuple>  //  std::make_tuple
#endif
#endif

#include "../../functional/gsl.h"
#include "../../schema/column.h"  //  make_column, make_hidden_column
#include "../../schema/virtual_table.h"
#include "../../ast/table_reference.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  A row of `PRAGMA table_info`, and data structure for the `pragma_table_info` eponymous virtual table.
     */
    struct table_info {
        int cid = 0;
        std::string name;
        std::string type;
        bool notnull = false;
        std::string dflt_value;
        int pk = 0;

#ifndef SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
        table_info(decltype(cid) cid_,
                   decltype(name) name_,
                   decltype(type) type_,
                   decltype(notnull) notnull_,
                   decltype(dflt_value) dflt_value_,
                   decltype(pk) pk_) :
            cid(cid_), name(std::move(name_)), type(std::move(type_)), notnull(notnull_),
            dflt_value(std::move(dflt_value_)), pk(pk_) {}
#endif

        /**
         *  Hidden columns of the `pragma_table_info` virtual table: the PRAGMA argument (the table name) and the schema.
         */
        struct hidden {
            std::string arg;
            std::string schema;

            using enclosing_type = table_info;
        };
    };

    /**
     *  A row of `PRAGMA table_xinfo`, and data structure for the `pragma_table_xinfo` eponymous virtual table.
     */
    struct table_xinfo {
        int cid = 0;
        std::string name;
        std::string type;
        bool notnull = false;
        std::string dflt_value;
        int pk = 0;
        //  as reported by `PRAGMA table_xinfo`: 0 = normal column, 1 = hidden column in a virtual table,
        //  2 = VIRTUAL generated column, 3 = STORED generated column;
        //  `get_table_info()` stores 1 for a generated column, so only zero vs non-zero may be compared
        int hidden = 0;

#ifndef SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
        table_xinfo(decltype(cid) cid_,
                    decltype(name) name_,
                    decltype(type) type_,
                    decltype(notnull) notnull_,
                    decltype(dflt_value) dflt_value_,
                    decltype(pk) pk_,
                    decltype(hidden) hidden_) :
            cid(cid_), name(std::move(name_)), type(std::move(type_)), notnull(notnull_),
            dflt_value(std::move(dflt_value_)), pk(pk_), hidden{hidden_} {}
#endif

        /**
         *  Hidden columns of the `pragma_table_xinfo` virtual table: the PRAGMA argument (the table name) and the schema.
         *
         *  The data member `hidden` hides this struct's name: `table_xinfo::hidden::arg` names a hidden column,
         *  `struct table_xinfo::hidden` names the struct.
         */
        struct hidden {
            std::string arg;
            std::string schema;

            using enclosing_type = table_xinfo;
        };
    };
}

#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED)
namespace sqlite_orm::internal {
    struct pragma_table_info_module_tag {
        // simplify conceptual/meta programming
        using module_type = pragma_table_info_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "pragma_table_info";
        }
    };

    template<>
    struct virtual_table_module_traits<pragma_table_info_module_tag> {
        using module_type = pragma_table_info_module_tag;
        using is_eponymous = std::true_type;
        using is_without_rowid = std::false_type;
        using omit_column_type = std::true_type;
    };

    template<class... Cs>
    struct virtual_table_traits<pragma_table_info_module_tag, Cs...>
        : virtual_table_module_traits<pragma_table_info_module_tag> {
        using definition_type = table_definition<Cs...>;
        using elements_type = typename definition_type::elements_type;
    };

    template<class... Cs>
    inline virtual_table_definition<pragma_table_info_module_tag, Cs...>
    make_pragma_table_info_definition(Cs... columns) {
        return {{std::make_tuple(std::move(columns)...)}};
    }

    /**
     *  Factory function for a `pragma_table_info` virtual table definition.
     *
     *  The hidden columns 'arg' and 'schema' are mapped into each table definition.
     */
    inline auto using_pragma_table_info() {
        return make_pragma_table_info_definition(make_column("cid", &table_info::cid),
                                                 make_column("name", &table_info::name),
                                                 make_column("type", &table_info::type),
                                                 make_column("notnull", &table_info::notnull),
                                                 make_column("dflt_value", &table_info::dflt_value),
                                                 make_column("pk", &table_info::pk),
                                                 make_hidden_column("arg", &table_info::hidden::arg),
                                                 make_hidden_column("schema", &table_info::hidden::schema));
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    inline constexpr orm_table_reference auto pragma_table_info_table = c<table_info>();
#else
    inline constexpr auto pragma_table_info_table = c<table_info>();
#endif

    /**
     *  Factory function for the `pragma_table_info` eponymous virtual table.
     */
    inline auto make_pragma_table_info_table() {
        return make_virtual_table<table_info>(internal::pragma_table_info_module_tag::name(),
                                              internal::using_pragma_table_info());
    }
}

#if SQLITE_VERSION_NUMBER >= 3026000
namespace sqlite_orm::internal {
    struct pragma_table_xinfo_module_tag {
        // simplify conceptual/meta programming
        using module_type = pragma_table_xinfo_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "pragma_table_xinfo";
        }
    };

    template<>
    struct virtual_table_module_traits<pragma_table_xinfo_module_tag> {
        using module_type = pragma_table_xinfo_module_tag;
        using is_eponymous = std::true_type;
        using is_without_rowid = std::false_type;
        using omit_column_type = std::true_type;
    };

    template<class... Cs>
    struct virtual_table_traits<pragma_table_xinfo_module_tag, Cs...>
        : virtual_table_module_traits<pragma_table_xinfo_module_tag> {
        using definition_type = table_definition<Cs...>;
        using elements_type = typename definition_type::elements_type;
    };

    template<class... Cs>
    inline virtual_table_definition<pragma_table_xinfo_module_tag, Cs...>
    make_pragma_table_xinfo_definition(Cs... columns) {
        return {{std::make_tuple(std::move(columns)...)}};
    }

    /**
     *  Factory function for a `pragma_table_xinfo` virtual table definition.
     *
     *  The hidden columns 'arg' and 'schema' are mapped into each table definition.
     */
    inline auto using_pragma_table_xinfo() {
        return make_pragma_table_xinfo_definition(make_column("cid", &table_xinfo::cid),
                                                  make_column("name", &table_xinfo::name),
                                                  make_column("type", &table_xinfo::type),
                                                  make_column("notnull", &table_xinfo::notnull),
                                                  make_column("dflt_value", &table_xinfo::dflt_value),
                                                  make_column("pk", &table_xinfo::pk),
                                                  make_column("hidden", &table_xinfo::hidden),
                                                  make_hidden_column("arg", &table_xinfo::hidden::arg),
                                                  make_hidden_column("schema", &table_xinfo::hidden::schema));
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    inline constexpr orm_table_reference auto pragma_table_xinfo_table = c<table_xinfo>();
#else
    inline constexpr auto pragma_table_xinfo_table = c<table_xinfo>();
#endif

    /**
     *  Factory function for the `pragma_table_xinfo` eponymous virtual table.
     */
    inline auto make_pragma_table_xinfo_table() {
        return make_virtual_table<table_xinfo>(internal::pragma_table_xinfo_module_tag::name(),
                                               internal::using_pragma_table_xinfo());
    }
}
#endif
#endif
