#pragma once

#include <sqlite3.h>  //  sqlite_int64
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple
#include <string>  //  std::string
#include <memory>  //  std::unique_ptr
#include <type_traits>  //  std::enable_if, std::is_same, std::is_member_object_pointer, std::is_signed
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tuple_helper/tuple_traits.h"
#include "../tuple_helper/tuple_filter.h"
#include "../member_traits/member_traits.h"
#include "../type_traits.h"
#include "../type_is_nullable.h"
#include "../column_constraints.h"

namespace sqlite_orm::internal {
    template<class T>
    using is_column_constraint = mpl::invoke_t<mpl::disjunction<check_if<std::is_base_of, primary_key_t<>>,
                                                                check_if_is_type<null_t>,
                                                                check_if_is_type<not_null_t>,
                                                                check_if_is_type<unique_t<>>,
                                                                check_if_is_template<default_t>,
                                                                check_if_is_template<check_t>,
                                                                check_if_is_type<collate_constraint_t>,
                                                                check_if<is_generated_always>,
                                                                check_if_is_type<unindexed_t>,
                                                                check_if<is_auxiliary>>,
                                               T>;

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
         *  Checks whether constraints contain specified type.
         */
        template<template<class...> class Trait>
        constexpr static bool is() {
            return tuple_has<constraints_type, Trait>::value;
        }

