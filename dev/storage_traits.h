#include <type_traits>  //  std::is_same, std::enable_if, std::true_type, std::false_type, std::integral_constant
#include <tuple>  //  std::tuple

#include "type_traits.h"
#include "storage_lookup.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"

namespace sqlite_orm {

    namespace internal {

        template<class... Ts>
        struct storage_impl;

        template<class T, bool WithoutRowId, class... Cs>
        struct table_t;

        template<class A, class B>
        struct foreign_key_t;

        namespace storage_traits {

            template<class S, class O, class SFINAE = void>
            SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v = false;
            template<class S, class O>
            SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v<S, O, polyfill::void_t<storage_pick_impl_t<S, O>>> = true;

            template<class S, class O>
            using is_mapped = polyfill::bool_constant<is_mapped_v<S, O>>;

            template<class S>
            struct storage_columns_count_impl : std::integral_constant<int, S::table_type::elements_count> {};

            template<>
            struct storage_columns_count_impl<storage_impl<>> : std::integral_constant<int, 0> {};

            /**
             *  S - storage
             *  O - mapped or not mapped data type
             */
            template<class S, class O>
            struct storage_columns_count : storage_columns_count_impl<storage_find_impl_t<S, O>> {};

            /**
             *  T - table type.
             */
            template<class T>
            struct table_types;

            /**
             *  type is std::tuple of field types of mapped colums.
             */
            template<class T, bool WithoutRowId, class... Args>
            struct table_types<table_t<T, WithoutRowId, Args...>> {
                using args_tuple = std::tuple<Args...>;
                using columns_tuple = typename tuple_filter<args_tuple, is_column>::type;

                using type = transform_tuple_t<columns_tuple, column_field_type>;
            };

            /**
             *  S - storage_impl type
             *  T - mapped or not mapped data type
             */
            template<class S, class T, class SFINAE = void>
            struct storage_mapped_columns_impl;

            /**
             *  S - storage
             *  T - mapped or not mapped data type
             */
            template<class S, class T>
            struct storage_mapped_columns : storage_mapped_columns_impl<typename S::impl_type, T> {};

            /**
             *  Final specialisation
             */
            template<class T>
            struct storage_mapped_columns_impl<storage_impl<>, T, void> {
                using type = std::tuple<>;
            };

            template<class S, class T>
            struct storage_mapped_columns_impl<
                S,
                T,
                typename std::enable_if<std::is_same<T, typename S::table_type::object_type>::value>::type> {
                using table_type = typename S::table_type;
                using type = typename table_types<table_type>::type;
            };

            template<class S, class T>
            struct storage_mapped_columns_impl<
                S,
                T,
                typename std::enable_if<!std::is_same<T, typename S::table_type::object_type>::value>::type>
                : storage_mapped_columns_impl<typename S::super, T> {};

            /**
             *  C is any column type: column_t or constraint type
             *  O - object type references in FOREIGN KEY
             */
            template<class C, class O>
            struct column_foreign_keys_count : std::integral_constant<int, 0> {};

            template<class A, class B, class O>
            struct column_foreign_keys_count<foreign_key_t<A, B>, O> {
                using target_type = typename foreign_key_t<A, B>::target_type;

                static constexpr int value = std::is_same<O, target_type>::value ? 1 : 0;
            };

            /**
             * O - object type references in FOREIGN KEY
             * Cs - column types which are stored in table_t::columns_type
             */
            template<class O, class... Cs>
            struct table_foreign_keys_count_impl;

            template<class O>
            struct table_foreign_keys_count_impl<O> {
                static constexpr int value = 0;
            };

            template<class O, class H, class... Tail>
            struct table_foreign_keys_count_impl<O, H, Tail...> {
                static constexpr int value =
                    column_foreign_keys_count<H, O>::value + table_foreign_keys_count_impl<O, Tail...>::value;
            };

            /**
             *  T is table_t type
             *  O is object type which is the reference target (e.g. foreign_key(&Visit::userId).references(&User::id) has O = User)
             */
            template<class T, class O>
            struct table_foreign_keys_count;

            template<class T, class... Cs, class O>
            struct table_foreign_keys_count<table_t<T, false, Cs...>, O> {
                using table_type = table_t<T, false, Cs...>;

                static constexpr int value = table_foreign_keys_count_impl<O, Cs...>::value;
            };

            /**
             *  S - storage class
             *  O - type mapped to S
             */
            template<class S, class O>
            struct storage_foreign_keys_count_impl;

            template<class O>
            struct storage_foreign_keys_count_impl<storage_impl<>, O> : std::integral_constant<int, 0> {};

            template<class H, class... Ts, class O>
            struct storage_foreign_keys_count_impl<storage_impl<H, Ts...>, O> {
                static constexpr int value = table_foreign_keys_count<H, O>::value +
                                             storage_foreign_keys_count_impl<storage_impl<Ts...>, O>::value;
            };

            /**
             * S - storage class
             * O - type mapped to S
             * This class tells how many types mapped to S have foreign keys to O
             */
            template<class S, class O>
            struct storage_foreign_keys_count {
                using impl_type = typename S::impl_type;

                static constexpr int value = storage_foreign_keys_count_impl<impl_type, O>::value;
            };

