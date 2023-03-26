/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializer_context -> db_objects_tuple -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once

#include <memory>  //  std::make_unique

#include "../functional/cxx_core_features.h"
#include "../functional/static_magic.h"
#include "../functional/index_sequence_util.h"
#include "../tuple_helper/tuple_filter.h"
#include "../tuple_helper/tuple_traits.h"
#include "../default_value_extractor.h"
#include "../column.h"

namespace sqlite_orm {
    namespace internal {

        template<class... Op>
        std::unique_ptr<std::string> column_constraints<Op...>::default_value() const {
            using default_op_index_sequence =
                filter_tuple_sequence_t<constraints_type, check_if_is_template<default_t>::template fn>;

            std::unique_ptr<std::string> value;
            call_if_constexpr<default_op_index_sequence::size()>(
                [&value](auto& constraints, auto op_index_sequence) {
                    using default_op_index_sequence = decltype(op_index_sequence);
                    constexpr size_t opIndex = first_index_sequence_value(default_op_index_sequence{});
                    value = std::make_unique<std::string>(serialize_default_value(get<opIndex>(constraints)));
                },
                this->constraints,
                default_op_index_sequence{});
            return value;
        }

    }
}
