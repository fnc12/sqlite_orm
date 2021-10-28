#pragma once

#include <type_traits>  //  std::false_type, std::true_type, std::integral_constant

namespace sqlite_orm {

    //  got from here
    //  https://stackoverflow.com/questions/37617677/implementing-a-compile-time-static-if-logic-for-different-string-types-in-a-co
    namespace internal {

        static inline decltype(auto) empty_callable() {
            static auto res = [](auto&&...) {};
            return (res);
        }

        template<typename T, typename F>
        decltype(auto) static_if(std::true_type, const T& t, const F&) {
            return (t);
        }

        template<typename T, typename F>
        decltype(auto) static_if(std::false_type, const T&, const F& f) {
            return (f);
        }

        template<bool B, typename T, typename F>
        decltype(auto) static_if(const T& t, const F& f) {
            return static_if(std::integral_constant<bool, B>{}, t, f);
        }

        template<bool B, typename T>
        decltype(auto) static_if(const T& t) {
            return static_if(std::integral_constant<bool, B>{}, t, empty_callable());
        }

        template<typename T>
        using static_not = std::integral_constant<bool, !T::value>;
    }

}
