#pragma once

#include <vector>  //  std::vector
#include <initializer_list>
#include <tuple>  //  std::tuple

namespace sqlite_orm {

    namespace internal {

        template<class... Args>
        struct values_t {
            std::tuple<Args...> tuple;
        };

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