        /**
         *  Checks whether constraints contain specified class template.
         */
        template<template<class...> class Primary>
        constexpr static bool is_template() {
            return tuple_has_template<constraints_type, Primary>::value;
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
    struct column_t : column_identifier, column_field<G, S>, column_constraints<Op...> {};

    /**
     *  Definition of a hidden column.
     *  
     *  Implementation note: it is a separate type to make coding easier - hidden columns do not participate in normal column handling,
     *  e.g. they are not counted as columns when constructing objects, and are only needed when finding columns or for table-valued functions.
     */
    template<class G, class S, class... Op>
    struct hidden_column : column_identifier, column_field<G, S>, column_constraints<Op...> {};

    template<class T, class SFINAE = void>
    struct column_field_expression {
        using type = void;
    };

    template<class G, class S, class... Op>
    struct column_field_expression<column_t<G, S, Op...>, void> {
        using type = typename column_t<G, S, Op...>::member_pointer_t;
    };

    template<typename T>
    using column_field_expression_t = typename column_field_expression<T>::type;

    template<class T>
    inline constexpr bool is_column_v = polyfill::is_specialization_of<T, column_t>::value;

    template<class T>
    using is_column = polyfill::bool_constant<is_column_v<T>>;

    template<class T>
    inline constexpr bool is_hidden_column_v = polyfill::is_specialization_of<T, hidden_column>::value;

    template<class T>
    using is_hidden_column = polyfill::bool_constant<is_hidden_column_v<T>>;

    // Custom type:
    // It is the programmer's responsibility to ensure data integrity in the value range of the custom type
    // and in purview of SQLite using a 64-bit signed integer.
    template<class F, class SFINAE = void>
    struct check_pkcol {
        static constexpr void validate_column_primary_key_with_autoincrement() {}
    };

    // For integer types: further checks
    template<class F>
    struct check_pkcol<F, std::enable_if_t<std::is_integral<F>::value>> {
        // For 64-bit signed integer type: valid
        template<class X = F,
                 std::enable_if_t<sizeof(X) == sizeof(sqlite_int64) &&
                                      std::is_signed<X>::value == std::is_signed<sqlite_int64>::value,
                                  bool> = true>
        static constexpr void validate_column_primary_key_with_autoincrement() {}

        // Design decision for integral types other than 64-bit signed integer:
        // It is the programmer's responsibility to ensure data integrity in the value range of the integral type
        // and in purview of SQLite using a 64-bit signed integer.
        template<class X = F,
                 std::enable_if_t<sizeof(X) != sizeof(sqlite_int64) ||
                                      std::is_signed<X>::value != std::is_signed<sqlite_int64>::value,
                                  bool> = true>
        static constexpr void validate_column_primary_key_with_autoincrement() {}
    };

    // For non-integer types: static_assert failure
    template<class F>
    struct check_pkcol<F, std::enable_if_t<!std::is_base_of<integer_printer, type_printer<F>>::value>> {
        static constexpr void validate_column_primary_key_with_autoincrement() {
            static_assert(polyfill::always_false_v<F>,
                          R"(AUTOINCREMENT is only allowed on an INTEGER PRIMARY KEY as an alias for the "rowid" key)");
        }
    };

    template<class G, class... Op>
    constexpr void validate_column_definition() {
        using constraints_type = std::tuple<Op...>;

        static_assert(polyfill::conjunction_v<is_column_constraint<Op>...>, "Incorrect column constraints");

        if constexpr (tuple_has_template<constraints_type, primary_key_with_autoincrement>::value) {
            check_pkcol<member_field_type_t<G>>::validate_column_primary_key_with_autoincrement();
        }
    }

    /**
     *  Factory function for a column definition from a member object pointer for hidden virtual table columns.
     */
    template<class M, class... Op, satisfies<std::is_member_object_pointer, M> = true>
    hidden_column<M, empty_setter, Op...> make_hidden_column(std::string name, M memberPointer, Op... constraints) {
        static_assert(polyfill::conjunction_v<is_column_constraint<Op>...>, "Incorrect column constraints");

        // attention: do not use `std::make_tuple()` for constructing the tuple member `[[no_unique_address]] column_constraints::constraints`,
        // as this will lead to UB with Clang on MinGW!
        return {std::move(name), memberPointer, {}, std::tuple<Op...>{std::move(constraints)...}};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a column definition from a member object pointer of the object to be mapped.
     */
    template<class M, class... Op, internal::satisfies<std::is_member_object_pointer, M> = true>
    internal::column_t<M, internal::empty_setter, Op...>
    make_column(std::string name, M memberPointer, Op... constraints) {
        internal::validate_column_definition<M, Op...>();

        // attention: do not use `std::make_tuple()` for constructing the tuple member `[[no_unique_address]] column_constraints::constraints`,
        // as this will lead to UB with Clang on MinGW!
        return {std::move(name), memberPointer, {}, std::tuple<Op...>{std::move(constraints)...}};
    }

    /**
     *  Factory function for a column definition from "setter" and "getter" member function pointers of the object to be mapped.
     */
    template<class G,
             class S,
             class... Op,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true>
    internal::column_t<G, S, Op...> make_column(std::string name, S setter, G getter, Op... constraints) {
        static_assert(std::is_same<internal::setter_field_type_t<S>, internal::getter_field_type_t<G>>::value,
                      "Getter and setter must get and set same data type");
        internal::validate_column_definition<G, Op...>();

        // attention: do not use `std::make_tuple()` for constructing the tuple member `[[no_unique_address]] column_constraints::constraints`,
        // as this will lead to UB with Clang on MinGW!
        return {std::move(name), getter, setter, std::tuple<Op...>{std::move(constraints)...}};
    }

    /**
     *  Factory function for a column definition from "getter" and "setter" member function pointers of the object to be mapped.
     */
    template<class G,
             class S,
             class... Op,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true>
    internal::column_t<G, S, Op...> make_column(std::string name, G getter, S setter, Op... constraints) {
        static_assert(std::is_same<internal::setter_field_type_t<S>, internal::getter_field_type_t<G>>::value,
                      "Getter and setter must get and set same data type");
        internal::validate_column_definition<G, Op...>();

        // attention: do not use `std::make_tuple()` for constructing the tuple member `[[no_unique_address]] column_constraints::constraints`,
        // as this will lead to UB with Clang on MinGW!
        return {std::move(name), getter, setter, std::tuple<Op...>{std::move(constraints)...}};
    }
}
