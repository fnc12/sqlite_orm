#pragma once

#include <type_traits>  //  std::enable_if, std::is_base_of, std::is_member_pointer, std::remove_const
#include <utility>  //  std::index_sequence, std::make_index_sequence
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <algorithm>  //  std::copy_n

#include "functional/cxx_universal.h"  //  ::size_t
#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "alias_traits.h"

namespace sqlite_orm {

    namespace internal {

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
        };
    }

    /**
     *  @return column with table alias attached. Place it instead of a column statement in case you need to specify a
     *  column with table alias prefix like 'a.column'.
     */
    template<class A, class C, std::enable_if_t<internal::is_table_alias_v<A>, bool> = true>
    internal::alias_column_t<A, C> alias_column(C c) {
        using aliased_type = internal::type_t<A>;
        static_assert(std::is_same<polyfill::detected_t<internal::table_type_of_t, C>, aliased_type>::value,
                      "Column must be from aliased table");
        return {c};
    }

    /** 
     *  Alias a column expression.
     */
    template<class A, class E, internal::satisfies<internal::is_column_alias, A> = true>
    internal::as_t<A, E> as(E expression) {
        return {std::move(expression)};
    }

    template<class A, internal::satisfies<internal::is_column_alias, A> = true>
    internal::alias_holder<A> get() {
        return {};
    }

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
}
