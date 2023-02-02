#pragma once

#include <tuple>  //  std::tuple
#include <string>  //  std::string
#include <memory>  //  std::unique_ptr
#include <type_traits>  //  std::is_same, std::is_member_object_pointer

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "type_traits.h"
#include "member_traits/member_traits.h"
#include "type_is_nullable.h"
#include "constraints.h"

namespace sqlite_orm {

    namespace internal {

        struct column_identifier {

            /**
             *  Column name.
             */
            std::string name;
        };

        struct empty_setter {};

        /*
         *  Encapsulates object member pointers that are used as column fields,
         *  and whose object is mapped to storage.
         *  
         *  G is a member object pointer or member function pointer
         *  S is a member function pointer or `empty_setter`
         */
        template<class G, class S>
        struct column_field {
            using member_pointer_t = G;
            using setter_type = S;
            using object_type = member_object_type_t<G>;
            using field_type = member_field_type_t<G>;

            /**
             *  Member pointer used to read a field value.
             *  If it is a object member pointer it is also used to write a field value.
             */
            const member_pointer_t member_pointer;

            /**
             *  Setter member function to write a field value
             */
            SQLITE_ORM_NOUNIQUEADDRESS
            const setter_type setter;

            /**
             *  Simplified interface for `NOT NULL` constraint
             */
            constexpr bool is_not_null() const {
                return !type_is_nullable<field_type>::value;
            }
        };

        /*
         *  Encapsulates a tuple of column constraints.
         *  
         *  Op... is a constraints pack, e.g. primary_key_t, unique_t etc
         */
        template<class... Op>
        struct column_constraints {
            using constraints_type = std::tuple<Op...>;

            SQLITE_ORM_NOUNIQUEADDRESS
            constraints_type constraints;

            /**
             *  Checks whether contraints are of trait `Trait`
             */
            template<template<class...> class Trait>
            constexpr bool is() const {
                return tuple_has<Trait, constraints_type>::value;
            }

            constexpr bool is_generated() const {
#if SQLITE_VERSION_NUMBER >= 3031000
                return is<is_generated_always>();
#else
                return false;
#endif
            }

            /**
             *  Simplified interface for `DEFAULT` constraint
             *  @return string representation of default value if it exists otherwise nullptr
             */
            std::unique_ptr<std::string> default_value() const;
        };

        /**
         *  Column definition.
         *  
         *  It is a composition of orthogonal information stored in different base classes.
         */
        template<class G, class S, class... Op>
        struct column_t : column_identifier, column_field<G, S>, column_constraints<Op...> {
#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            column_t(std::string name, G memberPointer, S setter, std::tuple<Op...> op) :
                column_identifier{std::move(name)}, column_field<G, S>{memberPointer, setter},
                column_constraints<Op...>{std::move(op)} {}
#endif
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_v = polyfill::is_specialization_of_v<T, column_t>;

        template<class T>
        using is_column = polyfill::bool_constant<is_column_v<T>>;

        template<class Elements, class F>
        using col_index_sequence_with_field_type =
            filter_tuple_sequence_t<Elements,
                                    check_if_is_type<F>::template fn,
                                    field_type_t,
                                    filter_tuple_sequence_t<Elements, is_column>>;

        template<class Elements, template<class...> class TraitFn>
        using col_index_sequence_with = filter_tuple_sequence_t<Elements,
                                                                check_if_tuple_has<TraitFn>::template fn,
                                                                constraints_type_t,
                                                                filter_tuple_sequence_t<Elements, is_column>>;

        template<class Elements, template<class...> class TraitFn>
        using col_index_sequence_excluding = filter_tuple_sequence_t<Elements,
                                                                     check_if_tuple_has_not<TraitFn>::template fn,
                                                                     constraints_type_t,
                                                                     filter_tuple_sequence_t<Elements, is_column>>;
    }

    /**
     *  Column builder function. You should use it to create columns instead of constructor
     */
    template<class M, class... Op, internal::satisfies<std::is_member_object_pointer, M> = true>
    internal::column_t<M, internal::empty_setter, Op...> make_column(std::string name, M m, Op... constraints) {
        static_assert(polyfill::conjunction_v<internal::is_constraint<Op>...>, "Incorrect constraints pack");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::move(name), m, {}, std::make_tuple(constraints...)});
    }

    /**
     *  Column builder function with setter and getter. You should use it to create columns instead of constructor
     */
    template<class G,
             class S,
             class... Op,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true>
    internal::column_t<G, S, Op...> make_column(std::string name, S setter, G getter, Op... constraints) {
        static_assert(std::is_same<internal::setter_field_type_t<S>, internal::getter_field_type_t<G>>::value,
                      "Getter and setter must get and set same data type");
        static_assert(polyfill::conjunction_v<internal::is_constraint<Op>...>, "Incorrect constraints pack");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), getter, setter, std::make_tuple(constraints...)});
    }

    /**
     *  Column builder function with getter and setter (reverse order). You should use it to create columns instead of
     * constructor
     */
    template<class G,
             class S,
             class... Op,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true>
    internal::column_t<G, S, Op...> make_column(std::string name, G getter, S setter, Op... constraints) {
        static_assert(std::is_same<internal::setter_field_type_t<S>, internal::getter_field_type_t<G>>::value,
                      "Getter and setter must get and set same data type");
        static_assert(polyfill::conjunction_v<internal::is_constraint<Op>...>, "Incorrect constraints pack");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), getter, setter, std::make_tuple(constraints...)});
    }
}
