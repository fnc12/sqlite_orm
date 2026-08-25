#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../tags.h"
#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"

namespace sqlite_orm::internal {
    template<class T>
    struct exists_t : condition_t, negatable_t {
        using expression_type = T;

        expression_type expression;

        exists_t(expression_type expression_) : expression(std::move(expression_)) {}
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  EXISTS(condition).
     *  Example: storage.select(columns(&Agent::code, &Agent::name, &Agent::workingArea, &Agent::comission),
     *  where(exists(select(asterisk<Customer>(),
     *  where(is_equal(&Customer::grade, 3) and
     *  is_equal(&Agent::code, &Customer::agentCode))))),
     *  order_by(&Agent::comission));
     */
    template<class T>
    internal::exists_t<T> exists(T expression) {
        static_assert(polyfill::disjunction<internal::is_select<T>, internal::is_compound_operator<T>>::value,
                      "exists() requires a select statement");
        return {std::move(expression)};
    }
}
