#pragma once

#include <vector>  //  std::vector
#include <initializer_list>
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::false_type, std::true_type

#include "start_macros.h"
#include "cxx_polyfill.h"

namespace sqlite_orm {

    namespace internal {

        template<class... Args>
        struct values_t {
            using args_tuple = std::tuple<Args...>;

            args_tuple tuple;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_values_v = false;
        template<class... Args>
        SQLITE_ORM_INLINE_VAR constexpr bool is_values_v<values_t<Args...>> = true;

        template<class T>
        using is_values = polyfill::bool_constant<is_values_v<T>>;

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
