#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class... Args>
    struct values_t {
        using args_tuple = std::tuple<Args...>;

        args_tuple tuple;
    };

    template<class T>
    constexpr bool is_values_v = polyfill::is_specialization_of<T, values_t>::value;

    template<class T>
    struct dynamic_values_t {
        std::vector<T> vector;
    };

    template<class T>
    constexpr bool is_dynamic_values_v = polyfill::is_specialization_of<T, dynamic_values_t>::value;

    template<class T>
    constexpr bool is_any_values_v = std::disjunction<is_values<T>, is_dynamic_values<T>>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class... Args>
    internal::values_t<Args...> values(Args... args) {
        return {{std::forward<Args>(args)...}};
    }

    template<class T>
    internal::dynamic_values_t<T> values(std::vector<T> vector) {
        return {{std::move(vector)}};
    }
}
