#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of
#endif

#include "vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct negatable_t {};

    template<class T>
    constexpr bool is_negatable_operand_v = std::is_base_of<negatable_t, T>::value;

    /**
     *  Inherit from this class to support arithmetic types overloading
     */
    struct arithmetic_t {};

    template<class T>
    constexpr bool is_arithmetic_operand_v = std::is_base_of<arithmetic_t, T>::value;

    /**
     *  Inherit from this class if target class can be chained with other conditions with '&&' and '||' operators
     */
    struct condition_t {};

    template<class T>
    constexpr bool is_conditional_operand_v = std::is_base_of<condition_t, T>::value;
}
