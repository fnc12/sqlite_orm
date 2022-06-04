#pragma once

#include <tuple>  //  std::tuple_size
#include <typeindex>  //  std::type_index
#include <string>  //  std::string
#include <type_traits>  //  std::is_same, std::decay, std::make_index_sequence, std::index_sequence
#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
#include <algorithm>  //  std::min, std::max
#endif

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
        void for_each([[maybe_unused]] const S& impl, std::index_sequence<Idx...>, [[maybe_unused]] L& lambda) {
            if constexpr(sizeof...(Idx) > 0) {
                constexpr size_t lowerIndex = std::min(std::initializer_list<size_t>{Idx...});
                constexpr size_t upperIndex = std::max(std::initializer_list<size_t>{Idx...});
                constexpr float pivot = sizeof...(Idx) > 1 ? (upperIndex + lowerIndex) / 2.f : lowerIndex;
                (lambda(std::get<size_t(pivot + (pivot - Idx))>(impl)), ...);
            }
        }
#else
        template<class S, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S&, std::index_sequence<>, L& /*lambda*/) {}

        template<class S, size_t I, size_t... Idx, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S& impl, std::index_sequence<I, Idx...>, L& lambda) {
            // reversed iteration
            for_each(impl, std::index_sequence<Idx...>{}, lambda);
            lambda(std::get<I>(impl));
        }
#endif

        template<class S, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S& impl, L lambda) {
            using all_index_sequence = std::make_index_sequence<std::tuple_size<S>::value>;
            for_each(impl, all_index_sequence{}, lambda);
        }

        template<template<class...> class SeqOp, class S, class L, satisfies<is_storage_impl, S> = true>
        void for_each(const S& impl, L lambda) {
            for_each(impl, SeqOp<S>{}, lambda);
        }

        template<class SOs>
        using tables_index_sequence = filter_tuple_sequence_t<SOs, is_table>;

        template<class S, satisfies<is_storage_impl, S> = true>
        int foreign_keys_count(const S& impl) {
            int res = 0;
            for_each<tables_index_sequence>(impl, [&res](const auto& table) {
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
            for_each<tables_index_sequence>(impl, [&ti, &res](const auto& table) {
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
