#pragma once

#ifndef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
#include <type_traits>  //  std::false_type, std::true_type, std::integral_constant
#endif
#include <utility>  //  std::forward

namespace sqlite_orm {

    //  got from here
    //  https://stackoverflow.com/questions/37617677/implementing-a-compile-time-static-if-logic-for-different-string-types-in-a-co
    namespace internal {

        // note: this is a class template accompanied with a variable template because older compilers (e.g. VC 2017)
        // cannot handle a static lambda variable inside a template function
        template<class R>
        struct empty_callable_t {
            template<class... Args>
            R operator()(Args&&...) const {
                return R();
            }
        };
        template<class R = void>
        constexpr empty_callable_t<R> empty_callable{};

#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
        template<bool B, typename T, typename F>
        decltype(auto) static_if([[maybe_unused]] T&& trueFn, [[maybe_unused]] F&& falseFn) {
            if constexpr(B) {
                return std::forward<T>(trueFn);
            } else {
                return std::forward<F>(falseFn);
            }
        }

        template<bool B, typename T>
        decltype(auto) static_if([[maybe_unused]] T&& trueFn) {
            if constexpr(B) {
                return std::forward<T>(trueFn);
            } else {
                return empty_callable<>;
            }
        }

        template<bool B, typename L, typename... Args>
        void call_if_constexpr([[maybe_unused]] L&& lambda, [[maybe_unused]] Args&&... args) {
            if constexpr(B) {
                lambda(std::forward<Args>(args)...);
            }
        }
#else
        template<typename T, typename F>
        decltype(auto) static_if(std::true_type, T&& trueFn, const F&) {
            return std::forward<T>(trueFn);
        }

        template<typename T, typename F>
        decltype(auto) static_if(std::false_type, const T&, F&& falseFn) {
            return std::forward<F>(falseFn);
        }

        template<bool B, typename T, typename F>
        decltype(auto) static_if(T&& trueFn, F&& falseFn) {
            return static_if(std::integral_constant<bool, B>{}, std::forward<T>(trueFn), std::forward<F>(falseFn));
        }

        template<bool B, typename T>
        decltype(auto) static_if(T&& trueFn) {
            return static_if(std::integral_constant<bool, B>{}, std::forward<T>(trueFn), empty_callable<>);
        }

        template<bool B, typename L, typename... Args>
        void call_if_constexpr(L&& lambda, Args&&... args) {
            static_if<B>(std::forward<L>(lambda))(std::forward<Args>(args)...);
        }
#endif
    }

}
