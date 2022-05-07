#pragma once

#include <type_traits>  //  std::false_type, std::true_type, std::integral_constant

namespace sqlite_orm {

    //  got from here
    //  https://stackoverflow.com/questions/37617677/implementing-a-compile-time-static-if-logic-for-different-string-types-in-a-co
    namespace internal {

        template<class R = void>
        static inline decltype(auto) empty_callable() {
            static auto res = [](auto&&...) -> R {
                return R();
            };
            return (res);
        }

        template<typename T, typename F>
        decltype(auto) static_if(std::true_type, T&& t, const F&) {
            return static_cast<T&&>(t);
        }

        template<typename T, typename F>
        decltype(auto) static_if(std::false_type, const T&, F&& f) {
            return static_cast<F&&>(f);
        }

        template<bool B, typename T, typename F>
        decltype(auto) static_if(T&& t, F&& f) {
            return static_if(std::integral_constant<bool, B>{}, std::forward<T>(t), std::forward<F>(f));
        }

        template<bool B, typename T>
        decltype(auto) static_if(T&& t) {
            return static_if(std::integral_constant<bool, B>{}, std::forward<T>(t), empty_callable());
        }
    }

}
