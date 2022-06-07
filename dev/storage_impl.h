#pragma once

#include <utility>  //  std::forward, std::move

namespace sqlite_orm {

    namespace internal {

        /**
         *  A chain of schema objects
         */
        template<class... Ts>
        struct storage_impl {};

        template<class H, class... Ts>
        struct storage_impl<H, Ts...> : storage_impl<Ts...> {
            using super = storage_impl<Ts...>;
            using table_type = H;

            storage_impl(H h, Ts... ts) : super{std::forward<Ts>(ts)...}, table{std::move(h)} {}

            table_type table;
        };
    }
}

#include <typeindex>  //  std::type_index
#include <string>  //  std::string
#include <type_traits>  //  std::is_same, std::decay, std::make_index_sequence, std::index_sequence

#include "functional/cxx_universal.h"
#include "functional/static_magic.h"
#include "type_traits.h"
#include "select_constraints.h"
#include "storage_lookup.h"

// interface functions
namespace sqlite_orm {
    namespace internal {

#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
        template<class S, size_t... Idx, class L, satisfies<is_storage_impl, S> = true>
        void for_each([[maybe_unused]] const S& impl,
                      [[maybe_unused]] std::index_sequence<Idx...> seq,
                      [[maybe_unused]] L& lambda) {
            if constexpr(sizeof...(Idx) > 0) {
                using types = schema_objects_tuple_t<S>;
                // reversed iteration (using a properly reversed variadic index sequence)
                constexpr size_t nTypes = std::tuple_size<types>::value;
                constexpr size_t baseIndex = first_index_sequence_value(seq);
                (lambda(pick_table<std::tuple_element_t<nTypes - 1u - Idx + baseIndex, types>>(impl)), ...);
            }
        }
#else
        template<class S, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S&, std::index_sequence<>, L& /*lambda*/) {}

        template<class S, size_t I, size_t... Idx, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S& impl, std::index_sequence<I, Idx...>, L& lambda) {
            using types = schema_objects_tuple_t<S>;
            // reversed iteration
            for_each(impl, std::index_sequence<Idx...>{}, lambda);
            lambda(pick_table<std::tuple_element_t<I, types>>(impl));
        }
#endif

        template<class S, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S& impl, L lambda) {
            using all_index_sequence = std::make_index_sequence<std::tuple_size<schema_objects_tuple_t<S>>::value>;
            for_each(impl, all_index_sequence{}, lambda);
        }

        template<template<class...> class is_type, class S, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S& impl, L lambda) {
            using filtered_index_sequence =
                filter_tuple_sequence_t<schema_objects_tuple_t<S>, check_if<is_type>::template fn>;
            for_each(impl, filtered_index_sequence{}, lambda);
        }

        template<class S, satisfies<is_storage_impl, S> = true>
        int foreign_keys_count(const S& impl) {
            int res = 0;
            for_each<is_table>(impl, [&res](const auto& table) {
                res += table.foreign_keys_count();
            });
            return res;
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        auto lookup_table(const S& impl) {
            return static_if<is_mapped_v<S, Lookup>>(
                [](const auto& impl) {
                    return &pick_table<Lookup>(impl);
                },
                empty_callable<nullptr_t>())(impl);
        }

        template<class S, satisfies<is_storage_impl, S> = true>
        std::string find_table_name(const S& impl, const std::type_index& ti) {
            std::string res;
            for_each<is_table>(impl, [&ti, &res](const auto& table) {
                using table_type = std::decay_t<decltype(table)>;
                if(ti == typeid(object_type_t<table_type>)) {
                    res = table.name;
                }
            });
            return res;
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        std::string lookup_table_name(const S& impl) {
            return static_if<is_mapped_v<S, Lookup>>(
                [](const auto& impl) {
                    return pick_table<Lookup>(impl).name;
                },
                empty_callable<std::string>())(impl);
        }

        /**
         *  Find column name by its type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& impl, F O::*field) {
            return pick_table<O>(impl).find_column_name(field);
        }

        /**
         *  Materialize column pointer:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) materialize_column_pointer(const S&, const column_pointer<O, F>& cp) {
            return cp.field;
        }

        /**
         *  Find column name by:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& impl, const column_pointer<O, F>& cp) {
            auto field = materialize_column_pointer(impl, cp);
            return pick_table<O>(impl).find_column_name(field);
        }
    }
}
