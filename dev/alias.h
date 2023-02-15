#pragma once

#include <type_traits>  //  std::enable_if, std::is_base_of, std::is_member_pointer, std::remove_const
#include <utility>  //  std::index_sequence, std::make_index_sequence
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <algorithm>  //  std::copy_n

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "alias_traits.h"

namespace sqlite_orm {

    namespace internal {

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
        /*
         *  Helper class to facilitate user-defined string literal operator template
         */
        template<size_t N>
        struct string_identifier_template {
            static constexpr size_t size() {
                return N - 1;
            }

            constexpr string_identifier_template(const char (&id)[N]) {
                std::copy_n(id, N, this->id);
            }

            char id[N];
        };

        template<template<char...> class Alias, string_identifier_template t, size_t... Idx>
        consteval auto to_alias(std::index_sequence<Idx...>) {
            return Alias<t.id[Idx]...>{};
        }
#endif

        /**
         *  This is a common built-in class used for custom single character table aliases.
         *  For convenience there exist public type aliases `alias_a`, `alias_b`, ...
         */
        template<class T, char A, char... X>
        struct table_alias : alias_tag {
            using type = T;

            static std::string get() {
                return {A, X...};
            }
        };

        /**
         *  Column expression with table alias attached like 'C.ID'. This is not a column alias
         */
        template<class T, class C>
        struct alias_column_t {
            using alias_type = T;
            using column_type = C;

            column_type column;
        };

        /*
         * Encapsulates extracting the alias identifier of a non-alias.
         */
        template<class T, class SFINAE = void>
        struct alias_extractor {
            static std::string extract() {
                return {};
            }

            static std::string as_alias() {
                return {};
            }
        };

        /*
         * Encapsulates extracting the alias identifier of an alias.
         * 
         * `extract()` always returns the alias identifier.
         * `as_alias()` is used in contexts where a table is aliased.
         */
        template<class A>
        struct alias_extractor<A, match_if<is_alias, A>> {
            static std::string extract() {
                std::stringstream ss;
                ss << A::get();
                return ss.str();
            }

            // for column and regular table aliases -> alias identifier
            template<class T = A, satisfies_not<std::is_same, polyfill::detected_t<type_t, T>, A> = true>
            static std::string as_alias() {
                return alias_extractor::extract();
            }
        };

        /**
         * Used to store alias for expression
         */
        template<class T, class E>
        struct as_t {
            using alias_type = T;
            using expression_type = E;

            expression_type expression;
        };

        /**
         *  This is a common built-in class used for custom single-character column aliases.
         *  For convenience there exist type aliases `colalias_a`, `colalias_b`, ...
         */
        template<char A, char... X>
        struct column_alias : alias_tag {
            static std::string get() {
                return {A, X...};
            }
        };

        template<class T>
        struct alias_holder {
            using type = T;

            alias_holder() = default;
        };

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
        template<char A, char... C>
        struct table_alias_builder {
            static_assert(sizeof...(C) == 0 && ((A >= 'A' && 'Z' <= A) || (A >= 'a' && 'z' <= A)),
                          "Table alias identifiers shall consist of a single alphabetic character, in order to evade "
                          "clashes with CTE aliases.");

            template<auto t>
            [[nodiscard]] consteval internal::table_alias<std::remove_const_t<decltype(t)>, A, C...> for_() const {
                return {};
            }

            template<class T>
            [[nodiscard]] consteval internal::table_alias<T, A, C...> for_() const {
                return {};
            }
        };
#endif
    }

    /**
     *  @return column with table alias attached. Place it instead of a column statement in case you need to specify a
     *  column with table alias prefix like 'a.column'.
     */
    template<class A, class C, std::enable_if_t<internal::is_table_alias_v<A>, bool> = true>
    internal::alias_column_t<A, C> alias_column(C c) {
        using table_type = internal::type_t<A>;
        static_assert(std::is_same<polyfill::detected_t<internal::table_type_of_t, C>, table_type>::value,
                      "Column must be from aliased table");
        return {c};
    }

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
    template<auto als,
             class C,
             class A = std::remove_const_t<decltype(als)>,
             std::enable_if_t<internal::is_table_alias_v<A>, bool> = true>
    auto alias_column(C c) {
        using table_type = internal::type_t<A>;
        static_assert(std::is_same_v<polyfill::detected_t<internal::table_type_of_t, C>, table_type>,
                      "Column must be from aliased table");
        return internal::alias_column_t<A, decltype(c)>{c};
    }

    template<class A, class F, std::enable_if_t<internal::is_table_alias_v<A>, bool> = true>
    constexpr auto operator->*(const A& /*tableAlias*/, F field) {
        return alias_column<A>(std::move(field));
    }
#endif

