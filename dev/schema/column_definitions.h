#pragma once

/** @file
 *  Purpose: define the members of a column whose bodies need the statement serializer.
 *
 *  Intention: keeps `schema/column.h` free of the serializer; gathered by `interface_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <optional>  //  std::optional
#endif

#include "../functional/mpl/convenience.h"
#include "../serialization/default_value_extractor.h"
#include "../vocabulary/node_traits.h"
#include "column.h"

namespace sqlite_orm::internal {
    template<class... Op>
    std::optional<std::string> column_constraints<Op...>::default_value() const {
        static constexpr size_t default_op_index = find_tuple_element<constraints_type, is_default>::value;

        if constexpr (default_op_index != std::tuple_size<constraints_type>::value) {
            return serialize_default_value(std::get<default_op_index>(this->constraints));
        } else {
            return std::nullopt;
        }
    }
}
