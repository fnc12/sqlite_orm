#pragma once

#include <type_traits>  //  std::is_same, std::enable_if, std::true_type, std::false_type, std::integral_constant
#include <tuple>  //  std::tuple

#include "type_traits.h"
#include "storage_lookup.h"
#include "tuple_helper/tuple_transformer.h"

namespace sqlite_orm {
    namespace internal {

        template<class O, bool WithoutRowId, class... Cs>
        struct table_t;

        template<class A, class B>
        struct foreign_key_t;

        namespace storage_traits {

            template<class S>
            struct type_is_mapped_impl : std::true_type {};

            template<>
            struct type_is_mapped_impl<storage_impl<>> : std::false_type {};

            /**
             *  S - storage
             *  O - mapped or not mapped data type
             */
            template<class S, class O>
            struct type_is_mapped : type_is_mapped_impl<storage_find_impl_t<S, O>> {};

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
             *  TransformOp - Unary metafunction that transforms a column into a type.
             *                (Note that multiple arguments are accepted to allow for
             *                 metafunctions that need SFINAE specialization)
             */
            template<class T, template<class...> class TransformOp = column_field_type>
            struct table_types;

            /**
             *  type is std::tuple of field types of mapped colums.
             */
            template<class O, bool WithoutRowId, class... Args, template<class...> class TransformOp>
            struct table_types<table_t<O, WithoutRowId, Args...>, TransformOp> {
                using args_tuple = std::tuple<Args...>;
                using columns_tuple = typename tuple_filter<args_tuple, is_column>::type;

                using type = typename tuple_transformer<columns_tuple, TransformOp>::type;
            };

            /**
             *  S - storage_impl type
             *  O - mapped or not mapped data type
             */
            template<class S>
            struct storage_mapped_columns_impl {
                using table_type = typename S::table_type;
                using type = typename table_types<table_type, column_field_type>::type;
            };

            template<>
            struct storage_mapped_columns_impl<storage_impl<>> {
                using type = std::tuple<>;
            };

            /**
             *  S - storage
             *  O - mapped or not mapped data type
             */
            template<class S, class O>
            struct storage_mapped_columns : storage_mapped_columns_impl<storage_find_impl_t<S, O>> {};

            /**
             *  S - storage_impl type for specific mapped type
             *  O - mapped data type
             */
            template<class S>
            struct storage_mapped_column_expressions_impl : table_types<table_type_t<S>, column_field_expression> {};

            template<>
            struct storage_mapped_column_expressions_impl<storage_impl<>> {
                using type = std::tuple<>;
            };

            /**
             *  S - storage
             *  O - mapped data type
             */
            template<class S, class O>
            struct storage_mapped_column_expressions
                : storage_mapped_column_expressions_impl<storage_find_impl_t<S, O>> {};

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

            template<class O, class... Cs, class P>
            struct table_foreign_keys_count<table_t<O, false, Cs...>, P> {
                using table_type = table_t<O, false, Cs...>;

                static constexpr int value = table_foreign_keys_count_impl<P, Cs...>::value;
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
                using type = typename conc_tuple<head_tuple, tail_tuple>::type;
            };

            template<class O, class H, class... Tail>
            struct table_foreign_keys_impl<O, H, Tail...> {
                using head_tuple = typename column_foreign_keys<H, O>::type;
                using tail_tuple = typename table_foreign_keys_impl<O, Tail...>::type;
                using type = typename conc_tuple<head_tuple, tail_tuple>::type;
            };

            /**
             *  T is table_t type
             *  O is object type which is the reference target (e.g. foreign_key(&Visit::userId).references(&User::id) has O = User)
             */
            template<class T, class O>
            struct table_fk_references;

            template<class T, class O>
            struct table_foreign_keys;

            template<class O, bool WithoutRowId, class... Cs, class P>
            struct table_fk_references<table_t<O, WithoutRowId, Cs...>, P> {
                using table_type = table_t<O, WithoutRowId, Cs...>;

                using type = typename table_fk_references_impl<P, Cs...>::type;
            };

            template<class O, bool WithoutRowId, class... Cs, class P>
            struct table_foreign_keys<table_t<O, WithoutRowId, Cs...>, P> {
                using table_type = table_t<O, WithoutRowId, Cs...>;

                using type = typename table_foreign_keys_impl<P, Cs...>::type;
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
                using type = typename conc_tuple<head_tuple, tail_tuple>::type;
            };

            template<class H, class... Ts, class O>
            struct storage_foreign_keys_impl<storage_impl<H, Ts...>, O> {
                using head_tuple = typename table_foreign_keys<H, O>::type;
                using tail_tuple = typename storage_foreign_keys_impl<storage_impl<Ts...>, O>::type;
                using type = typename conc_tuple<head_tuple, tail_tuple>::type;
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
