#pragma once

/** @file
 *  Purpose: tag a node with what it can be an operand of, so that the operators and conditions of the DSL take it.
 *
 *  Intention: a helper of the node headers in `ast/`, placed by `docs/internals/source-layout.md`; its tags specialize
 *  the operand traits of the vocabulary layer, as `docs/internals/vocabulary-layer.md` holds, for the nodes deriving
 *  from them.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of, std::enable_if_t
#endif

#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

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
    constexpr bool is_conditional_operand_v<T, std::enable_if_t<std::is_base_of<condition_t, T>::value>> = true;
}
