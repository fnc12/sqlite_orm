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
        template<class DBOs, size_t... Idx, class L, satisfies<is_db_objects, DBOs> = true>
        void for_each([[maybe_unused]] const DBOs& dbObjects, std::index_sequence<Idx...>, [[maybe_unused]] L& lambda) {
            if constexpr(sizeof...(Idx) > 0) {
                constexpr size_t lowerIndex = std::min(std::initializer_list<size_t>{Idx...});
                constexpr size_t upperIndex = std::max(std::initializer_list<size_t>{Idx...});
                constexpr float pivot = sizeof...(Idx) > 1 ? (upperIndex + lowerIndex) / 2.f : lowerIndex;
                (lambda(std::get<size_t(pivot + (pivot - Idx))>(dbObjects)), ...);
            }
        }
#else
        template<class DBOs, class L, satisfies<is_db_objects, DBOs> = true>
        void for_each(const DBOs&, std::index_sequence<>, L& /*lambda*/) {}

        template<class DBOs, size_t I, size_t... Idx, class L, satisfies<is_db_objects, DBOs> = true>
        void for_each(const DBOs& dbObjects, std::index_sequence<I, Idx...>, L& lambda) {
            // reversed iteration
            for_each(dbObjects, std::index_sequence<Idx...>{}, lambda);
            lambda(std::get<I>(dbObjects));
        }
#endif

        template<class DBOs, class L, satisfies<is_db_objects, DBOs> = true>
        void for_each(const DBOs& dbObjects, L lambda) {
            using all_index_sequence = std::make_index_sequence<std::tuple_size<DBOs>::value>;
            for_each(dbObjects, all_index_sequence{}, lambda);
        }

        template<template<class...> class SeqOp, class DBOs, class L, satisfies<is_db_objects, DBOs> = true>
        void for_each(const DBOs& dbObjects, L lambda) {
            for_each(dbObjects, SeqOp<DBOs>{}, lambda);
        }

        template<class DBOs>
        using tables_index_sequence = filter_tuple_sequence_t<DBOs, is_table>;

        template<class DBOs, satisfies<is_db_objects, DBOs> = true>
        int foreign_keys_count(const DBOs& dbObjects) {
            int res = 0;
            for_each<tables_index_sequence>(dbObjects, [&res](const auto& table) {
                res += table.foreign_keys_count();
            });
            return res;
        }

        template<class Lookup, class DBOs, satisfies<is_db_objects, DBOs> = true>
        auto lookup_table(const DBOs& dbObjects) {
            return static_if<is_mapped_v<DBOs, Lookup>>(
                [](const auto& dbObjects) {
                    return &pick_table<Lookup>(dbObjects);
                },
                empty_callable<nullptr_t>())(dbObjects);
        }

        template<class DBOs, satisfies<is_db_objects, DBOs> = true>
        std::string find_table_name(const DBOs& dbObjects, const std::type_index& ti) {
            std::string res;
            for_each<tables_index_sequence>(dbObjects, [&ti, &res](const auto& table) {
                using table_type = std::decay_t<decltype(table)>;
                if(ti == typeid(object_type_t<table_type>)) {
                    res = table.name;
                }
            });
            return res;
        }

        template<class Lookup, class DBOs, satisfies<is_db_objects, DBOs> = true>
        std::string lookup_table_name(const DBOs& dbObjects) {
            return static_if<is_mapped_v<DBOs, Lookup>>(
                [](const auto& dbObjects) {
                    return pick_table<Lookup>(dbObjects).name;
                },
                empty_callable<std::string>())(dbObjects);
        }

        /**
         *  Find column name by its type and member pointer.
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        const std::string* find_column_name(const DBOs& dbObjects, F O::*field) {
            return pick_table<O>(dbObjects).find_column_name(field);
        }

        /**
         *  Materialize column pointer:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        constexpr decltype(auto) materialize_column_pointer(const DBOs&, const column_pointer<O, F>& cp) {
            return cp.field;
        }

        /**
         *  Find column name by:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        const std::string* find_column_name(const DBOs& dbObjects, const column_pointer<O, F>& cp) {
            auto field = materialize_column_pointer(dbObjects, cp);
            return pick_table<O>(dbObjects).find_column_name(field);
        }
    }
}
