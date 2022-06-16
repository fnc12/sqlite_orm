#pragma once

#include <type_traits>  //  std::integral_constant, std::decay, std::is_constructible, std::is_default_constructible, std::enable_if
#include <utility>  //  std::move, std::forward

#include "cxx_universal.h"
#include "cxx_type_traits_polyfill.h"
#include "fast_and.h"
#include "type_at.h"
#include "tuple_common.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (as seen in boost hana)

    /*
     *  storage element of a unique tuple
     */
    template<class X, bool EBOable = is_ebo_able_v<X>>
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

    template<class X, typename = std::enable_if_t<!is_ebo_able_v<X>>>
    constexpr const X& ebo_get(const uplem<X, false>& elem) {
        return (elem.data);
    }
    template<class X, std::enable_if_t<!is_ebo_able_v<X>, bool> = true>
    constexpr X& ebo_get(uplem<X, false>& elem) {
        return (elem.data);
    }
    template<class X, std::enable_if_t<!is_ebo_able_v<X>, bool> = true>
    constexpr X&& ebo_get(uplem<X, false>&& elem) {
        return std::forward<X>(elem.data);
    }
    template<class X, std::enable_if_t<is_ebo_able_v<X>, bool> = true>
    constexpr const X& ebo_get(const uplem<X, true>& elem) {
        return elem;
    }
    template<class X, std::enable_if_t<is_ebo_able_v<X>, bool> = true>
    constexpr X& ebo_get(uplem<X, true>& elem) {
        return elem;
    }
    template<class X, std::enable_if_t<is_ebo_able_v<X>, bool> = true>
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

            template<class... X, class... Void>
            struct enable_tuple_ctor<true, uple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Void...>), bool> {};

            template<class... X, class... Void>
            struct enable_tuple_ctor<false, uple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};

            template<class... X, class... Y>
            struct enable_tuple_variadic_ctor<true, uple<X...>, Y...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> {};

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

    namespace mpl = internal::mpl;
}

// retain stl tuple interface for `uple`
namespace std {
    template<class Tpl>
    struct tuple_size;

    template<size_t n, class Tpl>
    struct tuple_element;

    template<class... X>
    struct tuple_size<sqlite_orm::mpl::uple<X...>> : integral_constant<size_t, sizeof...(X)> {};

    template<size_t n, class... X>
    struct tuple_element<n, sqlite_orm::mpl::uple<X...>> : sqlite_orm::mpl::type_at<n, sqlite_orm::mpl::uple<X...>> {};

    template<size_t n, class... X>
    constexpr decltype(auto) get(const sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        using type = type_at_t<n, X...>;
        return ebo_get<type>(static_cast<const uplem<type>&>(tpl));
    }

    template<size_t n, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        using type = type_at_t<n, X...>;
        return ebo_get<type>(static_cast<uplem<type>&>(tpl));
    }

    template<size_t n, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>&& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        using type = type_at_t<n, X...>;
        return ebo_get<type>(static_cast<uplem<type>&&>(tpl));
    }

    template<class T, class... X>
    constexpr decltype(auto) get(const sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        return ebo_get<T>(static_cast<const uplem<T>&>(tpl));
    }

    template<class T, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        return ebo_get<T>(static_cast<uplem<T>&>(tpl));
    }

    template<class T, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::uple<X...>&& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        return ebo_get<T>(static_cast<uplem<T>&&>(tpl));
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
                constexpr uple(const uple<Y...>& other) : uplem<X>(ebo_get<Y>(other))... {}

                // converting move constructor
                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> = true>
                constexpr uple(uple<Y...>&& other) : uplem<X>(ebo_get<Y>(std::move(other)))... {}

                // default copy constructor
                constexpr uple(const uple&) = default;
                // default move constructor
                constexpr uple(uple&&) = default;

                // non-const copy constructor.
                // The non-const copy constructor is required to make sure that
                // the converting uple(Y&&...) constructor is _not_ preferred over the copy
                // constructor for unary tuples containing a type that is constructible from uple<...>.
