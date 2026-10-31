#pragma once

/** @file
 *  Purpose: define the field predicates whose condition is a concept, beside their C++17 fallback.
 *
 *  Intention: the concepts of the field predicates in the vocabulary layer, whose rules
 *  `docs/internals/vocabulary-layer.md` holds; `node_algorithms.h` includes it, and consumers read the
 *  predicates through that umbrella.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_same
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  // std::same_as
#endif
#endif

#include "../../functional/field_of.h"
#include "../../functional/member_traits.h"
#include "../node_traits.h"

namespace sqlite_orm::internal {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /**
     *  Whether the field `F Hidden::*` names a hidden column of the virtual table `VTabStruct`.
     */
    template<class Hidden, class F, class VTabStruct>
    concept hidden_field_of_vtab =
        is_field_of_v<F Hidden::*, typename VTabStruct::hidden::template _of<typename Hidden::enclosing_type>>;

    /**
     *  Whether the column pointer `CP` names a hidden column of the virtual table `VTabStruct`.
     */
    template<class CP, class VTabStruct>
    concept hidden_column_of_vtab = std::same_as<member_object_type_t<field_type_t<CP>>, typename VTabStruct::hidden>;
#else
    /**
     *  Whether the column pointer `CP` names a hidden column of the virtual table `VTabStruct`.
     */
    template<class CP, class VTabStruct, class SFINAE = void>
    constexpr bool is_hidden_column_of_vtab_v = false;

    template<class CP, class VTabStruct>
    constexpr bool is_hidden_column_of_vtab_v<
        CP,
        VTabStruct,
        std::enable_if_t<std::is_same<member_object_type_t<field_type_t<CP>>, typename VTabStruct::hidden>::value>> =
        true;

    /**
     *  Whether the field `F Hidden::*` names a hidden column of the virtual table `VTabStruct`.
     */
    template<class Hidden, class F, class VTabStruct>
    constexpr bool is_hidden_field_of_vtab_v =
        is_field_of_v<F Hidden::*, typename VTabStruct::hidden::template _of<typename Hidden::enclosing_type>>;
#endif
}
