#pragma once

#include <type_traits>  //  std::integral_constant, std::decay, std::remove_reference, std::is_constructible, std::is_default_constructible, std::enable_if, std::declval
#include <utility>  //  std::move, std::forward
#include <functional>  //  std::equal_to

#include "cxx_universal.h"
#include "cxx_type_traits_polyfill.h"
#include "fast_and.h"
#include "indexed_type.h"
#include "index_sequence_util.h"
#include "pack_util.h"
#include "type_at.h"
#include "tuple_common.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (as seen in boost hana)

    /*
     *  storage element of a tuple
     */
    template<size_t n, class X, bool EBOable = std::is_empty<X>::value && !std::is_final<X>::value>
    struct SQLITE_ORM_MSVC_EMPTYBASES tuplem : indexed_type<n, X> {
        X data;

        constexpr tuplem() : data() {}

        template<class Y>
        constexpr tuplem(Y&& y) : data(std::forward<Y>(y)) {}
    };

    /*
     *  storage element of a tuple, using EBO
     */
    template<size_t n, class X>
    struct SQLITE_ORM_MSVC_EMPTYBASES tuplem<n, X, true> : X, indexed_type<n, X> {

        constexpr tuplem() = default;

        template<class Y>
        constexpr tuplem(Y&& y) : X(std::forward<Y>(y)) {}
    };

    template<size_t n, class X>
    constexpr const X& ebo_get(const tuplem<n, X, false>& elem) {
        return (elem.data);
    }
    template<size_t n, class X>
    constexpr X& ebo_get(tuplem<n, X, false>& elem) {
        return (elem.data);
    }
    template<size_t n, class X>
    constexpr X&& ebo_get(tuplem<n, X, false>&& elem) {
        return std::forward<X>(elem.data);
    }
    template<size_t n, class X>
    constexpr const X& ebo_get(const tuplem<n, X, true>& elem) {
        return elem;
    }
    template<size_t n, class X>
    constexpr X& ebo_get(tuplem<n, X, true>& elem) {
        return elem;
    }
    template<size_t n, class X>
    constexpr X&& ebo_get(tuplem<n, X, true>&& elem) {
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

            template<class... X, class... Void>
            struct enable_tuple_ctor<true, tuple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Void...>), bool> {};

            template<class... X, class... Void>
            struct enable_tuple_ctor<false, tuple<X...>, Void...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, const X&, Void...>), bool> {};

            template<class... X, class... Y>
            struct enable_tuple_variadic_ctor<true, tuple<X...>, Y...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> {};

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

    namespace mpl = internal::mpl;
}

// retain stl tuple interface for `tuple`
namespace std {
    template<class Tpl>
    struct tuple_size;

    template<size_t n, class Tpl>
    struct tuple_element;

    template<class... X>
    struct tuple_size<sqlite_orm::mpl::tuple<X...>> : integral_constant<size_t, sizeof...(X)> {};

    template<size_t n, class... X>
    struct tuple_element<n, sqlite_orm::mpl::tuple<X...>> : sqlite_orm::mpl::type_at<n, sqlite_orm::mpl::tuple<X...>> {
    };

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

            template<size_t... Ix, class... X>
            struct SQLITE_ORM_MSVC_EMPTYBASES basic_tuple<std::index_sequence<Ix...>, X...> : tuplem<Ix, X>... {
                constexpr basic_tuple() = default;

                // variadic constructor
                template<class... Y>
                constexpr basic_tuple(from_variadic_t, Y&&... y) : tuplem<Ix, X>(std::forward<Y>(y))... {}

                // converting copy/move constructor
                template<class Other>
                constexpr basic_tuple(Other&& other) : tuplem<Ix, X>(ebo_get<Ix>(std::forward<Other>(other)))... {}

                // default copy constructor
                constexpr basic_tuple(const basic_tuple&) = default;
                // default move constructor
                constexpr basic_tuple(basic_tuple&&) = default;

