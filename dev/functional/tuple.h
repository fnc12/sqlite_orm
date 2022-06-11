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
     *  storage element of a tuple
     */
    template<size_t I, class X, bool UseEBO = std::is_empty<X>::value && !std::is_final<X>::value>
    struct SQLITE_ORM_MSVC_EMPTYBASES tuplem : indexed_type<I, X> {
        X data;

        constexpr tuplem() : data() {}

        template<class Y>
        constexpr tuplem(Y&& y) : data(std::forward<Y>(y)) {}
    };

    /*
     *  storage element of a tuple, using EBO
     */
    template<size_t I, class X>
    struct SQLITE_ORM_MSVC_EMPTYBASES tuplem<I, X, true> : X, indexed_type<I, X> {

        constexpr tuplem() = default;

        template<class Y>
        constexpr tuplem(Y&& y) : X(std::forward<Y>(y)) {}
    };

    template<size_t I, class X>
    constexpr const X& ebo_get(const tuplem<I, X, false>& elem) {
        return (elem.data);
    }
    template<size_t I, class X>
    constexpr X& ebo_get(tuplem<I, X, false>& elem) {
        return (elem.data);
    }
    template<size_t I, class X>
    constexpr X&& ebo_get(tuplem<I, X, false>&& elem) {
        return std::forward<X>(elem.data);
    }
    template<size_t I, class X>
    constexpr const X& ebo_get(const tuplem<I, X, true>& elem) {
        return elem;
    }
    template<size_t I, class X>
    constexpr X& ebo_get(tuplem<I, X, true>& elem) {
        return elem;
    }
    template<size_t I, class X>
    constexpr X&& ebo_get(tuplem<I, X, true>&& elem) {
        return std::forward<X>(elem);
    }
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            using ::_sqlite_orm::ebo_get;
            using ::_sqlite_orm::tuplem;

            template<class Indices, class... X>
            struct basic_tuple;

            template<class... X>
            struct tuple;

            template<class T>
            struct remove_rvalue_reference {
                using type = T;
            };
            template<class T>
            struct remove_rvalue_reference<T&&> {
                using type = T;
            };
            template<class T>
            using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

            struct from_variadic_t {};

            template<bool DefaultOrDirect, class Tuple, class... Void>
            struct enable_tuple_ctor;

            template<class... X, class... Void>
            struct enable_tuple_ctor<true, tuple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Void...>), bool> {};

            template<class... X, class... Void>
            struct enable_tuple_ctor<false, tuple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};

            template<bool SameNumberOfElements, class Tuple, class... Y>
            struct enable_tuple_variadic_ctor;

            template<class... X, class... Y>
            struct enable_tuple_variadic_ctor<true, tuple<X...>, Y...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> {};

            template<class Tuple, class Other, class... Void>
            struct enable_tuple_nonconst_copy_ctor;

#ifdef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
            template<class... X, class... Void>
            struct enable_tuple_nonconst_copy_ctor<tuple<X...>, tuple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};

            template<class... X, class... Void>
            struct enable_tuple_nonconst_copy_ctor<tuple<X...>, const tuple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};
#endif
        }
    }

    namespace mpl = mpl;
}

// retain stl tuple interface for `tuple`
namespace std {
    template<class Tpl>
    struct tuple_size;

    template<class... X>
    struct tuple_size<sqlite_orm::mpl::tuple<X...>> : integral_constant<size_t, sizeof...(X)> {};

    template<size_t n, class... X>
    constexpr decltype(auto) get(const sqlite_orm::mpl::tuple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        return ebo_get<n>(tpl);
    }

