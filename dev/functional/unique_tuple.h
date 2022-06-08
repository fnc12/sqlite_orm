#pragma once

#include <type_traits>  //  std::integral_constant, std::decay, std::is_constructible, std::enable_if
#include <utility>  //  std::move, std::forward

#include "cxx_universal.h"
#include "cxx_type_traits_polyfill.h"
#include "type_at.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (seen in boost hana)

    /*
     *  element of a unique tuple
     */
    template<class X>
    struct uplem {
        SQLITE_ORM_NOUNIQUEADDRESS X element;
    };
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            using ::_sqlite_orm::uplem;

            /*
             *  unique tuple
             */
            template<class... X>
            struct SQLITE_ORM_MSVC_EMPTYBASES uple final : uplem<X>... {
#ifdef SQLITE_ORM_CONDITIONAL_EXPLICIT_SUPPORTED
                constexpr explicit(!polyfill::conjunction_v<std::is_default_constructible<X>...>) uple() = default;
#else
                constexpr uple() = default;
#endif

                template<class... U,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, U&&>...>, bool> = true>
                constexpr uple(U&&... t) : uplem<X>{std::forward<U>(t)}... {}

                constexpr uple(const uple& other) = default;
                constexpr uple(uple&& other) = default;
            };

            template<class... X>
            auto make_unique_tuple(X&&... t) {
                return uple<std::decay_t<X>...>{std::forward<X>(t)...};
            }

            template<size_t n, typename... X>
            struct type_at<n, uple<X...>> : type_at<n, X...> {};
        }
    }

    namespace mpl = mpl;
}

// retain stl tuple interface for `uple`
namespace std {
    template<class... X>
    struct tuple_size<sqlite_orm::mpl::uple<X...>> : integral_constant<size_t, sizeof...(X)> {};

    template<size_t n, class... X>
    decltype(auto) get(const sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        const uplem<type_at_t<n, X...>>& elem = tpl;
        return (elem.element);
    }

    template<size_t n, class... X>
    decltype(auto) get(sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        uplem<type_at_t<n, X...>>& elem = tpl;
        return (elem.element);
    }

    template<size_t n, class... X>
    decltype(auto) get(sqlite_orm::mpl::uple<X...>&& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        uplem<type_at_t<n, X...>>& elem = tpl;
        return std::move(elem.element);
    }

    template<class T, class... X>
    decltype(auto) get(const sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using sqlite_orm::mpl::uplem;
        const uplem<T>& elem = tpl;
        return (elem.element);
    }

    template<class T, class... X>
    decltype(auto) get(sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using sqlite_orm::mpl::uplem;
        uplem<T>& elem = tpl;
        return (elem.element);
    }

    template<class T, class... X>
    decltype(auto) get(sqlite_orm::mpl::uple<X...>&& tpl) noexcept {
        using sqlite_orm::mpl::uplem;
        uplem<T>& elem = tpl;
        return std::move(elem.element);
    }
}
