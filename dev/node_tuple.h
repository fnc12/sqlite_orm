#pragma once

#include <type_traits>  //  std::enable_if
#include <tuple>  //  std::tuple
#include <utility>  //  std::pair
#include <functional>  //  std::reference_wrapper
#include "functional/cxx_optional.h"

#include "functional/cxx_type_traits_polyfill.h"
#include "tuple_helper/tuple_filter.h"
#include "type_traits.h"
#include "conditions.h"
#include "operators.h"
#include "select_constraints.h"
#include "prepared_statement.h"
#include "optional_container.h"
#include "core_functions.h"
#include "function.h"
#include "ast/excluded.h"
#include "ast/upsert_clause.h"
#include "ast/where.h"
#include "ast/into.h"
#include "ast/group_by.h"
#include "ast/match.h"

namespace sqlite_orm {
    namespace internal {

        template<class T, class SFINAE = void>
        struct node_tuple {
            using type = std::tuple<T>;
        };

        template<class T>
        using node_tuple_t = typename node_tuple<T>::type;

        /*
         *   Node tuple for several types.
         */
        template<class... T>
        using node_tuple_for = conc_tuple<typename node_tuple<T>::type...>;

        template<>
        struct node_tuple<void, void> {
            using type = std::tuple<>;
        };

        template<class T>
        struct node_tuple<std::reference_wrapper<T>, void> : node_tuple<T> {};

        template<class... Args>
        struct node_tuple<std::tuple<Args...>, void> : node_tuple_for<Args...> {};

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct node_tuple<as_optional_t<T>, void> : node_tuple<T> {};
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class... Args>
        struct node_tuple<group_by_t<Args...>, void> : node_tuple_for<Args...> {};

        template<class T, class... Args>
        struct node_tuple<group_by_with_having<T, Args...>, void> : node_tuple_for<Args..., T> {};

#if SQLITE_VERSION_NUMBER >= 3024000
        template<class Targets, class Actions>
        struct node_tuple<upsert_clause<Targets, Actions>, void> : node_tuple<Actions> {};
#endif

        template<class... Args>
        struct node_tuple<set_t<Args...>, void> : node_tuple_for<Args...> {};

        template<class T, class X, class Y, class Z>
        struct node_tuple<highlight_t<T, X, Y, Z>, void> : node_tuple_for<X, Y, Z> {};

        template<class T>
        struct node_tuple<excluded_t<T>, void> : node_tuple<T> {};

        template<class C>
        struct node_tuple<where_t<C>, void> : node_tuple<C> {};

        template<class T, class X>
        struct node_tuple<match_t<T, X>, void> : node_tuple<X> {};

        /**
         *  Column alias
         */
        template<class A>
        struct node_tuple<alias_holder<A>, void> : node_tuple<void> {};

        /**
         *  Column alias
         */
        template<char... C>
        struct node_tuple<column_alias<C...>, void> : node_tuple<void> {};

        /**
         *  Literal
         */
        template<class T>
        struct node_tuple<literal_holder<T>, void> : node_tuple<void> {};

        template<class E>
        struct node_tuple<order_by_t<E>, void> : node_tuple<E> {};

        template<class L, class R>
        struct node_tuple<is_equal_with_table_t<L, R>, void> : node_tuple<R> {};

        template<class T>
        struct node_tuple<T, match_if<is_binary_condition, T>> : node_tuple_for<left_type_t<T>, right_type_t<T>> {};

        template<class T>
        struct node_tuple<T, match_if<is_binary_operator, T>> : node_tuple_for<left_type_t<T>, right_type_t<T>> {};

        template<class... Args>
        struct node_tuple<columns_t<Args...>, void> : node_tuple_for<Args...> {};

        template<class T, class... Args>
        struct node_tuple<struct_t<T, Args...>, void> : node_tuple_for<Args...> {};

        template<class L, class A>
        struct node_tuple<dynamic_in_t<L, A>, void> : node_tuple_for<L, A> {};

        template<class L, class... Args>
        struct node_tuple<in_t<L, Args...>, void> : node_tuple_for<L, Args...> {};

        template<class T>
        struct node_tuple<T, match_if<is_compound_operator, T>> : node_tuple<typename T::expressions_tuple> {};

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
        template<class CTE>
        struct node_tuple<CTE, match_specialization_of<CTE, common_table_expression>>
            : node_tuple<typename CTE::expression_type> {};

        template<class With>
        struct node_tuple<With, match_specialization_of<With, with_t>>
            : node_tuple_for<typename With::cte_type, typename With::expression_type> {};
#endif