    /** 
     *  Alias a column expression.
     */
    template<class A, class E, internal::satisfies<internal::is_column_alias, A> = true>
    internal::as_t<A, E> as(E expression) {
        return {std::move(expression)};
    }

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
    template<auto als, class E, internal::satisfies<internal::is_column_alias, decltype(als)> = true>
    auto as(E expression) {
        return internal::as_t<std::remove_const_t<decltype(als)>, E>{std::move(expression)};
    }

    /** 
     *  Alias a column expression.
     */
    template<class A, class E, internal::satisfies<internal::is_column_alias, A> = true>
    internal::as_t<A, E> operator>>=(E expression, const A&) {
        return {std::move(expression)};
    }
#endif

    template<class A, internal::satisfies<internal::is_column_alias, A> = true>
    internal::alias_holder<A> get() {
        return {};
    }

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
    template<auto als, internal::satisfies<internal::is_column_alias, decltype(als)> = true>
    auto get() {
        return internal::alias_holder<std::remove_const_t<decltype(als)>>{};
    }
#endif

    template<class T>
    using alias_a = internal::table_alias<T, 'a'>;
    template<class T>
    using alias_b = internal::table_alias<T, 'b'>;
    template<class T>
    using alias_c = internal::table_alias<T, 'c'>;
    template<class T>
    using alias_d = internal::table_alias<T, 'd'>;
    template<class T>
    using alias_e = internal::table_alias<T, 'e'>;
    template<class T>
    using alias_f = internal::table_alias<T, 'f'>;
    template<class T>
    using alias_g = internal::table_alias<T, 'g'>;
    template<class T>
    using alias_h = internal::table_alias<T, 'h'>;
    template<class T>
    using alias_i = internal::table_alias<T, 'i'>;
    template<class T>
    using alias_j = internal::table_alias<T, 'j'>;
    template<class T>
    using alias_k = internal::table_alias<T, 'k'>;
    template<class T>
    using alias_l = internal::table_alias<T, 'l'>;
    template<class T>
    using alias_m = internal::table_alias<T, 'm'>;
    template<class T>
    using alias_n = internal::table_alias<T, 'n'>;
    template<class T>
    using alias_o = internal::table_alias<T, 'o'>;
    template<class T>
    using alias_p = internal::table_alias<T, 'p'>;
    template<class T>
    using alias_q = internal::table_alias<T, 'q'>;
    template<class T>
    using alias_r = internal::table_alias<T, 'r'>;
    template<class T>
    using alias_s = internal::table_alias<T, 's'>;
    template<class T>
    using alias_t = internal::table_alias<T, 't'>;
    template<class T>
    using alias_u = internal::table_alias<T, 'u'>;
    template<class T>
    using alias_v = internal::table_alias<T, 'v'>;
    template<class T>
    using alias_w = internal::table_alias<T, 'w'>;
    template<class T>
    using alias_x = internal::table_alias<T, 'x'>;
    template<class T>
    using alias_y = internal::table_alias<T, 'y'>;
    template<class T>
    using alias_z = internal::table_alias<T, 'z'>;

    using colalias_a = internal::column_alias<'a'>;
    using colalias_b = internal::column_alias<'b'>;
    using colalias_c = internal::column_alias<'c'>;
    using colalias_d = internal::column_alias<'d'>;
    using colalias_e = internal::column_alias<'e'>;
    using colalias_f = internal::column_alias<'f'>;
    using colalias_g = internal::column_alias<'g'>;
    using colalias_h = internal::column_alias<'h'>;
    using colalias_i = internal::column_alias<'i'>;

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
    /** @short Create aliased tables e.g. `constexpr auto z_alias = alias_<'z'>.for_<User>()`.
     */
    template<char A>
    inline constexpr internal::table_alias_builder<A> alias_{};

    /** @short Create a table alias.
     *
     *  Examples:
     *  constexpr auto z_alias = "z"_alias.for_<User>();
     */
    template<internal::string_identifier_template t>
    [[nodiscard]] consteval auto operator"" _alias() {
        return internal::to_alias<internal::table_alias_builder, t>(std::make_index_sequence<t.size()>{});
    }

    /** @short Create a column alias.
     *  column_alias<'a'[, ...]> from a string literal.
     *  E.g. "a"_col, "b"_col
     */
    template<internal::string_identifier_template t>
    [[nodiscard]] consteval auto operator"" _col() {
        return internal::to_alias<internal::column_alias, t>(std::make_index_sequence<t.size()>{});
    }
#endif
}