            /**
             * C is any table element type: column_t or constraint type
             * O - object type references in FOREIGN KEY
             */
            template<class C, class O, class SFINAE = void>
            struct column_fk_references {
                using type = std::tuple<>;
            };

            template<class C, class O, class SFINAE = void>
            struct column_foreign_keys {
                using type = std::tuple<>;
            };

            template<class A, class B, class O>
            struct column_foreign_keys<
                foreign_key_t<A, B>,
                O,
                typename std::enable_if<std::is_same<O, typename foreign_key_t<A, B>::target_type>::value>::type> {
                using type = std::tuple<foreign_key_t<A, B>>;
            };

            template<class A, class B, class O>
            struct column_foreign_keys<
                foreign_key_t<A, B>,
                O,
                typename std::enable_if<!std::is_same<O, typename foreign_key_t<A, B>::target_type>::value>::type> {
                using type = std::tuple<>;
            };

            template<class A, class B, class O>
            struct column_fk_references<
                foreign_key_t<A, B>,
                O,
                typename std::enable_if<std::is_same<O, typename foreign_key_t<A, B>::target_type>::value>::type> {
                using target_type = typename foreign_key_t<A, B>::source_type;

                using type = std::tuple<target_type>;
            };

            template<class A, class B, class O>
            struct column_fk_references<
                foreign_key_t<A, B>,
                O,
                typename std::enable_if<!std::is_same<O, typename foreign_key_t<A, B>::target_type>::value>::type> {
                using type = std::tuple<>;
            };

            /**
             * O - object type references in FOREIGN KEY
             * Cs - column types which are stored in table_t::columns_type
             */
            template<class O, class... Cs>
            struct table_fk_references_impl;

            template<class O, class... Cs>
            struct table_foreign_keys_impl;

            template<class O>
            struct table_fk_references_impl<O> {
                using type = std::tuple<>;
            };

            template<class O>
            struct table_foreign_keys_impl<O> {
                using type = std::tuple<>;
            };

            template<class O, class H, class... Tail>
            struct table_fk_references_impl<O, H, Tail...> {
                using head_tuple = typename column_fk_references<H, O>::type;
                using tail_tuple = typename table_fk_references_impl<O, Tail...>::type;
                using type = conc_tuple_t<head_tuple, tail_tuple>;
            };

            template<class O, class H, class... Tail>
            struct table_foreign_keys_impl<O, H, Tail...> {
                using head_tuple = typename column_foreign_keys<H, O>::type;
                using tail_tuple = typename table_foreign_keys_impl<O, Tail...>::type;
                using type = conc_tuple_t<head_tuple, tail_tuple>;
            };

            /**
             *  T is table_t type
             *  O is object type which is the reference target (e.g. foreign_key(&Visit::userId).references(&User::id) has O = User)
             */
            template<class T, class O>
            struct table_fk_references;

            template<class T, class O>
            struct table_foreign_keys;

            template<class T, bool WithoutRowId, class... Cs, class O>
            struct table_fk_references<table_t<T, WithoutRowId, Cs...>, O> {
                using table_type = table_t<T, WithoutRowId, Cs...>;

                using type = typename table_fk_references_impl<O, Cs...>::type;
            };

            template<class T, bool WithoutRowId, class... Cs, class O>
            struct table_foreign_keys<table_t<T, WithoutRowId, Cs...>, O> {
                using table_type = table_t<T, WithoutRowId, Cs...>;

                using type = typename table_foreign_keys_impl<O, Cs...>::type;
            };

            /**
             *  S - storage class
             *  O - type mapped to S
             */
            template<class S, class O>
            struct storage_fk_references_impl;

            template<class S, class O>
            struct storage_foreign_keys_impl;

            template<class O>
            struct storage_fk_references_impl<storage_impl<>, O> {
                using type = std::tuple<>;
            };

            template<class O>
            struct storage_foreign_keys_impl<storage_impl<>, O> {
                using type = std::tuple<>;
            };

            template<class H, class... Ts, class O>
            struct storage_fk_references_impl<storage_impl<H, Ts...>, O> {
                using head_tuple = typename table_fk_references<H, O>::type;
                using tail_tuple = typename storage_fk_references_impl<storage_impl<Ts...>, O>::type;
                using type = conc_tuple_t<head_tuple, tail_tuple>;
            };

            template<class H, class... Ts, class O>
            struct storage_foreign_keys_impl<storage_impl<H, Ts...>, O> {
                using head_tuple = typename table_foreign_keys<H, O>::type;
                using tail_tuple = typename storage_foreign_keys_impl<storage_impl<Ts...>, O>::type;
                using type = conc_tuple_t<head_tuple, tail_tuple>;
            };

            /**
             *  S - storage class
             *  O - type mapped to S
             *  type holds `std::tuple` with types that has references to O as  foreign keys
             */
            template<class S, class O>
            struct storage_fk_references {
                using impl_type = typename S::impl_type;

                using type = typename storage_fk_references_impl<impl_type, O>::type;
            };

            template<class S, class O>
            struct storage_foreign_keys {
                using impl_type = typename S::impl_type;

                using type = typename storage_foreign_keys_impl<impl_type, O>::type;
            };

        }
    }
}
