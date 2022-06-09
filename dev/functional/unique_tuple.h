#pragma once

#include <type_traits>  //  std::integral_constant, std::decay, std::is_constructible, std::is_default_constructible, std::enable_if
#include <utility>  //  std::move, std::forward

#include "cxx_universal.h"
#include "cxx_type_traits_polyfill.h"
#include "fast_and.h"
#include "type_at.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (as seen in boost hana)

    /*
     *  element of a unique tuple
     */
    template<class X>
    struct uplem {
        SQLITE_ORM_NOUNIQUEADDRESS X element;

        constexpr uplem() = default;

        template<class Y>
        constexpr uplem(Y&& y) : element(std::forward<Y>(y)) {}
    };
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            using ::_sqlite_orm::uplem;

            /*
             *  unique tuple, which allows only distinct types.
             */
            template<class... X>
            struct SQLITE_ORM_MSVC_EMPTYBASES uple final : uplem<X>... {
#ifdef SQLITE_ORM_CONDITIONAL_EXPLICIT_SUPPORTED
                template<class... Void,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, Void...>...>, bool> = true>
                constexpr explicit(!SQLITE_ORM_FAST_AND(std::is_default_constructible<X>)) uple() {}
#else
                template<class... Void,
                         std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Void...>), bool> = true>
                constexpr uple() {}
#endif

                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> = true>
                constexpr uple(Y&&... y) : uplem<X>(std::forward<Y>(y))... {}

                template<class... Y,
                         std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, const Y&>), bool> = true>
                constexpr uple(const uple<Y...>& other) : uplem<X>(std::get<Y>(other))... {}

                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> = true>
                constexpr uple(uple<Y...>&& other) : uplem<X>(std::get<Y>(std::move(other)))... {}

                // The two following constructors are required to make sure that
                // the tuple(Y&&...) constructor is _not_ preferred over the copy
                // constructor for unary tuples containing a type that is constructible
                // from uple<...>.

                template<class... Void,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, const X&, Void...>...>,
                                          bool> = true>
                constexpr uple(const uple& other) : uplem<X>(std::get<X>(other))... {}

                template<
                    class... Void,
                    std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, X&&, Void...>...>, bool> = true>
                constexpr uple(uple&& other) : uplem<X>(std::get<X>(std::move(other)))... {}
            };

            template<>
            struct uple<> final {
                constexpr uple() = default;
            };

            template<class... X>
            constexpr auto make_unique_tuple(X&&... x) {
                return uple<std::decay_t<X>...>{std::forward<X>(x)...};
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
