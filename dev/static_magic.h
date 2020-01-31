#pragma once

#include <type_traits>  //  std::false_type, std::true_type, std::integral_constant

namespace sqlite_orm {

    //  got from here
    //  https://stackoverflow.com/questions/37617677/implementing-a-compile-time-static-if-logic-for-different-string-types-in-a-co
    namespace internal {

        template<typename T, typename F>
        auto static_if(std::true_type, T t, F) {
            return std::move(t);
        }

        template<typename T, typename F>
        auto static_if(std::false_type, T, F f) {
            return std::move(f);
        }

        template<bool B, typename T, typename F>
        auto static_if(T t, F f) {
            return static_if(std::integral_constant<bool, B>{}, std::move(t), std::move(f));
        }

        template<bool B, typename T>
        auto static_if(T t) {
            return static_if(std::integral_constant<bool, B>{}, t, [](auto &&...) {});
        }

        template<typename T>
        using static_not = std::integral_constant<bool, !T::value>;
    }

}
