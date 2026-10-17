#pragma once

/** @file Accessor functions for uniformly obtaining a node's relevant sub-expression or itself,
          independent of which concrete grammar family it belongs to.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::remove_reference, std::remove_const, std::void_t
#include <tuple>  //  std::get
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
    template<class DML>
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
        requires (is_raw_dml_expression_v<DML>)
#endif
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
     *
     *  `is_object_dml_expression` alone would be one notch too wide: a `remove<O>(ids)` names its
     *  object by primary key and carries no object to reach for. Hence the second half.
     */
    template<class DML>
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
        requires (is_object_dml_expression_v<expression_type_t<DML>> &&
                  requires(DML& statement) { statement.expression.object; })
#endif
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
     *  Access the left-most select expression of a compound select statement or the passed in select expression.
     *
     *  The columns of a table formed from a subselect, like a CTE, are those of its left-most select expression.
     *  This is all that is needed to define such a table, and the rest of a compound statement may not even be
     *  resolvable yet - a recursive CTE refers to itself.
     */
    template<class Select, std::enable_if_t<is_select_v<Select>, bool> = true>
    constexpr decltype(auto) access_leftmost_select(const Select& select) {
        if constexpr (is_compound_operator_v<return_type_t<Select>>) {
            return std::get<0>(select.col.compound);
        } else {
            return select;
        }
    }

    /*
     *  Move a possibly quoted plain expression or the expression itself.
     *
     *  No node holds the quoting wrapper `quoted_expression_t`: nothing serializes, iterates or binds one.
     *  Unwrapping is therefore part of normalizing an operand, alongside validating it (`is_operand_or_bindable`,
     *  `are_valid_operands`), and every factory taking operands is to do both, whether it has an operator spelling
     *  or not - the operator factories do, the rest is listed in TODO.md. The alternative, unwrapping only in the overloaded operators, which cannot do without `c()`, was
     *  rejected: it would make a quoted operand usable in one notation but not in the other.
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

// record set accessors
namespace sqlite_orm::internal {
    /** 
     *  Defines the `type` typename to be:
     *  - The unqualified unwrapped table reference type if T is a table reference.
     *  - The unqualified unwrapped table-valued expression type if T is a table-valued expression.
     *  - The unqualified aliased type if T is a recordset alias.
     *  - The enclosing data struct for eponymous virtual tables with hidden columns.
     *  - ... otherwise unqualified T.
     */
    template<class T, class SFINAE = void>
    struct mapped_type_of : std::remove_const<T> {};

    template<class T>
    struct mapped_type_of<T, std::void_t<enclosing_type_t<T>>> {
        using type = enclosing_type_t<T>;
    };

    template<class R>
    struct mapped_type_of<R, match_if<is_table_reference, R>> : R {};

    template<class E>
    struct mapped_type_of<E, match_if<is_table_valued_expression, E>> : E {};

    template<class A>
    struct mapped_type_of<A, match_if<is_recordset_alias, A>> : std::remove_const<type_t<A>> {};

    template<class T>
    using mapped_type_of_t = typename mapped_type_of<T>::type;
}
