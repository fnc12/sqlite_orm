#pragma once

/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializer_context -> db_objects_tuple -> base_table -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  //  std::make_unique
#endif

#include "../tuple_helper/tuple_traits.h"
#include "../default_value_extractor.h"
#include "../schema/column.h"

namespace sqlite_orm::internal {
    template<class... Op>
    std::unique_ptr<std::string> column_constraints<Op...>::default_value() const {
        static constexpr size_t default_op_index = find_tuple_element<constraints_type, is_default>::value;

        std::unique_ptr<std::string> value;
        if constexpr (default_op_index != std::tuple_size<constraints_type>::value) {
            value =
                std::make_unique<std::string>(serialize_default_value(std::get<default_op_index>(this->constraints)));
        }
        return value;
    }
}
