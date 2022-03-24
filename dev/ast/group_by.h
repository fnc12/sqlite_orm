#pragma once

#include <tuple>  //  std::tuple, std::make_tuple
#include <type_traits>  //  std::true_type, std::false_type
#include <utility>  //  std::forward, std::move

namespace sqlite_orm {
    namespace internal {

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
                return {move(this->args), std::move(expression)};
            }
        };

        template<class T>
        struct is_group_by : std::false_type {};

        template<class... Args>
        struct is_group_by<group_by_t<Args...>> : std::true_type {};

        template<class T, class... Args>
        struct is_group_by<group_by_with_having<T, Args...>> : std::true_type {};

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
        struct is_having : std::false_type {};

        template<class T>
        struct is_having<having_t<T>> : std::true_type {};
    }

    /**
     *  GROUP BY column.
     *  Example: storage.get_all<Employee>(group_by(&Employee::name))
     */
    template<class... Args>
    internal::group_by_t<Args...> group_by(Args&&... args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
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
