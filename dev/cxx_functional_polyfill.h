#pragma once
#if __cplusplus < 201703L  // C++14 or earlier
#include <type_traits>  //  std::enable_if, std::is_member_object_pointer, std::is_member_function_pointer
#endif
#include <functional>

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
#if __cplusplus < 201703L  // C++14 or earlier
            // pointer-to-data-member
            template<class Callable,
                     class Arg1,
                     class... Args,
                     class Unqualified = remove_cvref_t<Callable>,
                     std::enable_if_t<std::is_member_object_pointer<Unqualified>::value, bool> = true>
            decltype(auto) invoke(Callable&& obj, Arg1&& arg1, Args&&... args) {
                return static_cast<Arg1&&>(arg1).*obj;
            }

            // pointer-to-member-function
            template<class Callable,
                     class Arg1,
                     class... Args,
                     class Unqualified = remove_cvref_t<Callable>,
                     std::enable_if_t<std::is_member_function_pointer<Unqualified>::value, bool> = true>
            decltype(auto) invoke(Callable&& obj, Arg1&& arg1, Args&&... args) {
                return (static_cast<Arg1&&>(arg1).*obj)(static_cast<Args&&>(args)...);
            }
#else
            using std::invoke;
#endif
        }
    }

    namespace polyfill = internal::polyfill;
}
