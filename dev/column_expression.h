#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::add_lvalue_reference
#include <functional>  //  std::reference_wrapper
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/mpl.h"
#include "type_traits.h"
#include "tuple_helper/tuple_transformer.h"
#include "vocabulary/node_traits.h"
#include "ast/result_columns.h"
#include "alias.h"
#include "storage_traits.h"

namespace sqlite_orm::internal {
    template<class DBOs, class E, class SFINAE = void>
    struct column_expression_type;

    /**
     *  Obains the expressions that form the columns of a subselect statement.
     */
    template<class DBOs, class E>
    using column_expression_of_t = typename column_expression_type<DBOs, E>::type;

    template<class A>
    struct add_column_alias {
        template<typename ColExpr>
        using apply_t = alias_column_t<A, ColExpr>;
    };

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
    struct column_expression_type<DBOs, As, match_if<is_as_node, As>> {
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
    template<class DBOs, class T>
    struct column_expression_type<DBOs, T, match_if<is_object_node, T>> {
        static_assert(polyfill::always_false_v<T>, "Selecting an object in a subselect is not allowed");
    };

    /**
     *  Resolve all columns of a mapped object or CTE.
     *  asterisk_t<O> -> tuple<ColExpr...>
     */
    template<class DBOs, class T>
    struct column_expression_type<
        DBOs,
        T,
        std::enable_if_t<is_asterisk_v<T> &&
                         std::disjunction_v<std::negation<is_recordset_alias<type_t<T>>>, is_cte_moniker<type_t<T>>>>>
        : storage_traits::storage_mapped_column_expressions<DBOs, type_t<T>> {};

    /**
     *  Resolve all columns of an aliased object.
     *  asterisk_t<Alias> -> tuple<alias_column_t<Alias, ColExpr>...>
     */
    template<class DBOs, class T>
    struct column_expression_type<DBOs, T, std::enable_if_t<is_asterisk_v<T> && is_table_alias_v<type_t<T>>>>
        : tuple_transformer<typename storage_traits::storage_mapped_column_expressions<DBOs, type_t<type_t<T>>>::type,
                            add_column_alias<type_t<T>>::template apply_t> {};

    /**
     *  Resolve multiple columns.
     *  columns_t<C...> -> tuple<ColExpr...>
     */
    template<class DBOs, class T>
    struct column_expression_type<DBOs, T, match_if<is_columns, T>>
        : tuple_transformer<columns_type_t<T>, mpl::bind_front_fn<column_expression_of_t, DBOs>::template fn> {};

    /**
     *  Resolve multiple columns.
     *  struct_t<T, C...> -> tuple<ColExpr...>
     */
    template<class DBOs, class T>
    struct column_expression_type<DBOs, T, match_if<is_struct, T>>
        : tuple_transformer<columns_type_t<T>, mpl::bind_front_fn<column_expression_of_t, DBOs>::template fn> {};

    /**
     *  Resolve column(s) of subselect.
     *  select_t<E, Args...> -> ColExpr, tuple<ColExpr....>
     */
    template<class DBOs, class T>
    struct column_expression_type<DBOs, T, match_if<is_select, T>> : column_expression_type<DBOs, return_type_t<T>> {};
}
