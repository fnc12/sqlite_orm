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
     *  storage element of a unique tuple
     */
    template<class X, bool UseEBO = std::is_empty<X>::value && !std::is_final<X>::value>
    struct uplem {
        X data;

        constexpr uplem() : data() {}

        template<class Y>
        constexpr uplem(Y&& y) : data(std::forward<Y>(y)) {}
    };

    /*
     *  storage element of a unique tuple, using EBO
     */
    template<class X>
    struct uplem<X, true> : X {

        constexpr uplem() = default;

        template<class Y>
        constexpr uplem(Y&& y) : X(std::forward<Y>(y)) {}
    };

    template<class X>
    constexpr const X& ebo_get(const uplem<X, false>& elem) {
        return (elem.data);
    }
    template<class X>
    constexpr X& ebo_get(uplem<X, false>& elem) {
        return (elem.data);
    }
    template<class X>
    constexpr X&& ebo_get(uplem<X, false>&& elem) {
        return std::forward<X>(elem.data);
    }
    template<class X>
    constexpr const X& ebo_get(const uplem<X, true>& elem) {
        return elem;
    }
    template<class X>
    constexpr X& ebo_get(uplem<X, true>& elem) {
        return elem;
    }
    template<class X>
    constexpr X&& ebo_get(uplem<X, true>&& elem) {
        return std::forward<X>(elem);
    }
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            using ::_sqlite_orm::ebo_get;
            using ::_sqlite_orm::uplem;

            template<class... X>
            struct uple;

            template<bool DefaultOrDirect, class Tuple, class... Void>
            struct enable_tuple_ctor;

            template<class... X, class... Void>
            struct enable_tuple_ctor<true, uple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Void...>), bool> {};

            template<class... X, class... Void>
            struct enable_tuple_ctor<false, uple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};

            template<bool SameNumberOfElements, typename Tuple, typename... Y>
            struct enable_tuple_variadic_ctor;

            template<typename... X, typename... Y>
            struct enable_tuple_variadic_ctor<true, uple<X...>, Y...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> {};

            template<class Tuple, class Other, class... Void>
            struct enable_tuple_nonconst_copy_ctor;

#ifdef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
            template<class... X, class... Void>
            struct enable_tuple_nonconst_copy_ctor<uple<X...>, uple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};

            template<class... X, class... Void>
            struct enable_tuple_nonconst_copy_ctor<uple<X...>, const uple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};
#endif
        }
    }

    namespace mpl = mpl;
}

// retain stl tuple interface for `uple`
namespace std {
    template<class... X>
    struct tuple_size<sqlite_orm::mpl::uple<X...>> : integral_constant<size_t, sizeof...(X)> {};

    template<size_t n, class... X>
    constexpr decltype(auto) get(const sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        using uplem_t = uplem<type_at_t<n, X...>>;
        return ebo_get(static_cast<const uplem_t&>(tpl));
    }

    template<size_t n, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        using uplem_t = uplem<type_at_t<n, X...>>;
        return ebo_get(static_cast<uplem_t&>(tpl));
    }

    template<size_t n, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>&& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        using uplem_t = uplem<type_at_t<n, X...>>;
        return ebo_get(static_cast<uplem_t&&>(tpl));
    }

    template<class T, class... X>
    constexpr decltype(auto) get(const sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using sqlite_orm::mpl::uplem;
        using uplem_t = uplem<T>;
        return ebo_get(static_cast<const uplem_t&>(tpl));
    }

    template<class T, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using sqlite_orm::mpl::uplem;
        using uplem_t = uplem<T>;
        return ebo_get(static_cast<uplem_t&>(tpl));
    }

    template<class T, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>&& tpl) noexcept {
        using sqlite_orm::mpl::uplem;
        using uplem_t = uplem<T>;
        return ebo_get(static_cast<uplem_t&&>(tpl));
    }
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            template<>
            struct uple<> final {
                constexpr uple() = default;
            };

            /*
             *  unique tuple, which allows only distinct types.
             */
            template<class... X>
            struct SQLITE_ORM_MSVC_EMPTYBASES uple final : uplem<X>... {
                // default constructor
                template<class... Void, typename enable_tuple_ctor<true, uple, Void...>::type = true>
                constexpr uple() {}

                // direct constructor
                template<class... Void, typename enable_tuple_ctor<false, uple, Void...>::type = true>
                constexpr uple(const X&... x) : uplem<X>(x)... {}

                // converting constructor
                template<class... Y,
                         typename enable_tuple_variadic_ctor<sizeof...(Y) == sizeof...(X), uple, Y...>::type = true>
                constexpr uple(Y&&... y) : uplem<X>(std::forward<Y>(y))... {}

                // converting copy constructor
                template<class... Y,
                         std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, const Y&>), bool> = true>
                constexpr uple(const uple<Y...>& other) : uplem<X>(std::get<Y>(other))... {}

                // converting move constructor
                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> = true>
                constexpr uple(uple<Y...>&& other) : uplem<X>(std::get<Y>(std::move(other)))... {}

#ifndef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
                constexpr uple(const uple&) = default;
                constexpr uple(uple&&) = default;

                // non-const copy constructor.
                // The non-const copy constructor is required to make sure that
                // the converting uple(Y&&...) constructor is _not_ preferred over the copy
                // constructor for unary tuples containing a type that is constructible from uple<...>.
                constexpr uple(uple& other) : uplem<X>(std::get<X>(const_cast<const uple&>(other)))... {}
#else
                template<class Other, typename enable_tuple_nonconst_copy_ctor<uple, Other>::type = true>
                constexpr uple(Other& other) : uplem<X>(std::get<X>(const_cast<const uple&>(other)))... {}
#endif
            };

            template<class... X>
            constexpr auto make_unique_tuple(X&&... x) {
                return uple<std::decay_t<X>...>{std::forward<X>(x)...};
            }

            template<size_t n, typename... X>
            struct type_at<n, uple<X...>> : type_at<n, X...> {};
        }
    }
}
