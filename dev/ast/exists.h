#pragma once

#include <utility>  //  std::move

#include "../tags.h"

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct exists_t : condition_t, negatable_t {
            using expression_type = T;
            using self = exists_t<expression_type>;

            expression_type expression;

            exists_t(expression_type expression_) : expression(std::move(expression_)) {}
        };
    }

    /**
     *  EXISTS(condition).
     *  Example: storage.select(columns(&Agent::code, &Agent::name, &Agent::workingArea, &Agent::comission),
         where(exists(select(asterisk<Customer>(),
         where(is_equal(&Customer::grade, 3) and
         is_equal(&Agent::code, &Customer::agentCode))))),
         order_by(&Agent::comission));
     */
    template<class T>
    internal::exists_t<T> exists(T expression) {
        return {std::move(expression)};
    }
}
