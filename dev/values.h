#pragma once

#include <vector>  //  std::vector
#include <initializer_list>
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::false_type, std::true_type

namespace sqlite_orm {

    namespace internal {

        template<class... Args>
        struct values_t {
            using args_tuple = std::tuple<Args...>;

            args_tuple tuple;
        };

        template<class T>
        struct is_values : std::false_type {};

        template<class... Args>
        struct is_values<values_t<Args...>> : std::true_type {};

        template<class T>
        struct dynamic_values_t {
            std::vector<T> vector;
        };

    }

    template<class... Args>
    internal::values_t<Args...> values(Args... args) {
        return {{std::forward<Args>(args)...}};
    }

    template<class T>
    internal::dynamic_values_t<T> values(std::vector<T> vector) {
        return {{move(vector)}};
    }

}
