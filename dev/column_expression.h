#pragma once

#include <type_traits>  //  std::enable_if, std::is_same, std::decay, std::is_arithmetic
#include <tuple>  //  std::tuple
#include <functional>  //  std::reference_wrapper

#include "functional/cxx_type_traits_polyfill.h"
#include "tuple_helper/tuple_transformer.h"
#include "type_traits.h"
#include "select_constraints.h"
#include "alias.h"
#include "storage_traits.h"

namespace sqlite_orm {

    namespace internal {

        template<class DBOs, class E, class SFINAE = void>
        struct column_expression_type;

        /**
         *  Obains the expressions that form the columns of a subselect statement.
         */
        template<class DBOs, class E>
        using column_expression_of_t = typename column_expression_type<DBOs, E>::type;

        /**
         *  Identity.
         */
        template<class DBOs, class E, class SFINAE>
        struct column_expression_type {
            using type = E;
        };

        /**
         *  Resolve column alias.
         *  as_t<Alias, E> -> as_t<Alias, ColExpr>
         */
        template<class DBOs, class As>
        struct column_expression_type<DBOs, As, match_specialization_of<As, as_t>> {
            using type = as_t<alias_type_t<As>, column_expression_of_t<DBOs, expression_type_t<As>>>;
        };

        /**
         *  Resolve reference wrapper.
         *  reference_wrapper<T> -> T&
         */
        template<class DBOs, class E>
        struct column_expression_type<DBOs, std::reference_wrapper<E>, void>
            : std::add_lvalue_reference<column_expression_of_t<DBOs, E>> {};

        // No CTE for object expression.
        template<class DBOs, class E>
        struct column_expression_type<DBOs, object_t<E>, void> {
            static_assert(polyfill::always_false_v<E>, "Selecting an object in a subselect is not allowed.");
        };

        /**
         *  Resolve all columns of a mapped object or CTE.
         *  asterisk_t<O> -> tuple<ColExpr...>
         */
        template<class DBOs, class E>
        struct column_expression_type<DBOs,
                                      asterisk_t<E>,
                                      std::enable_if_t<polyfill::disjunction<polyfill::negation<is_recordset_alias<E>>,
                                                                             is_cte_moniker<E>>::value>>
            : storage_traits::storage_mapped_column_expressions<DBOs, E> {};

        template<class A>
        struct add_column_alias {
            template<typename ColExpr>
            using apply_t = alias_column_t<A, ColExpr>;
        };
        /**
         *  Resolve all columns of an aliased object.
         *  asterisk_t<Alias> -> tuple<alias_column_t<Alias, ColExpr>...>
         */
        template<class DBOs, class A>
        struct column_expression_type<DBOs, asterisk_t<A>, match_if<is_table_alias, A>>
            : tuple_transformer<typename storage_traits::storage_mapped_column_expressions<DBOs, type_t<A>>::type,
                                add_column_alias<A>::template apply_t> {};

        /**
         *  Resolve multiple columns.
         *  columns_t<C...> -> tuple<ColExpr...>
         */
        template<class DBOs, class... Args>
        struct column_expression_type<DBOs, columns_t<Args...>, void> {
            using type = std::tuple<column_expression_of_t<DBOs, std::decay_t<Args>>...>;
        };

        /**
         *  Resolve multiple columns.
         *  struct_t<T, C...> -> tuple<ColExpr...>
         */
        template<class DBOs, class T, class... Args>
        struct column_expression_type<DBOs, struct_t<T, Args...>, void> {
            using type = std::tuple<column_expression_of_t<DBOs, std::decay_t<Args>>...>;
        };

        /**
         *  Resolve column(s) of subselect.
         *  select_t<E, Args...> -> ColExpr, tuple<ColExpr....>
         */
        template<class DBOs, class E, class... Args>
        struct column_expression_type<DBOs, select_t<E, Args...>> : column_expression_type<DBOs, E> {};
    }
}
