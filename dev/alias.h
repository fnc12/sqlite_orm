#pragma once

#include <type_traits>  //  std::enable_if, std::is_same
#include <utility>  //  std::make_index_sequence, std::move
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#include <array>
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/mpl/conditional.h"
#include "functional/cstring_literal.h"
#include "type_traits.h"
#include "alias_traits.h"
#include "table_type_of.h"
#include "tags.h"
#include "column_pointer.h"

namespace sqlite_orm {

    namespace internal {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<class T>
        inline constexpr bool is_operator_argument_v<T, std::enable_if_t<orm_column_alias<T>>> = true;
#endif

        /**
         *  This is a common built-in class used for character based table aliases.
         *  For convenience there exist public type aliases `alias_a`, `alias_b`, ...
         *  The easiest way to create a table alias is using `"z"_alias.for_<Object>()`.
         */
        template<class T, char A, char... X>
        struct recordset_alias : alias_tag {
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

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool
            is_operator_argument_v<T, std::enable_if_t<polyfill::is_specialization_of<T, alias_column_t>::value>> =
                true;

        struct basic_table;

        /*
         * Encapsulates extracting the alias identifier of a non-alias.
         * 
         * `extract()` always returns the empty string.
         * `as_alias()` is used in contexts where a table might be aliased, and the empty string is returned.
         * `as_qualifier()` is used in contexts where a table might be aliased, and the given table's name is returned.
         */
        template<class T, class SFINAE = void>
        struct alias_extractor {
            static std::string extract() {
                return {};
            }

            static std::string as_alias() {
                return {};
            }

            template<class X = basic_table>
            static const std::string& as_qualifier(const X& table) {
                return table.name;
            }
        };

        /*
         * Encapsulates extracting the alias identifier of an alias.
         * 
         * `extract()` always returns the alias identifier or CTE moniker.
         * `as_alias()` is used in contexts where a recordset is aliased, and the alias identifier is returned.
         * `as_qualifier()` is used in contexts where a table is aliased, and the alias identifier is returned.
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

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            // for CTE monikers -> empty
            template<class T = A, satisfies<std::is_same, polyfill::detected_t<type_t, T>, A> = true>
            static std::string as_alias() {
                return {};
            }
#endif

            // for regular table aliases -> alias identifier
            template<class T = A, satisfies<is_table_alias, T> = true>
            static std::string as_qualifier(const basic_table&) {
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
         *  Built-in column alias.
         *  For convenience there exist type aliases `colalias_a`, `colalias_b`, ...
         *  The easiest way to create a column alias is using `"xyz"_col`.
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

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool
            is_operator_argument_v<T, std::enable_if_t<polyfill::is_specialization_of<T, alias_holder>::value>> = true;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<char A, char... X>
        struct recordset_alias_builder {
            template<class T>
            [[nodiscard]] consteval recordset_alias<T, A, X...> for_() const {
                return {};
            }

            template<auto t>
            [[nodiscard]] consteval auto for_() const {
                using T = std::remove_const_t<decltype(t)>;
                return recordset_alias<T, A, X...>{};
            }
        };
#endif

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
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
     *  Using a column pointer, create a column reference to an aliased table column.
     *  
     *  Example:
     *  using als = alias_u<User>;
     *  select(alias_column<als>(column<User>(&User::id)))
     */
    template<class A,
             class C,
             std::enable_if_t<
                 polyfill::conjunction<internal::is_table_alias<A>,
                                       polyfill::negation<internal::is_cte_moniker<internal::type_t<A>>>>::value,
                 bool> = true>
    constexpr auto alias_column(C field) {
        using namespace ::sqlite_orm::internal;
        using aliased_type = type_t<A>;
        static_assert(is_field_of_v<C, aliased_type>, "Column must be from aliased table");

        return alias_column_t<A, C>{std::move(field)};
    }

