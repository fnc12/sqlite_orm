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

        template<class St, class T, class SFINAE = void>
        struct column_expression_type;

        /**
         *  Obains the expressions that form the columns of a subselect statement.
         */
        template<class St, class T>
        using column_expression_of_t = typename column_expression_type<St, T>::type;

        template<class St, class T, class SFINAE>
        struct column_expression_type {
            using type = T;
        };

        template<class St, class T>
        struct column_expression_type<St, std::reference_wrapper<T>, void>
            : std::add_lvalue_reference<column_expression_of_t<St, T>> {};

        // No CTE for object expressions.
        template<class St, class T>
        struct column_expression_type<St, object_t<T>, void> {
            static_assert(polyfill::always_false_v<T>, "Selecting an object in a subselect is not allowed.");
        };

        template<class St, class T>
        struct column_expression_type<St, asterisk_t<T>, match_if_not<std::is_base_of, alias_tag, T>>
            : storage_traits::storage_mapped_column_expressions<St, T> {};

        template<class A>
        struct add_column_alias {
            template<typename ColExpr>
            struct apply {
                using type = alias_column_t<A, ColExpr>;
            };
        };
        template<class St, class A>
        struct column_expression_type<St, asterisk_t<A>, match_if<std::is_base_of, alias_tag, A>>
            : tuple_transformer<typename storage_traits::storage_mapped_column_expressions<St, type_t<A>>::type,
                                add_column_alias<A>::template apply> {};

        template<class St, class O, class F, F O::*m>
        struct column_expression_type<St, ice_t<m>, void> : ice_t<m> {};

        template<class St, class... Args>
        struct column_expression_type<St, columns_t<Args...>, void> {
            using type = std::tuple<column_expression_of_t<St, std::decay_t<Args>>...>;
        };

        template<class St, class T, class... Args>
        struct column_expression_type<St, select_t<T, Args...>> : column_expression_type<St, T> {};
    }
}
