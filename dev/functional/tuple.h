#pragma once

#include <type_traits>  //  std::integral_constant, std::decay, std::is_constructible, std::enable_if
#include <utility>  //  std::move, std::forward

#include "cxx_universal.h"
#include "cxx_type_traits_polyfill.h"
#include "pack.h"
#include "type_at.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (seen in boost hana)

    /*
     *  element of a tuple
     */
    template<size_t I, class X>
    struct tuplem : indexed_type<I, X> {
        SQLITE_ORM_NOUNIQUEADDRESS X element;

        tuplem(X t) : element{std::move(t)} {}
    };
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            using ::_sqlite_orm::tuplem;

            template<class Indices, class... X>
            struct basic_tuple;

            template<size_t... Idx, class... X>
            struct basic_tuple<std::index_sequence<Idx...>, X...> : tuplem<Idx, X>... {
#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
                constexpr basic_tuple() = default;

                template<class... U,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, U&&>...>, bool> = true>
                constexpr basic_tuple(U&&... t) : tuplem<Idx, X>{std::forward<U>(t)}... {}
#endif
            };

            /*
             *  tuple
             */
            template<class... X>
            struct SQLITE_ORM_MSVC_EMPTYBASES tuple final : basic_tuple<std::index_sequence_for<X...>, X...> {
#ifdef SQLITE_ORM_CONDITIONAL_EXPLICIT_SUPPORTED
                constexpr explicit(!polyfill::conjunction_v<std::is_default_constructible<X>...>) tuple() = default;
#else
                constexpr tuple() = default;
#endif

                template<class... U,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, U&&>...>, bool> = true>
                constexpr tuple(U&&... t) : basic_tuple<std::index_sequence_for<X...>, X...>{std::forward<U>(t)...} {}

                constexpr tuple(const tuple& other) = default;
                constexpr tuple(tuple&& other) = default;
            };

            template<class... X>
            auto make_tuple(X&&... t) {
                return tuple<std::decay_t<X>...>{std::forward<X>(t)...};
            }

            template<size_t n, typename... X>
            struct type_at<n, tuple<X...>> {
                using indexed_t = decltype(get_indexed_type<n>(std::declval<tuple<X...>>()));
                using type = typename indexed_t::type;
            };
        }
    }

    namespace mpl = mpl;
}

// retain stl tuple interface for `tuple`
namespace std {
    template<class... X>
    struct tuple_size<sqlite_orm::mpl::tuple<X...>> : integral_constant<size_t, sizeof...(X)> {};

    template<size_t n, class... X>
    decltype(auto) get(const sqlite_orm::mpl::tuple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        const tuplem<n, element_at_t<n, tuple<X...>>>& elem = tpl;
        return (elem.element);
    }

    template<size_t n, class... X>
    decltype(auto) get(sqlite_orm::mpl::tuple<X...>& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        tuplem<n, element_at_t<n, tuple<X...>>>& elem = tpl;
        return (elem.element);
    }

    template<size_t n, class... X>
    decltype(auto) get(sqlite_orm::mpl::tuple<X...>&& tpl) noexcept {
        using namespace sqlite_orm::mpl;
        tuplem<n, element_at_t<n, tuple<X...>>>& elem = tpl;
        return std::move(elem.element);
    }

    template<class T, class... X>
    decltype(auto) get(const sqlite_orm::mpl::tuple<X...>&) noexcept = delete;

    template<class T, class... X>
    decltype(auto) get(sqlite_orm::mpl::tuple<X...>&) noexcept = delete;

    template<class T, class... X>
    decltype(auto) get(sqlite_orm::mpl::tuple<X...>&&) noexcept = delete;
}