    /**
     *  Using an object member field, create a column reference to an aliased table column.
     *  
     *  @note The object member pointer can be from a derived class without explicitly forming a column pointer.
     *  
     *  Example:
     *  using als = alias_u<User>;
     *  select(alias_column<als>(&User::id))
     */
    template<class A,
             class F,
             class O,
             std::enable_if_t<
                 polyfill::conjunction<internal::is_table_alias<A>,
                                       polyfill::negation<internal::is_cte_moniker<internal::type_t<A>>>>::value,
                 bool> = true>
    constexpr auto alias_column(F O::* field) {
        using namespace ::sqlite_orm::internal;
        using aliased_type = type_t<A>;
        static_assert(is_field_of_v<F O::*, aliased_type>, "Column must be from aliased table");

        using C1 =
            mpl::conditional_t<std::is_same<O, aliased_type>::value, F O::*, column_pointer<aliased_type, F O::*>>;
        return alias_column_t<A, C1>{C1{field}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a column reference to an aliased table column.
     *  
     *  @note An object member pointer can be from a derived class without explicitly forming a column pointer.
     *  
     *  Example:
     *  constexpr orm_table_alias auto als = "u"_alias.for_<User>();
     *  select(alias_column<als>(&User::id))
     */
    template<orm_table_alias auto als, class C>
        requires(!orm_cte_moniker<internal::auto_type_t<als>>)
    constexpr auto alias_column(C field) {
        using namespace ::sqlite_orm::internal;
        using A = decltype(als);
        using aliased_type = type_t<A>;
        static_assert(is_field_of_v<C, aliased_type>, "Column must be from aliased table");

        if constexpr(is_column_pointer_v<C>) {
            return alias_column_t<A, C>{std::move(field)};
        } else if constexpr(std::is_same_v<member_object_type_t<C>, aliased_type>) {
            return alias_column_t<A, C>{field};
        } else {
            // wrap in column_pointer
            using C1 = column_pointer<aliased_type, C>;
            return alias_column_t<A, C1>{{field}};
        }
    }

    /**
     *  Create a column reference to an aliased table column.
     *  
     *  @note An object member pointer can be from a derived class without explicitly forming a column pointer.
     *  
     *  @note (internal) Intentionally place in the sqlite_orm namespace for ADL (Argument Dependent Lookup)
     *  because recordset aliases are derived from `sqlite_orm::alias_tag`
     *  
     *  Example:
     *  constexpr auto als = "u"_alias.for_<User>();
     *  select(als->*&User::id)
     */
    template<orm_table_alias A, class F>
        requires(!orm_cte_moniker<internal::type_t<A>>)
    constexpr auto operator->*(const A& /*tableAlias*/, F field) {
        return alias_column<A>(std::move(field));
    }
#endif

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    /**
     *  Create a column reference to an aliased CTE column.
     */
    template<class A,
             class C,
             std::enable_if_t<
                 polyfill::conjunction_v<internal::is_table_alias<A>, internal::is_cte_moniker<internal::type_t<A>>>,
                 bool> = true>
    constexpr auto alias_column(C c) {
        using namespace ::sqlite_orm::internal;
        using cte_moniker_t = type_t<A>;

        if constexpr(is_column_pointer_v<C>) {
            static_assert(std::is_same<table_type_of_t<C>, cte_moniker_t>::value,
                          "Column pointer must match aliased CTE");
            return alias_column_t<A, C>{c};
        } else {
            auto cp = column<cte_moniker_t>(c);
            return alias_column_t<A, decltype(cp)>{std::move(cp)};
        }
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a column reference to an aliased CTE column.
     *  
     *  @note (internal) Intentionally place in the sqlite_orm namespace for ADL (Argument Dependent Lookup)
     *  because recordset aliases are derived from `sqlite_orm::alias_tag`
     */
    template<orm_table_alias A, class C>
        requires(orm_cte_moniker<internal::type_t<A>>)
    constexpr auto operator->*(const A& /*tableAlias*/, C c) {
        return alias_column<A>(std::move(c));
    }

    /**
     *  Create a column reference to an aliased CTE column.
     */
    template<orm_table_alias auto als, class C>
        requires(orm_cte_moniker<internal::auto_type_t<als>>)
    constexpr auto alias_column(C c) {
        using A = std::remove_const_t<decltype(als)>;
        return alias_column<A>(std::move(c));
    }
#endif
#endif

    /** 
     *  Alias a column expression.
     */
    template<class A, class E, internal::satisfies<internal::is_column_alias, A> = true>
    internal::as_t<A, E> as(E expression) {
        return {std::move(expression)};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /** 
     *  Alias a column expression.
     */
    template<orm_column_alias auto als, class E>
    auto as(E expression) {
        return internal::as_t<decltype(als), E>{std::move(expression)};
    }

    /**
     *  Alias a column expression.
     */
    template<orm_column_alias A, class E>
    internal::as_t<A, E> operator>>=(E expression, const A&) {
        return {std::move(expression)};
    }
#else
    /**
     *  Alias a column expression.
     */
    template<class A, class E, internal::satisfies<internal::is_column_alias, A> = true>
    internal::as_t<A, E> operator>>=(E expression, const A&) {
        return {std::move(expression)};
    }
#endif

    /**
     *  Wrap a column alias in an alias holder.
     */
    template<class T>
    internal::alias_holder<T> get() {
        static_assert(internal::is_column_alias_v<T>, "");
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_column_alias auto als>
    auto get() {
        return internal::alias_holder<decltype(als)>{};
    }
#endif

    template<class T>
    using alias_a = internal::recordset_alias<T, 'a'>;
    template<class T>
    using alias_b = internal::recordset_alias<T, 'b'>;
    template<class T>
    using alias_c = internal::recordset_alias<T, 'c'>;
    template<class T>
    using alias_d = internal::recordset_alias<T, 'd'>;
    template<class T>
    using alias_e = internal::recordset_alias<T, 'e'>;
    template<class T>
    using alias_f = internal::recordset_alias<T, 'f'>;
    template<class T>
    using alias_g = internal::recordset_alias<T, 'g'>;
    template<class T>
    using alias_h = internal::recordset_alias<T, 'h'>;
    template<class T>
    using alias_i = internal::recordset_alias<T, 'i'>;
    template<class T>
    using alias_j = internal::recordset_alias<T, 'j'>;
    template<class T>
    using alias_k = internal::recordset_alias<T, 'k'>;
    template<class T>
    using alias_l = internal::recordset_alias<T, 'l'>;
    template<class T>
    using alias_m = internal::recordset_alias<T, 'm'>;
    template<class T>
    using alias_n = internal::recordset_alias<T, 'n'>;
    template<class T>
    using alias_o = internal::recordset_alias<T, 'o'>;
    template<class T>
    using alias_p = internal::recordset_alias<T, 'p'>;
    template<class T>
    using alias_q = internal::recordset_alias<T, 'q'>;
    template<class T>
    using alias_r = internal::recordset_alias<T, 'r'>;
    template<class T>
    using alias_s = internal::recordset_alias<T, 's'>;
    template<class T>
    using alias_t = internal::recordset_alias<T, 't'>;
    template<class T>
    using alias_u = internal::recordset_alias<T, 'u'>;
    template<class T>
    using alias_v = internal::recordset_alias<T, 'v'>;
    template<class T>
    using alias_w = internal::recordset_alias<T, 'w'>;
    template<class T>
    using alias_x = internal::recordset_alias<T, 'x'>;
    template<class T>
    using alias_y = internal::recordset_alias<T, 'y'>;
    template<class T>
    using alias_z = internal::recordset_alias<T, 'z'>;

    using colalias_a = internal::column_alias<'a'>;
    using colalias_b = internal::column_alias<'b'>;
    using colalias_c = internal::column_alias<'c'>;
    using colalias_d = internal::column_alias<'d'>;
    using colalias_e = internal::column_alias<'e'>;
    using colalias_f = internal::column_alias<'f'>;
    using colalias_g = internal::column_alias<'g'>;
    using colalias_h = internal::column_alias<'h'>;
    using colalias_i = internal::column_alias<'i'>;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /** @short Create a table alias.
     *
     *  Examples:
     *  constexpr orm_table_alias auto z_alias = alias<'z'>.for_<User>();
     */
    template<char A, char... X>
    inline constexpr internal::recordset_alias_builder<A, X...> alias{};

    inline namespace literals {
        /** @short Create a table alias.
         *
         *  Examples:
         *  constexpr orm_table_alias auto z_alias = "z"_alias.for_<User>();
         */
        template<internal::cstring_literal name>
        [[nodiscard]] consteval auto operator"" _alias() {
            return internal::explode_into<internal::recordset_alias_builder, name>(
                std::make_index_sequence<name.size()>{});
        }

        /** @short Create a column alias.
         *  column_alias<'a'[, ...]> from a string literal.
         *  E.g. "a"_col, "b"_col
         */
        template<internal::cstring_literal name>
        [[nodiscard]] consteval auto operator"" _col() {
            return internal::explode_into<internal::column_alias, name>(std::make_index_sequence<name.size()>{});
        }
    }
#endif

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    inline namespace literals {
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
    }
#endif
}
