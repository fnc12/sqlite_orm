#pragma once

/** @file Accessor functions for uniformly obtaining a node's relevant sub-expression or itself,
          independent of which concrete grammar family it belongs to.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <utility>  //  std::declval
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../node_traits.h"

namespace sqlite_orm::internal {
    template<class T, std::enable_if_t<!is_rowset_deduplicator<T>::value, bool> = true>
    const T& access_column_expression(const T& expression) {
        return expression;
    }

    /*  
     *  Access a column expression prefixed by a result set deduplicator (as part of a simple select expression, i.e. distinct, all)
     */
    template<class D, std::enable_if_t<is_rowset_deduplicator<D>::value, bool> = true>
    const typename D::expression_type& access_column_expression(const D& modifier) {
        return modifier.expression;
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
}