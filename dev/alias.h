#pragma once

#include <type_traits>  //  std::enable_if, std::is_base_of, std::is_member_pointer
#include <sstream>  //  std::stringstream
#include <string>  //  std::string
#ifdef SQLITE_ORM_WITH_CTE
#include <array>
#endif

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"

namespace sqlite_orm {

    /**
     *  This is base class for every class which is used as a custom table alias, column alias or expression alias.
     *  For more information please look through self_join.cpp example
     */
    struct alias_tag {};

    namespace internal {

        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_alias_v = std::is_base_of<alias_tag, A>::value;

        template<class A>
        using is_alias = polyfill::bool_constant<is_alias_v<A>>;

        /*
         *  Note: Currently, there is no distinguishing property of a column alias other than that it is derived from "alias_tag".
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_alias_v = is_alias_v<A>;

        template<class A>
        using is_column_alias = is_alias<A>;

        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_table_alias_v =
            polyfill::conjunction_v<is_alias<A>, polyfill::is_detected<type_t, A>>;

        template<class A>
        using is_table_alias = polyfill::bool_constant<is_table_alias_v<A>>;

        /**
         *  A CTE alias is a specialization of a table alias, which is both, a storage lookup type (mapping type) and an alias.
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_cte_alias_v =
#ifdef SQLITE_ORM_WITH_CTE
            polyfill::conjunction_v<std::is_base_of<alias_tag, A>,
                                    std::is_same<polyfill::detected_t<type_t, A>, std::remove_const_t<A>>>;
#else
            false;
#endif

        template<class A>
        using is_cte_alias = polyfill::bool_constant<is_cte_alias_v<A>>;

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
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
    }

    namespace internal {

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
         *   In case of a CTE the alias is empty since the CTE identifier is already the alias itself.
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
            // for cte aliases -> empty
            template<class T = A, satisfies<std::is_same, polyfill::detected_t<type_t, T>, A> = true>
            static std::string as_alias() {
                return {};
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
            // CTE feature needs it to implicitly convert a column alias to an alias_holder; see `cte()` factory function
            alias_holder(const T&) noexcept {}
        };

#ifdef SQLITE_ORM_WITH_CTE
        template<size_t n, char... C>
        SQLITE_ORM_CONSTEVAL auto n_to_colalias() {
            constexpr column_alias<'1' + n % 10, C...> colalias{};
            if constexpr(n > 10) {
                return n_to_colalias<n / 10, '1' + n % 10, C...>();
            } else {
                return colalias;
            }
        }

        template<class T>
        inline constexpr bool is_builtin_numeric_column_alias_v = false;
        template<char... C>
        inline constexpr bool is_builtin_numeric_column_alias_v<column_alias<C...>> = ((C >= '0' && C <= '9') && ...);
#endif
    }

    /**
     *  @return column with table alias attached. Place it instead of a column statement in case you need to specify a
     *  column with table alias prefix like 'a.column'. For more information please look through self_join.cpp example
     */
    template<class A, class C, internal::satisfies<internal::is_table_alias, A> = true>
    internal::alias_column_t<A, C> alias_column(C c) {
        static_assert(internal::is_cte_alias_v<internal::type_t<A>> || std::is_member_pointer<C>::value,
                      "alias_column argument must be mapped to a storage");
        return {c};
    }

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
    template<auto als, class C, internal::satisfies<internal::is_table_alias, decltype(als)> = true>
    internal::alias_column_t<decltype(als), C> alias_column(C c) {
        static_assert(internal::is_cte_alias_v<internal::type_t<decltype(als)>> || std::is_member_pointer<C>::value,
                      "alias_column argument must be mapped to a storage");
        return {c};
    }
#endif

    /** 
     *  Alias a column expression.
     */
    template<class A, class E>
    internal::as_t<A, E> as(E expression) {
        return {std::move(expression)};
    }

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
    template<auto als, class E>
    internal::as_t<decltype(als), E> as(E expression) {
        return {std::move(expression)};
    }
#endif

    /** 
     *  Alias a column expression.
     */
    template<class A, class E, internal::satisfies<internal::is_column_alias, A> = true>
    internal::as_t<A, E> operator>>=(E expression, const A&) {
        return {std::move(expression)};
    }

    /** 
     *  Alias a column expression.
     */
    template<class A, class E, internal::satisfies<internal::is_column_alias, A> = true>
    internal::as_t<A, E> operator|=(E expression, const A&) {
        return {std::move(expression)};
    }

    template<class A, internal::satisfies<internal::is_column_alias, A> = true>
    internal::alias_holder<A> get() {
        return {};
    }

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
    template<auto als>
    internal::alias_holder<decltype(als)> get() {
        return {};
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

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
    /**
     *  column_alias<'a'[, ...]> from a string literal.
     *  E.g. "a"_col, "b"_col
     */
    template<internal::string_identifier_template t>
    [[nodiscard]] consteval auto operator"" _col() {
        return internal::to_alias<internal::column_alias, t>(std::make_index_sequence<t.size()>{});
    }
#endif

#ifdef SQLITE_ORM_WITH_CTE
    /**
     *  column_alias<'1'[, ...]> from a numeric literal.
     *  E.g. 1_colalias, 2_colalias
     */
    template<char... Chars>
    [[nodiscard]] SQLITE_ORM_CONSTEVAL auto operator"" _colalias() {
        // numeric identifiers are used for automatically assigning implicit aliases to unaliased column expressions,
        // which start at "1".
        static_assert(std::array{Chars...}[0] > '0');
        return internal::column_alias<Chars...>{};
    }
#endif
}
