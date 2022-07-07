#pragma once

#include <utility>  //  std::forward, std::move

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/tuple.h"

namespace sqlite_orm {
    namespace internal {

        template<class T, class... Args>
        struct group_by_with_having {
            using args_type = mpl::tuple<Args...>;
            using expression_type = T;

            args_type args;
            expression_type expression;
        };

        /**
         *  GROUP BY pack holder.
         */
        template<class... Args>
        struct group_by_t {
            using args_type = mpl::tuple<Args...>;

            args_type args;

            template<class T>
            group_by_with_having<T, Args...> having(T expression) {
                return {move(this->args), std::move(expression)};
            }
        };

        template<class T>
        using is_group_by = polyfill::disjunction<polyfill::is_specialization_of<T, group_by_t>,
                                                  polyfill::is_specialization_of<T, group_by_with_having>>;

        /**
         *  HAVING holder.
         *  T is having argument type.
         */
        template<class T>
        struct having_t {
            using expression_type = T;

            expression_type expression;
        };

        template<class T>
        using is_having = polyfill::is_specialization_of<T, having_t>;
    }

    /**
     *  GROUP BY column.
     *  Example: storage.get_all<Employee>(group_by(&Employee::name))
     */
    template<class... Args>
    internal::group_by_t<Args...> group_by(Args&&... args) {
        return {mpl::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  [Deprecation notice]: this function is deprecated and will be removed in v1.9. Please use `group_by(...).having(...)` instead.
     *
     *  HAVING(expression).
     *  Example: storage.get_all<Employee>(group_by(&Employee::name), having(greater_than(count(&Employee::name), 2)));
     */
    template<class T>
    internal::having_t<T> having(T expression) {
        return {std::move(expression)};
    }
}
