#pragma once

#ifndef _IMPORT_STD_MODULE
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple
#include <utility>  //  std::forward
#endif

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"

_EXPORT_SQLITE_ORM namespace sqlite_orm {

    namespace internal {

        template<class... Args>
        struct values_t {
            using args_tuple = std::tuple<Args...>;

            args_tuple tuple;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_values_v = polyfill::is_specialization_of<T, values_t>::value;

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
        return {{std::move(vector)}};
    }
}
