#pragma once

#ifdef _IMPORT_STD_MODULE
#include <version>
#else
#include <tuple>  //  std::apply; std::tuple_size
#if __cpp_lib_apply < 201603L
#include <utility>  //  std::forward, std::index_sequence, std::make_index_sequence
#endif
#endif

#include "../functional/cxx_universal.h"  //  ::size_t
#include "../functional/cxx_functional_polyfill.h"  //  std::invoke

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
#if __cpp_lib_apply >= 201603L
            using std::apply;
#else
            template<class Callable, class Tpl, size_t... Idx>
            decltype(auto) apply(Callable&& callable, Tpl&& tpl, std::index_sequence<Idx...>) {
                return polyfill::invoke(std::forward<Callable>(callable), std::get<Idx>(std::forward<Tpl>(tpl))...);
            }

            template<class Callable, class Tpl>
            decltype(auto) apply(Callable&& callable, Tpl&& tpl) {
                constexpr size_t size = std::tuple_size<std::remove_reference_t<Tpl>>::value;
                return apply(std::forward<Callable>(callable),
                             std::forward<Tpl>(tpl),
                             std::make_index_sequence<size>{});
            }
#endif
        }
    }

    namespace polyfill = internal::polyfill;
}