                // converting copy/move assignment
                template<class Other>
                SQLITE_ORM_NONCONST_CONSTEXPR void operator=(Other&& other) {
                    int poormansfold[] = {(ebo_get<Ix>(*this) = ebo_get<Ix>(std::forward<Other>(other)), int{})...};
                    (void)poormansfold;
                }

                // default copy assignment
                SQLITE_ORM_NONCONST_CONSTEXPR basic_tuple& operator=(const basic_tuple&) = default;
                // default move assignment
                SQLITE_ORM_NONCONST_CONSTEXPR basic_tuple& operator=(basic_tuple&&) = default;
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
                using base_type = basic_tuple<std::make_index_sequence<sizeof...(X)>, X...>;

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

                // default copy constructor
                constexpr tuple(const tuple&) = default;
                // default move constructor
                constexpr tuple(tuple&&) = default;

                // non-const copy constructor.
                // The non-const copy constructor is required to make sure that
                // the converting tuple(Y&&...) constructor is _not_ preferred over the copy
                // constructor for unary tuples containing a type that is constructible from tuple<...>.
#ifndef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
                constexpr tuple(tuple& other) : base_type{const_cast<const tuple&>(other)} {}
#else
                template<class Other, typename enable_tuple_nonconst_copy_ctor<tuple, Other>::type = true>
                constexpr tuple(Other& other) : base_type{const_cast<const tuple&>(other)} {}
#endif

                // converting copy assignment
                template<class... Y,
                         std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_assignable<X&, const Y&>), bool> = true>
                SQLITE_ORM_NONCONST_CONSTEXPR tuple& operator=(const tuple<Y...>& other) {
                    base_type::operator=(other);
                    return *this;
                }

                // converting move assignment
                template<class... Y, std::enable_if_t<SQLITE_ORM_FAST_AND(std::is_assignable<X&, Y&&>), bool> = true>
                SQLITE_ORM_NONCONST_CONSTEXPR tuple& operator=(tuple<Y...>&& other) {
                    base_type::operator=(std::move(other));
                    return *this;
                }

                // default copy assignment
                SQLITE_ORM_NONCONST_CONSTEXPR tuple& operator=(const tuple&) = default;
                // default move assignment
                SQLITE_ORM_NONCONST_CONSTEXPR tuple& operator=(tuple&&) = default;
            };

            namespace adl {
                template<class... X>
                constexpr auto make_tuple(X&&... x) {
                    return tuple<polyfill::unwrap_ref_decay_t<X>...>{std::forward<X>(x)...};
                }

                template<class... X>
                constexpr tuple<X&&...> forward_as_tuple(X&&... args) noexcept {
                    return {std::forward<X>(args)...};
                }

                template<class... X>
                constexpr tuple<X&...> tie(X&... args) noexcept {
                    return {args...};
                }

                template<class... Tuples, size_t... Ix, size_t... Jx>
                constexpr flatten_types_t<tuple, std::remove_reference_t<Tuples>...>
                tuple_cat_helper(std::index_sequence<Ix...>, std::index_sequence<Jx...>, Tuples&&... tuples) {
                    // Note that for each element in a tuple we have a coordinate pair (i, j),
                    // that is, the length of the lists containing the Is and the Js must be equal:
                    static_assert(sizeof...(Ix) == sizeof...(Jx), "");
                    // It then explodes the tuple of tuples into the return type using the coordinates (i, j) for each element:
                    return {ebo_get<Jx>(std::get<Ix>(adl::forward_as_tuple(std::forward<Tuples>(tuples)...)))...};
                }

                /*
                 *  This implementation of `tuple_cat` takes advantage of treating the specified tuples and
                 *  their elements as a two-dimensional array.
                 *  In order for this to work, the number of all elements combined (Jx) has to match
                 *  a sequence of numbers produced using the number of specified tuples (Ix).
                 *  Hence, each index in the sequence of tuples is duplicated by the number of elements of each respective tuple.
                 *  
                 *  Example, using 3 tuples:
                 *  tuple0<char, int, float>
                 *  tuple1<double>
                 *  tuple0<string, size_t>
                 *
                 *  outer sequence Jx (denoting which element to access):
                 *  for tuple0: <0, 1, 2>
                 *  for tuple1: <0>
                 *  for tuple2: <0, 1>
                 *  flattened -> <0, 1, 2, 0, 0, 1>
                 *
                 *  inner sequence Ix (denoting which tuple to access):
                 *  tuples as index sequence: <0, 1, 2>
                 *  sizes as number sequence: <3, 1, 2>
                 *  for tuple0: <0, 0, 0>
                 *  for tuple1: <1>
                 *  for tuple2: <2, 2>
                 *  flattened -> <0, 0, 0, 1, 2, 2>
                 */
                template<class... Tuples>
                constexpr auto tuple_cat(Tuples&&... tuples) {
                    using tuples_seq = std::make_index_sequence<sizeof...(Tuples)>;
                    using sizes_seq = std::index_sequence<std::tuple_size<std::remove_reference_t<Tuples>>::value...>;
                    using inner = flatten_idxseq_t<spread_idxseq_t<tuples_seq, sizes_seq>>;
                    using outer = typename flatten_idxseq<
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
                        std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuples>>::value>...
#else
                        typename make_idxseq_helper<std::remove_reference_t<Tuples>>::type...
#endif
                        >::type;

                    return tuple_cat_helper(inner{}, outer{}, std::forward<Tuples>(tuples)...);
                }
            }
            using adl::forward_as_tuple;
            using adl::make_tuple;
            using adl::tie;
            using adl::tuple_cat;
        }
    }
}

