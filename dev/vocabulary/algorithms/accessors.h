#pragma once

/** @file Accessor functions for uniformly obtaining a node's relevant sub-expression or itself,
          independent of which concrete grammar family it belongs to.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::remove_reference
#include <utility>  //  std::declval, std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/type_traits.h"  //  match_if, value_unref_type, forward_lvalue_ref
#include "../node_traits.h"

// DML accessors
namespace sqlite_orm::internal {
    /*  
     *  Access the main DML expression of a with clause or the passed in DML expression.
     */
    template<class DML, satisfies<is_raw_dml_expression, DML> = true>
    constexpr decltype(auto) access_main_dml(const DML& dml) {
        if constexpr (is_with_clause_v<DML>) {
            return (dml.expression);
        } else {
            return dml;
        }
    }

    template<class DML>
    using main_dml_t = polyfill::remove_cvref_t<decltype(access_main_dml(std::declval<DML>()))>;

    /*
     *  The mapped object type a DML expression operates on, uniformly across the DSL spellings
     *  that name that object differently.
     */
    template<class T, class SFINAE = void>
    struct expression_object_type;

    //  the object spellings store the object the way it was handed to them, which may be a `reference_wrapper`
    template<class T>
    struct expression_object_type<T, match_if<is_object_dml_expression, T>> : value_unref_type<object_type_t<T>> {};

    //  the range spellings have already deduced the object type from their projection
    template<class T>
    struct expression_object_type<T, std::enable_if_t<std::disjunction_v<is_insert_range<T>, is_replace_range<T>>>> {
        using type = object_type_t<T>;
    };

    template<class T>
    using expression_object_type_t = typename expression_object_type<T>::type;

    template<typename S>
    using statement_object_type_t = expression_object_type_t<expression_type_t<std::remove_reference_t<S>>>;

    /*
     *  Access the mapped object a prepared object DML statement carries.
     */
    template<class DML>
    decltype(auto) access_dml_object(DML& statement) {
        return forward_lvalue_ref(statement.expression.object);
    }
}

namespace sqlite_orm::internal {
    /*  
     *  Access the column expression prefixed by a result set deduplicator (as part of a simple select expression, i.e. distinct, all)
     *  or the column expression itself.
     */
    template<class T>
    decltype(auto) access_column_expression(const T& expression) {
        if constexpr (is_rowset_deduplicator_v<T>) {
            return (expression.expression);
        } else {
            return expression;
        }
    }

    /*  
     *  Access the main select expression of a with clause or the passed in select expression.
     */
    template<class Select, std::enable_if_t<is_select_expression<Select>::value, bool> = true>
    constexpr decltype(auto) access_main_select(const Select& select) {
        if constexpr (is_with_clause_v<Select>) {
            return (select.expression);
        } else {
            return select;
        }
    }

    template<class Select>
    using main_select_t = polyfill::remove_cvref_t<decltype(access_main_select(std::declval<Select>()))>;

    /*  
     *  Move a possibly quoted plain expression or the expression itself.
     */
    template<class T>
    constexpr auto unwrap_expression(T&& expression) {
        if constexpr (is_quoted_expression_v<std::remove_cv_t<T>>) {
            return std::move(expression._value);
        } else {
            return std::move(expression);
        }
    }

    /*  
     *  Access a possibly quoted plain expression or the expression itself.
     */
    template<class T>
    constexpr decltype(auto) unwrap_expression(T& expression) {
        if constexpr (is_quoted_expression_v<std::remove_cv_t<T>>) {
            return (expression._value);
        } else {
            return expression;
        }
    }

    template<class T>
    using unwrap_expression_t = decltype(unwrap_expression(std::declval<T>()));
}