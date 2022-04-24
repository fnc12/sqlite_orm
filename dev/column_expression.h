#pragma once

#include <type_traits>  //  std::enable_if, std::is_same, std::decay, std::is_arithmetic
#include <tuple>  //  std::tuple
#include <functional>  //  std::reference_wrapper

#include "cxx_polyfill.h"
#include "type_traits.h"
#include "select_constraints.h"
#include "alias.h"
#include "storage_traits.h"
#include "tuple_helper/tuple_transformer.h"

namespace sqlite_orm {

    namespace internal {

        template<class St, class E, class SFINAE = void>
        struct column_expression_type;

        /**
         *  Obains the expressions that form the columns of a subselect statement.
         */
        template<class St, class E>
        using column_expression_of_t = typename column_expression_type<St, E>::type;

        /**
         *  Identity.
         */
        template<class St, class E, class SFINAE>
        struct column_expression_type {
            using type = E;
        };

        /**
         *  Resolve column alias.
         *  as_t<Alias, E> -> as_t<Alias, ColExpr>
         */
        template<class St, class As>
        struct column_expression_type<St, As, match_specialization_of<As, as_t>> {
            using type = as_t<alias_type_t<As>, column_expression_of_t<St, expression_type_t<As>>>;
        };

        /**
         *  Resolve reference wrapper.
         *  reference_wrapper<T> -> T&
         */
        template<class St, class E>
        struct column_expression_type<St, std::reference_wrapper<E>, void>
            : std::add_lvalue_reference<column_expression_of_t<St, E>> {};

        // No CTE for object expression.
        template<class St, class E>
        struct column_expression_type<St, object_t<E>, void> {
            static_assert(polyfill::always_false_v<E>, "Selecting an object in a subselect is not allowed.");
        };

        /**
         *  Resolve all columns of an object.
         *  asterisk_t<O> -> tuple<ColExpr...>
         */
        template<class St, class E>
        struct column_expression_type<St, asterisk_t<E>, match_if_not<is_table_alias, E>>
            : storage_traits::storage_mapped_column_expressions<St, E> {};

        template<class A>
        struct add_column_alias {
            template<typename ColExpr>
            struct apply {
                using type = alias_column_t<A, ColExpr>;
            };
        };
        /**
         *  Resolve all columns of an aliased object.
         *  asterisk_t<Alias> -> tuple<alias_column_t<Alias, ColExpr>...>
         */
        template<class St, class A>
        struct column_expression_type<St, asterisk_t<A>, match_if<is_table_alias, A>>
            : tuple_transformer<typename storage_traits::storage_mapped_column_expressions<St, type_t<A>>::type,
                                add_column_alias<A>::template apply> {};

        /**
         *  Resolve multiple columns.
         *  columns_t<C...> -> tuple<ColExpr...>
         */
        template<class St, class... Args>
        struct column_expression_type<St, columns_t<Args...>, void> {
            using type = std::tuple<column_expression_of_t<St, std::decay_t<Args>>...>;
        };

        /**
         *  Resolve column(s) of subselect.
         *  select_t<E, Args...> -> ColExpr, tuple<ColExpr....>
         */
        template<class St, class E, class... Args>
        struct column_expression_type<St, select_t<E, Args...>> : column_expression_type<St, E> {};
    }
}
