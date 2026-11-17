#pragma once

/** @file
 *  Purpose: find the table that maps a type among the database objects of a schema.
 *
 *  Intention: a schema-level algorithm, placed by `docs/internals/vocabulary-layer.md` "Schema-level algorithms";
 *  consumes the vocabulary layer.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  // std::tuple_size, std::get
#include <type_traits>  //  std::true_type, std::false_type, std::remove_const, std::enable_if, std::is_same, std::is_void
#include <utility>  //  std::index_sequence, std::make_index_sequence
#endif

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../../functional/type_traits.h"
#include "../../vocabulary/node_algorithms.h"  //  column_field_types_t, column_field_expressions_t
#include "../../vocabulary/node_traits.h"
#include "../db_objects.h"

namespace sqlite_orm::internal {
    /**
     *  `std::true_type` if given object is mapped, `std::false_type` otherwise.
     *
     *  Note: unlike ordinary_table<>, index_t<>::object_type and trigger_t<>::object_type is always void.
     *  A lookup thus finds a type mapped by a table in the wide sense: an ordinary table, a view or a virtual table.
     */
    template<typename DBO, typename Lookup>
    struct object_type_matches
        : std::conjunction<std::negation<std::is_void<object_type_t<DBO>>>, std::is_same<Lookup, object_type_t<DBO>>> {
    };

    /**
     *  `std::true_type` if given lookup type (object or moniker) is mapped, `std::false_type` otherwise.
     */
    template<typename DBO, typename Lookup>
    using lookup_type_matches = object_type_matches<DBO, Lookup>;
}

// pick/lookup metafunctions
namespace sqlite_orm::internal {

    /**
     *  Indirect enabler for DBO, accepting an index to disambiguate non-unique DBOs
     */
    template<class Lookup, size_t Ix, class DBO>
    struct enable_found_table : std::enable_if<lookup_type_matches<DBO, Lookup>::value, DBO> {};

    /**
     *  SFINAE friendly facility to pick a table definition (`ordinary_table`) from a tuple of database objects.
     *
     *  Lookup - mapped data type
     *  Seq - index sequence matching the number of DBOs
     *  DBOs - db_objects_tuple type
     */
    template<class Lookup, class Seq, class DBOs>
    struct schema_pick_table;

    template<class Lookup, size_t... Ix, class... DBO>
    struct schema_pick_table<Lookup, std::index_sequence<Ix...>, db_objects_tuple<DBO...>>
        : enable_found_table<Lookup, Ix, DBO>... {};

    /**
     *  SFINAE friendly facility to pick a table definition (`ordinary_table`) from a tuple of database objects.
     *
     *  Lookup - 'table' type, mapped data type
     *  DBOs - db_objects_tuple type, possibly const-qualified
     */
    template<class Lookup, class DBOs>
    using schema_pick_table_t = typename schema_pick_table<Lookup,
                                                           std::make_index_sequence<std::tuple_size<DBOs>::value>,
                                                           std::remove_const_t<DBOs>>::type;

    /**
     *  Find a table definition (`ordinary_table`) from a tuple of database objects;
     *  `std::nonesuch` if not found.
     *
     *  DBOs - db_objects_tuple type
     *  Lookup - mapped data type
     */
    template<class Lookup, class DBOs>
    struct schema_find_table : polyfill::detected<schema_pick_table_t, Lookup, DBOs> {};

    /**
     *  Find a table definition (`ordinary_table`) from a tuple of database objects;
     *  `std::nonesuch` if not found.
     *
     *  DBOs - db_objects_tuple type, possibly const-qualified
     *  Lookup - mapped data type
     */
    template<class Lookup, class DBOs>
    using schema_find_table_t = typename schema_find_table<Lookup, std::remove_const_t<DBOs>>::type;

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
    template<class DBOs, class Lookup, class SFINAE = void>
    struct is_mapped : std::false_type {};
    template<class DBOs, class Lookup>
    struct is_mapped<DBOs, Lookup, std::void_t<schema_pick_table_t<Lookup, DBOs>>> : std::true_type {};
#else
    template<class DBOs, class Lookup, class SFINAE = schema_find_table_t<Lookup, DBOs>>
    struct is_mapped : std::true_type {};
    template<class DBOs, class Lookup>
    struct is_mapped<DBOs, Lookup, polyfill::nonesuch> : std::false_type {};
#endif

    template<class DBOs, class Lookup>
    constexpr bool is_mapped_v = is_mapped<DBOs, Lookup>::value;
}

// the columns of a looked-up table
namespace sqlite_orm::internal {
    template<class Table>
    struct schema_mapped_column_field_types_impl {
        using type = column_field_types_t<Table>;
    };

    template<>
    struct schema_mapped_column_field_types_impl<polyfill::nonesuch> {
        using type = std::tuple<>;
    };

    /**
     *  The field types of the columns of the table mapped for the given lookup type, in column order;
     *  an empty tuple if the lookup type is not mapped.
     *
     *  DBOs - db_objects_tuple type
     *  Lookup - mapped or unmapped data type
     */
    template<class DBOs, class Lookup>
    struct schema_mapped_column_field_types : schema_mapped_column_field_types_impl<schema_find_table_t<Lookup, DBOs>> {
    };

    template<class Table>
    struct schema_mapped_column_field_expressions_impl {
        using type = column_field_expressions_t<Table>;
    };

    template<>
    struct schema_mapped_column_field_expressions_impl<polyfill::nonesuch> {
        using type = std::tuple<>;
    };

    /**
     *  The member pointers the columns of the table mapped for the given lookup type are mapped by,
     *  in column order; an empty tuple if the lookup type is not mapped.
     *
     *  DBOs - db_objects_tuple type
     *  Lookup - mapped or unmapped data type
     */
    template<class DBOs, class Lookup>
    struct schema_mapped_column_field_expressions
        : schema_mapped_column_field_expressions_impl<schema_find_table_t<Lookup, DBOs>> {};
}

// runtime lookup functions
namespace sqlite_orm::internal {
    /**
     *  Pick the table definition for the specified lookup type from the given tuple of schema objects.
     *
     *  Note: This function requires Lookup to be mapped, otherwise it is removed from the overload resolution set.
     */
    template<class Lookup, class DBOs, satisfies<is_mapped, DBOs, Lookup> = true>
    auto& pick_table(DBOs& dbObjects) {
        using table_type = schema_pick_table_t<Lookup, DBOs>;
        return std::get<table_type>(dbObjects);
    }

    /**
     *  The name of the table mapped for the specified lookup type; an empty string if it is not mapped.
     */
    template<class Lookup, class DBOs, satisfies<is_db_objects, DBOs> = true>
    decltype(auto) lookup_table_name(const DBOs& dbObjects) {
        if constexpr (is_mapped_v<DBOs, Lookup>) {
            return (pick_table<Lookup>(dbObjects).name);
        } else {
            return std::string{};
        }
    }
}
