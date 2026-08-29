#pragma once

/** @file Operand/arithmetic-participation traits used to constrain operator overloads.
 *  
 *        Definitions and specializations of these traits are in the corresponding header files of the grammar nodes.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::bool_constant
#endif

namespace sqlite_orm::internal {
    /**
     *  Types participating as a negatable argument to overloaded operators
     */
    template<class T>
    extern const bool is_negatable_operand_v;

    template<class T>
    using is_negatable_operand = std::bool_constant<is_negatable_operand_v<T>>;

    /**
     *  Types participating as an arithmetic argument to overloaded operators
     */
    template<class T>
    extern const bool is_arithmetic_operand_v;

    template<class T>
    using is_arithmetic_operand = std::bool_constant<is_arithmetic_operand_v<T>>;

    /**
     *  Types participating as a conditional argument to overloaded operators
     */
    template<class T>
    extern const bool is_conditional_operand_v;

    template<class T>
    using is_conditional_operand = std::bool_constant<is_conditional_operand_v<T>>;

    /**
     *  Types participating as a chainable argument to overloaded operators
     */
    template<class T>
    constexpr bool is_chainable_operand_v = false;

    template<class T>
    using is_chainable_operand = std::bool_constant<is_chainable_operand_v<T>>;

    /**
     *  Other types not already classified as arithmetic, conditional, negation or chaining operators but participating as an argument to overloaded operators.
     */
    template<class T, class SFINAE = void>
    constexpr bool is_operator_argument_v = false;

    template<class T>
    using is_operator_argument = std::bool_constant<is_operator_argument_v<T>>;
}
