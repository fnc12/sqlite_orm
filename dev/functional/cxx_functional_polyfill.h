#pragma once
#include <functional>
#if __cpp_lib_invoke < 201411L
#include <type_traits>  //  std::enable_if, std::is_member_object_pointer, std::is_member_function_pointer
#endif
#include <utility>  //  std::forward

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
            // C++20 or later (unfortunately there's no feature test macro).
            // Stupidly, clang < 11 says C++20, but comes w/o std::identity.
            // Another way of detection would be the constrained algorithms feature macro __cpp_lib_ranges
#if(__cplusplus >= 202002L) && (!__clang_major__ || __clang_major__ >= 11)
            using std::identity;
#else
            struct identity {
                template<class T>
                [[nodiscard]] constexpr T&& operator()(T&& v) const noexcept {
                    return std::forward<T>(v);
                }

                using is_transparent = int;
            };
#endif

#if __cpp_lib_invoke >= 201411L
            using std::invoke;
#else
            // pointer-to-data-member+object
            template<class Callable,
                     class Arg1,
                     class... Args,
                     class Unqualified = remove_cvref_t<Callable>,
                     std::enable_if_t<std::is_member_object_pointer<Unqualified>::value, bool> = true>
            decltype(auto) invoke(Callable&& obj, Arg1&& arg1, Args&&... args) {
                return std::forward<Arg1>(arg1).*obj;
            }

            // pointer-to-member-function+object
            template<class Callable,
                     class Arg1,
                     class... Args,
                     class Unqualified = remove_cvref_t<Callable>,
                     std::enable_if_t<std::is_member_function_pointer<Unqualified>::value, bool> = true>
            decltype(auto) invoke(Callable&& obj, Arg1&& arg1, Args&&... args) {
                return (std::forward<Arg1>(arg1).*obj)(std::forward<Args>(args)...);
            }

            // pointer-to-member+reference-wrapped object
            template<class Callable, class Arg1, class... Args>
            decltype(auto) invoke(Callable&& obj, std::reference_wrapper<Arg1> arg1, Args&&... args) {
                return invoke(std::forward<Callable>(obj), arg1.get(), std::forward<Args>(args)...);
            }

            // functor
            template<class Callable, class... Args>
            decltype(auto) invoke(Callable&& obj, Args&&... args) {
                return std::forward<Callable>(obj)(std::forward<Args>(args)...);
            }
#endif
        }
    }

    namespace polyfill = internal::polyfill;
}
