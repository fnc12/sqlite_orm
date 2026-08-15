#pragma once

/** @file Compile-time computation of the database object synchronization order.
 *
 *  `sync_schema()` must sync database objects in dependency order, not in declaration order:
 *  SQLite refuses to create an index or a trigger whose target table (or view) doesn't exist yet,
 *  and recreating a table (or a view) destroys its dependent indexes and triggers,
 *  which therefore must be synced afterwards in order to be recreated.
 *
 *  These are the only hard creation-time dependencies in SQLite -
 *  foreign keys, view selects, trigger bodies and fts5 external content are validated lazily
 *  at DML time and don't constrain the order.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <array>  //  std::array
#include <type_traits>  //  std::is_same, std::is_void
#include <utility>  //  std::index_sequence
#endif

#include "../../functional/cxx_type_traits_polyfill.h"  //  polyfill::detected_or_t
#include "../../vocabulary/node_traits.h"  //  object_type_t, table_mapped_type_t

namespace sqlite_orm::internal {
    /**
     *  Whether database object `D` must be synced after database object `S`:
     *  indexes and triggers project the mapped object type of their target table (or view)
     *  as `table_mapped_type`, which is matched against the object type provided by `S`.
     */
    template<class D, class S>
    constexpr bool sync_depends_on_v =
        !std::is_void<polyfill::detected_or_t<void, table_mapped_type_t, D>>::value &&
        std::is_same<polyfill::detected_or_t<void, table_mapped_type_t, D>, object_type_t<S>>::value;

    template<class D, class... DBO>
    constexpr std::array<bool, sizeof...(DBO)> make_sync_dependency_row() {
        return {{sync_depends_on_v<D, DBO>...}};
    }

    /**
     *  Computes the order in which the database objects must be synced as an array of tuple indices:
     *  a stable topological sort which keeps the declaration order and only defers indexes and triggers
     *  until after their target table or view.
     *  The dependency graph is trivially acyclic since indexes and triggers depend on tables and views only,
     *  which in turn depend on nothing.
     */
    template<class... DBO>
    constexpr std::array<size_t, sizeof...(DBO)> compute_sync_order() {
        constexpr size_t nDBOs = sizeof...(DBO);
        const std::array<std::array<bool, nDBOs>, nDBOs> dependencies{{make_sync_dependency_row<DBO, DBO...>()...}};

        std::array<size_t, nDBOs> order{};
        std::array<bool, nDBOs> placed{};
        size_t position = 0;
        while (position < nDBOs) {
            bool madeProgress = false;
            for (size_t candidate = 0; candidate < nDBOs; ++candidate) {
                if (placed[candidate]) {
                    continue;
                }
                bool isReady = true;
                for (size_t dependency = 0; dependency < nDBOs; ++dependency) {
                    if (dependencies[candidate][dependency] && !placed[dependency]) {
                        isReady = false;
                        break;
                    }
                }
                if (isReady) {
                    order[position] = candidate;
                    ++position;
                    placed[candidate] = true;
                    madeProgress = true;
                }
            }
            //  impossible with the current dependency kinds, but guards against an endless loop
            //  in case an index or a trigger targets a type which is not mapped in the schema:
            //  place the remaining objects in declaration order
            if (!madeProgress) {
                for (size_t candidate = 0; candidate < nDBOs; ++candidate) {
                    if (!placed[candidate]) {
                        order[position] = candidate;
                        ++position;
                        placed[candidate] = true;
                    }
                }
            }
        }
        return order;
    }

    template<class Tpl>
    struct sync_order_of;

    template<template<class...> class Tpl, class... DBO>
    struct sync_order_of<Tpl<DBO...>> {
        static constexpr std::array<size_t, sizeof...(DBO)> value = compute_sync_order<DBO...>();

        template<size_t... Idx>
        static std::index_sequence<value[Idx]...> make_sequence(std::index_sequence<Idx...>);

        using type = decltype(make_sequence(std::index_sequence_for<DBO...>{}));
    };

    /**
     *  The index sequence of the database objects tuple in synchronization order.
     */
    template<class Tpl>
    using sync_order_sequence_t = typename sync_order_of<Tpl>::type;
}
