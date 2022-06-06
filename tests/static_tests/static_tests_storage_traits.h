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

            template<class S>
            struct storage_columns_count_impl
                : std::integral_constant<int, std::tuple_size<storage_elements_type_t<S>>::value> {};

            template<>
            struct storage_columns_count_impl<storage_impl<>> : std::integral_constant<int, 0> {};

            /**
             *  S - storage
             *  O - mapped or not mapped data type
             */
            template<class S, class O>
            struct storage_columns_count : storage_columns_count_impl<storage_find_impl_t<S, O>> {};

            /**
             *  Table A `table_t<>`
             *  O is object type which is the reference target (e.g. foreign_key(&Visit::userId).references(&User::id) has O = User)
             */
            template<class Table, class O>
            struct table_foreign_keys_count
                : count_filtered_tuple<elements_type_t<Table>,
                                       check_if_is_type<O>::template fn,
                                       filter_tuple_sequence_t<elements_type_t<Table>, is_foreign_key>,
                                       target_type_t> {};

            /**
             *  S - storage class
             *  O - type mapped to S
             */
            template<class S, class O>
            struct storage_foreign_keys_count_impl : std::integral_constant<int, 0> {};

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
            template<class... Ts, class O>
            struct storage_foreign_keys_count_impl<storage_impl<Ts...>, O> {
                static constexpr int value = (table_foreign_keys_count<Ts, O>::value + ...);
            };
#else
            template<class H, class... Ts, class O>
            struct storage_foreign_keys_count_impl<storage_impl<H, Ts...>, O> {
                static constexpr int value = table_foreign_keys_count<H, O>::value +
                                             storage_foreign_keys_count_impl<storage_impl<Ts...>, O>::value;
            };
#endif

            /**
             * S - storage class
             * O - type mapped to S
             * This class tells how many types mapped to S have foreign keys to O
             */
            template<class S, class O>
            struct storage_foreign_keys_count : storage_foreign_keys_count_impl<typename S::impl_type, O> {};

            template<class Table, class O>
            using table_foreign_keys_t =
                filter_tuple_t<elements_type_t<Table>,
                               check_if_is_type<O>::template fn,
                               target_type_t,
                               filter_tuple_sequence_t<elements_type_t<Table>, is_foreign_key>>;

            /*
             *  Implementation note: must be a struct instead of an template alias because the foreign keys tuple
             *  must be hoisted into a named alias, otherwise type replacement may fail for legacy compilers
             *  if an alias template has a dependent expression in it [SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION].
             */
            template<class Table, class O>
            struct table_fk_references {
                using foreign_keys = table_foreign_keys_t<Table, O>;

                using type = transform_tuple_t<foreign_keys, source_type_t>;
            };

            /**
             *  S - storage class
             *  O - type mapped to S
             */
            template<class S, class O>
            struct storage_fk_references_impl;

            template<class S, class O>
            struct storage_foreign_keys_impl;

            template<class... Ts, class O>
            struct storage_fk_references_impl<storage_impl<Ts...>, O>
                : conc_tuple<typename table_fk_references<Ts, O>::type...> {};

            template<class... Ts, class O>
            struct storage_foreign_keys_impl<storage_impl<Ts...>, O> : conc_tuple<table_foreign_keys_t<Ts, O>...> {};

            /**
             *  S - storage class
             *  O - type mapped to S
             *  type holds `std::tuple` with types that has references to O as  foreign keys
             */
            template<class S, class O>
            struct storage_fk_references : storage_fk_references_impl<typename S::impl_type, O> {};

            template<class S, class O>
            struct storage_foreign_keys : storage_foreign_keys_impl<typename S::impl_type, O> {};
        }
    }
}
