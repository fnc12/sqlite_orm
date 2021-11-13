#pragma once

#include <type_traits>  //  std::true_type, std::false_type

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct into_t {
            using type = T;
        };

        template<class T>
        struct is_into : std::false_type {};

        template<class T>
        struct is_into<into_t<T>> : std::true_type {};
    }

    template<class T>
    internal::into_t<T> into() {
        return {};
    }
}
