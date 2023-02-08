#pragma once
#include <functional>
#if __cpp_lib_invoke < 201411L
#include <type_traits>  //  std::enable_if, std::is_member_object_pointer, std::is_member_function_pointer
#endif
#include <utility>  //  std::forward

#if __cpp_lib_invoke < 201411L
#include "cxx_type_traits_polyfill.h"
#endif
#include "../member_traits/member_traits.h"

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
            // C++20 or later (unfortunately there's no feature test macro).
            // Stupidly, clang says C++20, but `std::identity` was only implemented in libc++ 13 and libstd++-v3 10
            // (the latter is used on Linux).
            // gcc got it right and reports C++20 only starting with v10.
            // The check here doesn't care and checks the library versions in use.
            //
            // Another way of detection would be the constrained algorithms feature macro __cpp_lib_ranges
#if(__cplusplus >= 202002L) &&                                                                                         \
    ((!_LIBCPP_VERSION || _LIBCPP_VERSION >= 13) && (!_GLIBCXX_RELEASE || _GLIBCXX_RELEASE >= 10))
            using std::identity;
#else
            struct identity {
                template<class T>
                constexpr T&& operator()(T&& v) const noexcept {
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
                     class Object,
                     class... Args,
                     class Unqualified = remove_cvref_t<Callable>,
                     std::enable_if_t<std::is_member_object_pointer<Unqualified>::value, bool> = true>
            decltype(auto) invoke(Callable&& callable, Object&& object, Args&&... args) {
                return std::forward<Object>(object).*callable;
            }

            // pointer-to-member-function+object
            template<class Callable,
                     class Object,
                     class... Args,
                     class Unqualified = remove_cvref_t<Callable>,
                     std::enable_if_t<std::is_member_function_pointer<Unqualified>::value, bool> = true>
            decltype(auto) invoke(Callable&& callable, Object&& object, Args&&... args) {
                return (std::forward<Object>(object).*callable)(std::forward<Args>(args)...);
            }

            // pointer-to-member+reference-wrapped object (expect `reference_wrapper::*`)
            template<class Callable,
                     class Object,
                     class... Args,
                     std::enable_if_t<polyfill::negation_v<polyfill::is_specialization_of<
                                          member_object_type_t<std::remove_reference_t<Callable>>,
                                          std::reference_wrapper>>,
                                      bool> = true>
            decltype(auto) invoke(Callable&& callable, std::reference_wrapper<Object> wrapper, Args&&... args) {
                return invoke(std::forward<Callable>(callable), wrapper.get(), std::forward<Args>(args)...);
            }

            // functor
            template<class Callable, class... Args>
            decltype(auto) invoke(Callable&& callable, Args&&... args) {
                return std::forward<Callable>(callable)(std::forward<Args>(args)...);
            }
#endif
        }
    }

    namespace polyfill = internal::polyfill;
}