// ops
namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            // implementation note: we could derive from `type_at<n, X...>` but leverage the fact that `tuple` is derived from `indexed_type`
            template<size_t n, class... X>
            struct type_at<n, tuple<X...>> {
                // implementation note: we could use `get_indexed_type<n>()`, but `ebo_get<n>` is readily available.
                using forwarded_t = decltype(ebo_get<n>(std::declval<tuple<X...>>()));
                using type = remove_rvalue_reference_t<forwarded_t>;
            };

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
            template<size_t... Ix, class... X, class... Y>
            constexpr bool equal_indexable([[maybe_unused]] const tuple<X...>& left,
                                           [[maybe_unused]] const tuple<Y...>& right,
                                           std::index_sequence<Ix...>) {
                constexpr std::equal_to<> predicate = {};
                return (predicate(ebo_get<Ix>(left), ebo_get<Ix>(right)) && ...);
            }
#else
            template<class... X, class... Y>
            constexpr bool equal_indexable(const tuple<X...>&, const tuple<Y...>&, std::index_sequence<>) {
                return true;
            }
            template<size_t n, size_t... Ix, class... X, class... Y>
            constexpr bool
            equal_indexable(const tuple<X...>& left, const tuple<Y...>& right, std::index_sequence<n, Ix...>) {
                return std::equal_to<>{}(ebo_get<n>(left), ebo_get<n>(right)) &&
                       equal_indexable(left, right, std::index_sequence<Ix...>{});
            }
#endif

            template<class... X, class... Y>
            constexpr bool operator==(const tuple<X...>& left, const tuple<Y...>& right) {
                static_assert(sizeof...(X) == sizeof...(Y), "cannot compare tuples of different sizes");
                return equal_indexable(left, right, std::make_index_sequence<sizeof...(X)>{});
            }

            template<class... X, class... Y>
            constexpr bool operator!=(const tuple<X...>& left, const tuple<Y...>& right) {
                static_assert(sizeof...(X) == sizeof...(Y), "cannot compare tuples of different sizes");
                return !equal_indexable(left, right, std::make_index_sequence<sizeof...(X)>{});
            }
        }
    }
}
