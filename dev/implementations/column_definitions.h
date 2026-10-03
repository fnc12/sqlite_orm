#pragma once

/** @file Out-of-class definitions of column members, which need the statement serializer's machinery
 *  (`default_value_extractor.h`) that `schema/column.h` stays free of.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <optional>  //  std::optional
#endif

#include "../tuple_helper/tuple_traits.h"
#include "../vocabulary/node_traits.h"
#include "../default_value_extractor.h"
#include "../schema/column.h"

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