    template<size_t n, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::tuple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        return ebo_get<n>(tpl);
    }

    template<size_t n, class... X>
    constexpr decltype(auto) get(sqlite_orm::mpl::tuple<X...>&& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        return ebo_get<n>(std::move(tpl));
    }

    template<class T, class... X>
    decltype(auto) get(const sqlite_orm::mpl::tuple<X...>&) noexcept = delete;

    template<class T, class... X>
    decltype(auto) get(sqlite_orm::mpl::tuple<X...>&) noexcept = delete;

    template<class T, class... X>
    decltype(auto) get(sqlite_orm::mpl::tuple<X...>&&) noexcept = delete;
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            template<size_t... Idx, class... X>
            struct SQLITE_ORM_MSVC_EMPTYBASES basic_tuple<std::index_sequence<Idx...>, X...> : tuplem<Idx, X>... {
                constexpr basic_tuple() = default;

                // variadic constructor
                template<class... Y>
                constexpr basic_tuple(from_variadic_t, Y&&... y) : tuplem<Idx, X>(std::forward<Y>(y))... {}

                // converting copy/move constructor
                template<class Other>
                constexpr basic_tuple(Other&& other) : tuplem<Idx, X>(std::get<Idx>(std::forward<Other>(other)))... {}

                constexpr basic_tuple(const basic_tuple&) = default;
                constexpr basic_tuple(basic_tuple&&) = default;
            };

            template<>
            struct tuple<> final {
                constexpr tuple() = default;
            };

            /*
             *  tuple
             */
            template<class... X>
            struct tuple final : basic_tuple<std::make_index_sequence<sizeof...(X)>, X...> {
              private:
                using base_type = basic_tuple<std::make_index_sequence<sizeof...(X)>, X...>;

              public:
                // default constructor
                template<class... Void, typename enable_tuple_ctor<true, tuple, Void...>::type = true>
                constexpr tuple() : base_type{} {}

                // direct constructor
                template<class... Void, typename enable_tuple_ctor<false, tuple, Void...>::type = true>
                constexpr tuple(const X&... x) : base_type{from_variadic_t{}, x...} {}

                // converting constructor
                template<class... Y,
                         typename enable_tuple_variadic_ctor<sizeof...(Y) == sizeof...(X), tuple, Y...>::type = true>
                constexpr tuple(Y&&... y) : base_type{from_variadic_t{}, std::forward<Y>(y)...} {}

                // converting copy constructor
                template<class... Y,
                         std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, const Y&>), bool> = true>
                constexpr tuple(const tuple<Y...>& other) : base_type{other} {}

                // converting move constructor
                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> = true>
                constexpr tuple(tuple<Y...>&& other) : base_type{std::move(other)} {}

#ifndef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
                constexpr tuple(const tuple&) = default;
                constexpr tuple(tuple&&) = default;

                // non-const copy constructor.
                // The non-const copy constructor is required to make sure that
                // the converting tuple(Y&&...) constructor is _not_ preferred over the copy
                // constructor for unary tuples containing a type that is constructible from tuple<...>.
                constexpr tuple(tuple& other) : base_type{const_cast<const tuple&>(other)} {}
#else
                template<class Other, typename enable_tuple_nonconst_copy_ctor<tuple, Other>::type = true>
                constexpr tuple(Other& other) : base_type{const_cast<const tuple&>(other)} {}
#endif
            };

            namespace adl {
                template<class... X>
                constexpr auto make_tuple(X&&... x) {
                    return tuple<std::decay_t<X>...>{std::forward<X>(x)...};
                }

                template<class... X>
                constexpr tuple<X&&...> forward_as_tuple(X&&... args) noexcept {
                    return tuple<X&&...>(_STD forward<X>(args)...);
                }
            }
            using adl::forward_as_tuple;
            using adl::make_tuple;

            // implementation note: we could derive from `type_at<n, X...>` but leverage the fact that `tuple` is derived from `indexed_type`
            template<size_t n, class... X>
            struct type_at<n, tuple<X...>> {
                // implementation note: we could use `get_indexed_type<n>()`, but `ebo_get<n>` is readily available.
                using forwarded_t = decltype(ebo_get<n>(std::declval<tuple<X...>>()));
                using type = remove_rvalue_reference_t<forwarded_t>;
            };
        }
    }
}
