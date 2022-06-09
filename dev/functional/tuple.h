#pragma once

#include <type_traits>  //  std::integral_constant, std::decay, std::is_constructible, std::is_default_constructible, std::enable_if
#include <utility>  //  std::move, std::forward

#include "cxx_universal.h"
#include "cxx_type_traits_polyfill.h"
#include "fast_and.h"
#include "pack.h"
#include "type_at.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (as seen in boost hana)

    /*
     *  element of a tuple
     */
    template<size_t I, class X>
    struct tuplem : indexed_type<I, X> {
        SQLITE_ORM_NOUNIQUEADDRESS X element;

        constexpr tuplem() = default;

        template<class Y>
        constexpr tuplem(Y&& y) : element(std::forward<Y>(y)) {}
    };
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            using ::_sqlite_orm::tuplem;

            struct from_variadic_t {};

            template<class Indices, class... X>
            struct basic_tuple;

            template<size_t... Idx, class... X>
            struct basic_tuple<std::index_sequence<Idx...>, X...> : tuplem<Idx, X>... {
                constexpr basic_tuple() = default;

                template<class... Y>
                constexpr basic_tuple(from_variadic_t, Y&&... y) : tuplem<Idx, X>(std::forward<Y>(y))... {}

                // copy/move constructor
                template<class Other>
                SQLITE_ORM_DELEGATING_CONSTEXPR basic_tuple(Other&& other) :
                    basic_tuple{from_variadic_t{}, std::get<Idx>(std::forward<Other>(other))...} {}
            };

            /*
             *  tuple
             */
            template<class... X>
            struct SQLITE_ORM_MSVC_EMPTYBASES tuple final : basic_tuple<std::make_index_sequence<sizeof...(X)>, X...> {
              private:
                using base_type = basic_tuple<std::make_index_sequence<sizeof...(X)>, X...>;

              public:
#ifdef SQLITE_ORM_CONDITIONAL_EXPLICIT_SUPPORTED
                template<class... Void,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, Void...>...>, bool> = true>
                constexpr explicit(!SQLITE_ORM_FAST_AND(std::is_default_constructible<X>)) tuple() : base_type{} {}
#else
                template<class... Void,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, Void...>...>, bool> = true>
                constexpr tuple() : base_type{} {}
#endif

                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> = true>
                SQLITE_ORM_DELEGATING_CONSTEXPR tuple(Y&&... y) : base_type{from_variadic_t{}, std::forward<Y>(y)...} {}

                template<class... Y,
                         std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, const Y&>), bool> = true>
                constexpr tuple(const tuple<Y...>& other) : base_type{other} {}

                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> = true>
                constexpr tuple(tuple<Y...>&& other) : base_type{std::move(other)} {}

                // The two following constructors are required to make sure that
                // the tuple(Y&&...) constructor is _not_ preferred over the copy
                // constructor for unary tuples containing a type that is constructible
                // from tuple<...>.

                template<class... Void,
                         std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, const X&, Void...>...>,
                                          bool> = true>
                constexpr tuple(const tuple& other) : base_type{other} {}

                template<
                    class... Void,
                    std::enable_if_t<polyfill::conjunction_v<std::is_constructible<X, X&&, Void...>...>, bool> = true>
                constexpr tuple(tuple&& other) : base_type{std::move(other)} {}
            };

            template<>
            struct tuple<> final {
                constexpr tuple() = default;
            };

            namespace adl {
                template<class... X>
                constexpr auto make_tuple(X&&... x) {
                    return tuple<std::decay_t<X>...>{std::forward<X>(x)...};
                }
            }
            using adl::make_tuple;

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