#ifndef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
                constexpr uple(uple& other) : uplem<X>(ebo_get<X>(const_cast<const uple&>(other)))... {}
#else
                template<class Other, typename enable_tuple_nonconst_copy_ctor<uple, Other>::type = true>
                constexpr uple(Other& other) : uplem<X>(ebo_get<X>(const_cast<const uple&>(other)))... {}
#endif

                // converting copy assignment
                template<class... Y,
                         std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_assignable<X&, const Y&>), bool> = true>
                SQLITE_ORM_NONCONST_CONSTEXPR uple& operator=(const uple<Y...>& other) {
                    int poormansfold[] = {(ebo_get<X>(*this) = ebo_get<Y>(other), int{})...};
                    (void)poormansfold;
                    return *this;
                }

                // converting move assignment
                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_assignable<X&, Y&&>), bool> = true>
                SQLITE_ORM_NONCONST_CONSTEXPR uple& operator=(uple<Y...>&& other) {
                    int poormansfold[] = {(ebo_get<X>(*this) = ebo_get<Y>(std::move(other)), int{})...};
                    (void)poormansfold;
                    return *this;
                }

                // default copy assignment
                SQLITE_ORM_NONCONST_CONSTEXPR uple& operator=(const uple&) = default;
                // default move assignment
                SQLITE_ORM_NONCONST_CONSTEXPR uple& operator=(uple&&) = default;
            };

            template<class... X>
            constexpr auto make_unique_tuple(X&&... x) {
                return uple<polyfill::unwrap_ref_decay_t<X>...>{std::forward<X>(x)...};
            }

            template<class... X>
            constexpr uple<X&&...> forward_as_unique_tuple(X&&... args) noexcept {
                return {std::forward<X>(args)...};
            }

            template<class... X>
            constexpr uple<X&...> tie_unique(X&... args) noexcept {
                return {args...};
            }
        }
    }
}

// ops
namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            template<size_t n, class... X>
            struct type_at<n, uple<X...>> : type_at<n, X...> {};

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
            template<size_t... Ix, class... X, class... Y, std::enable_if_t<sizeof...(Ix) == sizeof...(X), bool> = true>
            constexpr bool equal_indexable([[maybe_unused]] const uple<X...>& left,
                                           [[maybe_unused]] const uple<Y...>& right,
                                           std::index_sequence<Ix...>) {
                return ((ebo_get<X>(left) == ebo_get<Y>(right)) && ...);
            }

            template<size_t... Ix, class... X, class... Y, std::enable_if_t<sizeof...(Ix) != sizeof...(X), bool> = true>
            constexpr bool equal_indexable([[maybe_unused]] const uple<X...>& left,
                                           [[maybe_unused]] const uple<Y...>& right,
                                           std::index_sequence<Ix...>) {
                return ((std::get<Ix>(left) == std::get<Ix>(right)) && ...);
            }
#else
            template<class... X, class... Y>
            constexpr bool equal_indexable(const uple<X...>&, const uple<Y...>&, std::index_sequence<>) {
                return true;
            }
            template<size_t n, size_t... Ix, class... X, class... Y>
            constexpr bool
            equal_indexable(const uple<X...>& left, const uple<Y...>& right, std::index_sequence<n, Ix...>) {
                return (std::get<n>(left) == std::get<n>(right)) &&
                       equal_indexable(left, right, std::index_sequence<Ix...>{});
            }
#endif

            template<class... X, class... Y>
            constexpr bool operator==(const uple<X...>& left, const uple<Y...>& right) {
                static_assert(sizeof...(X) == sizeof...(Y), "cannot compare tuples of different sizes");
                return equal_indexable(left, right, std::make_index_sequence<sizeof...(X)>{});
            }

            template<class... X, class... Y>
            constexpr bool operator!=(const uple<X...>& left, const uple<Y...>& right) {
                static_assert(sizeof...(X) == sizeof...(Y), "cannot compare tuples of different sizes");
                return !equal_indexable(left, right, std::make_index_sequence<sizeof...(X)>{});
            }
        }
    }
}
