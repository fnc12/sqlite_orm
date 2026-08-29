#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple, std::make_tuple
#include <type_traits>  //  std::true_type, std::false_type
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/node_algorithms.h"  // is_statement_clause

namespace sqlite_orm::internal {
    template<class T, class... Args>
    struct group_by_with_having {
        using args_type = std::tuple<Args...>;
        using expression_type = T;

        args_type args;
        expression_type expression;
    };

    /**
     *  GROUP BY pack holder.
     */
    template<class... Args>
    struct group_by_t {
        using args_type = std::tuple<Args...>;

        args_type args;

        template<class T>
        group_by_with_having<T, Args...> having(T expression) {
            static_assert(!is_statement_clause<T>::value,
                          "a HAVING condition must be an expression, not a statement clause");
            return {std::move(this->args), std::move(expression)};
        }
    };

    template<class T>
    constexpr bool is_group_by_v = std::disjunction<polyfill::is_specialization_of<T, group_by_t>,
                                                    polyfill::is_specialization_of<T, group_by_with_having>>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  GROUP BY column.
     *  Example: storage.get_all<Employee>(group_by(&Employee::name))
     */
    template<class... Args>
    internal::group_by_t<Args...> group_by(Args... args) {
        static_assert((!internal::is_statement_clause<Args>::value && ...),
                      "a GROUP BY term must be an expression, not a statement clause");
        return {{std::forward<Args>(args)...}};
    }
}
