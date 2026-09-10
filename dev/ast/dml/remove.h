#pragma once

/** @file The DELETE statement, in both of the DSL spellings sqlite_orm offers for it - `remove`
 *        against the primary key of a mapped object, or `remove_all` raw against conditions.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::conjunction
#include <utility>  //  std::move, std::forward
#include <tuple>  //  std::tuple, std::tuple_size
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/type_traits.h"  //  value_unref_type_t
#include "../../tuple_helper/tuple_traits.h"
#include "../../alias_traits.h"
#include "../../vocabulary/node_traits.h"
#include "../../vocabulary/node_algorithms.h"  // clause predicates, is_bindable_v
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../../vocabulary/traits/semantic_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T, class... Ids>
    struct remove_t {
        using object_type = T;
        using ids_type = std::tuple<Ids...>;

        ids_type ids;
    };

    template<class T>
    constexpr bool is_remove_v = polyfill::is_specialization_of<T, remove_t>::value;

    template<class T>
    constexpr bool is_object_dml_expression_v<T, std::enable_if_t<is_remove_v<T>>> = true;

    template<class T, class... Args>
    struct remove_all_t {
        using object_type = T;
        using conditions_type = std::tuple<Args...>;

        conditions_type conditions;
    };

    template<class T>
    constexpr bool is_remove_all_v = polyfill::is_specialization_of<T, remove_all_t>::value;

    template<class T>
    constexpr bool is_raw_dml_expression_v<T, std::enable_if_t<is_remove_all_v<T>>> = true;

    template<class With>
    constexpr bool is_raw_dml_expression_v<
        With,
        std::enable_if_t<std::conjunction_v<is_with_clause<With>, is_remove_all<expression_type_t<With>>>>> = true;

    /**
     *  The delete statement counterpart of `validate_select_clauses()`; see there for the split of
     *  responsibilities between this and the clause factories.
     */
    template<class T>
    constexpr void validate_delete_clauses() {
        static_assert(count_tuple<T, is_where>::value <= 1, "a single statement cannot contain > 1 WHERE blocks");
        static_assert(count_tuple<T, is_any_order_by>::value <= 1,
                      "a single statement cannot contain > 1 ORDER BY blocks");
        static_assert(count_tuple<T, is_limit>::value <= 1, "a single statement cannot contain > 1 LIMIT blocks");
        static_assert(std::tuple_size<T>::value == count_tuple<T, is_delete_clause>::value,
                      "a DELETE argument must be a WHERE, ORDER BY or LIMIT clause");
        static_assert(check_delete_clause_order_v<T>,
                      "SQL clauses must be listed in the canonical order: WHERE, ORDER BY, LIMIT");
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Create a remove statement
     *  T is an object type mapped to a storage.
     *  Usage: remove<User>(5);
     */
    template<class T, class... Ids>
    internal::remove_t<T, Ids...> remove(Ids... ids) {
        static_assert((internal::is_bindable_v<internal::value_unref_type_t<Ids>> && ...),
                      "Only primary key values are accepted as Ids");
        return {{std::forward<Ids>(ids)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a remove statement
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: remove<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto remove(Ids... ids) {
        return remove<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

    /**
     *  Create a remove all statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.remove_all<User>(...);
     */
    template<class T, class... Args>
    internal::remove_all_t<T, Args...> remove_all(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_delete_clauses<args_tuple>();
        return {{std::forward<Args>(args)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a remove all statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: storage.remove_all<user_table>(...);
     */
    template<orm_table_reference auto table, class... Args>
    auto remove_all(Args... args) {
        return remove_all<internal::auto_decay_table_ref_t<table>>(std::forward<Args>(args)...);
    }
#endif
}
