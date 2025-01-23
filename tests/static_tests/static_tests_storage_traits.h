#pragma once
/*
 *  All symbols used to be in dev/storage_traits.h up to sqlite_orm 1.7.
 *  Because they were not used, they were moved to unit tests,
 *  and for simplicitly the namespace sqlite_orm::internal::storage_traits was kept.
 */

#include <type_traits>  //  std::integral_constant

#include <sqlite_orm/sqlite_orm.h>

namespace sqlite_orm {

    namespace internal {

        template<typename T>
        using source_type_t = typename T::source_type;

        namespace storage_traits {

            template<class DBOs>
            struct storage_columns_count_impl
                : std::integral_constant<int, std::tuple_size<elements_type_t<DBOs>>::value> {};

            template<>
            struct storage_columns_count_impl<polyfill::nonesuch> : std::integral_constant<int, 0> {};

            /**
             *  S - storage_t
             *  Lookup - mapped or not mapped data type
             */
            template<class S, class Lookup>
            struct storage_columns_count
                : storage_columns_count_impl<storage_find_table_t<Lookup, typename S::db_objects_type>> {};

            /**
             *  Table A `table_t<>`
             *  Lookup is object type which is the reference target (e.g. foreign_key(&Visit::userId).references(&User::id) has Lookup = User)
             */
            template<class Table, class Lookup>
            struct table_foreign_keys_count
                : count_filtered_tuple<elements_type_t<Table>,
                                       check_if_is_type<Lookup>::template fn,
                                       filter_tuple_sequence_t<elements_type_t<Table>, is_foreign_key>,
                                       target_type_t> {};

            /**
             *  DBOs - db_objects_tuple
             *  Lookup - type mapped to storage
             */
            template<class DBOs, class Lookup>
            struct storage_foreign_keys_count_impl : std::integral_constant<int, 0> {};

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
            template<class... DBO, class Lookup>
            struct storage_foreign_keys_count_impl<db_objects_tuple<DBO...>, Lookup> {
                static constexpr int value = (table_foreign_keys_count<DBO, Lookup>::value + ...);
            };
#else
            template<class H, class... DBO, class Lookup>
            struct storage_foreign_keys_count_impl<db_objects_tuple<H, DBO...>, Lookup> {
                static constexpr int value = table_foreign_keys_count<H, Lookup>::value +
                                             storage_foreign_keys_count_impl<db_objects_tuple<DBO...>, Lookup>::value;
            };
#endif

            /**
             * S - storage class
             * Lookup - type mapped to storage
             * This class tells how many types mapped to DBOs have foreign keys to Lookup
             */
            template<class S, class Lookup>
            struct storage_foreign_keys_count : storage_foreign_keys_count_impl<typename S::db_objects_type, Lookup> {};

            template<class Table, class Lookup>
            using table_foreign_keys_t =
                filter_tuple_t<elements_type_t<Table>,
                               check_if_is_type<Lookup>::template fn,
                               target_type_t,
                               filter_tuple_sequence_t<elements_type_t<Table>, is_foreign_key>>;

            /*
             *  Implementation note: must be a struct instead of an alias template because the foreign keys tuple
             *  must be hoisted into a named alias, otherwise type replacement may fail for legacy compilers
             *  if an alias template has a dependent expression in it [SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION].
             */
            template<class Table, class Lookup>
            struct table_fk_references {
                using foreign_keys = table_foreign_keys_t<Table, Lookup>;

                using type = transform_tuple_t<foreign_keys, source_type_t>;
            };

            /**
             *  DBOs - db_objects_tuple
             *  Lookup - type mapped to storage
             */
            template<class DBOs, class Lookup>
            struct storage_fk_references_impl;

            template<class DBOs, class Lookup>
            struct storage_foreign_keys_impl;

            template<class... DBO, class Lookup>
            struct storage_fk_references_impl<db_objects_tuple<DBO...>, Lookup>
                : conc_tuple<typename table_fk_references<DBO, Lookup>::type...> {};

            template<class... DBO, class Lookup>
            struct storage_foreign_keys_impl<db_objects_tuple<DBO...>, Lookup>
                : conc_tuple<table_foreign_keys_t<DBO, Lookup>...> {};

            /**
             *  S - storage class
             *  Lookup - type mapped to storage
             *  type holds `std::tuple` with types that has references to Lookup as foreign keys
             */
            template<class S, class Lookup>
            struct storage_fk_references : storage_fk_references_impl<typename S::db_objects_type, Lookup> {};

            template<class S, class Lookup>
            struct storage_foreign_keys : storage_foreign_keys_impl<typename S::db_objects_type, Lookup> {};
        }
    }
}