        template<class T, class... Args>
        struct node_tuple<select_t<T, Args...>, void> : node_tuple_for<T, Args...> {};

        template<class... Args>
        struct node_tuple<insert_raw_t<Args...>, void> : node_tuple_for<Args...> {};

        template<class... Args>
        struct node_tuple<replace_raw_t<Args...>, void> : node_tuple_for<Args...> {};

        template<class T>
        struct node_tuple<into_t<T>, void> : node_tuple<void> {};

        template<class... Args>
        struct node_tuple<values_t<Args...>, void> : node_tuple_for<Args...> {};

        template<class T, class R, class... Args>
        struct node_tuple<get_all_t<T, R, Args...>, void> : node_tuple_for<Args...> {};

        template<class T, class... Args>
        struct node_tuple<get_all_pointer_t<T, Args...>, void> : node_tuple_for<Args...> {};

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Args>
        struct node_tuple<get_all_optional_t<T, Args...>, void> : node_tuple_for<Args...> {};
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class... Args, class... Wargs>
        struct node_tuple<update_all_t<set_t<Args...>, Wargs...>, void> : node_tuple_for<Args..., Wargs...> {};

        template<class T, class... Args>
        struct node_tuple<remove_all_t<T, Args...>, void> : node_tuple_for<Args...> {};

        template<class T, class E>
        struct node_tuple<cast_t<T, E>, void> : node_tuple<E> {};

        template<class T>
        struct node_tuple<exists_t<T>, void> : node_tuple<T> {};

        template<class T>
        struct node_tuple<optional_container<T>, void> : node_tuple<T> {};

        template<class A, class T, class E>
        struct node_tuple<like_t<A, T, E>, void> : node_tuple_for<A, T, E> {};

        template<class A, class T>
        struct node_tuple<glob_t<A, T>, void> : node_tuple_for<A, T> {};

        template<class A, class T>
        struct node_tuple<between_t<A, T>, void> : node_tuple_for<A, T, T> {};

        template<class T>
        struct node_tuple<named_collate<T>, void> : node_tuple<T> {};

        template<class T>
        struct node_tuple<is_null_t<T>, void> : node_tuple<T> {};

        template<class T>
        struct node_tuple<is_not_null_t<T>, void> : node_tuple<T> {};

        template<class C>
        struct node_tuple<negated_condition_t<C>, void> : node_tuple<C> {};

        template<class R, class S, class... Args>
        struct node_tuple<built_in_function_t<R, S, Args...>, void> : node_tuple_for<Args...> {};

        template<class R, class S, class... Args>
        struct node_tuple<built_in_aggregate_function_t<R, S, Args...>, void> : node_tuple_for<Args...> {};

        template<class F, class W>
        struct node_tuple<filtered_aggregate_function<F, W>, void> : node_tuple_for<F, W> {};

        template<class F, class... Args>
        struct node_tuple<function_call<F, Args...>, void> : node_tuple_for<Args...> {};

        template<class T, class O>
        struct node_tuple<left_join_t<T, O>, void> : node_tuple<O> {};

        template<class T>
        struct node_tuple<on_t<T>, void> : node_tuple<T> {};

        // note: not strictly necessary as there's no binding support for USING;
        // we provide it nevertheless, in line with on_t.
        template<class T, class M>
        struct node_tuple<using_t<T, M>, void> : node_tuple<column_pointer<T, M>> {};

        template<class T, class O>
        struct node_tuple<join_t<T, O>, void> : node_tuple<O> {};

        template<class T, class O>
        struct node_tuple<left_outer_join_t<T, O>, void> : node_tuple<O> {};

        template<class T, class O>
        struct node_tuple<inner_join_t<T, O>, void> : node_tuple<O> {};

        template<class R, class T, class E, class... Args>
        struct node_tuple<simple_case_t<R, T, E, Args...>, void> : node_tuple_for<T, Args..., E> {};

        template<class L, class R>
        struct node_tuple<std::pair<L, R>, void> : node_tuple_for<L, R> {};

        template<class T, class E>
        struct node_tuple<as_t<T, E>, void> : node_tuple<E> {};

        template<class T>
        struct node_tuple<limit_t<T, false, false, void>, void> : node_tuple<T> {};

        template<class T, class O>
        struct node_tuple<limit_t<T, true, false, O>, void> : node_tuple_for<T, O> {};

        template<class T, class O>
        struct node_tuple<limit_t<T, true, true, O>, void> : node_tuple_for<O, T> {};
    }
}
