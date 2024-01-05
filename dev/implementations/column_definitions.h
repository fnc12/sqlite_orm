/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializer_context -> db_objects_tuple -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once

#include <memory>  //  std::make_unique

#include "../functional/static_magic.h"
#include "../tuple_helper/tuple_traits.h"
#include "../default_value_extractor.h"
#include "../schema/column.h"

namespace sqlite_orm {
    namespace internal {

        template<class... Op>
        std::unique_ptr<std::string> column_constraints<Op...>::default_value() const {
            static constexpr size_t default_op_index = find_tuple_template<constraints_type, default_t>::value;

            std::unique_ptr<std::string> value;
            call_if_constexpr<default_op_index != std::tuple_size<constraints_type>::value>(
                [&value](auto& constraints) {
                    value =
                        std::make_unique<std::string>(serialize_default_value(std::get<default_op_index>(constraints)));
                },
                this->constraints);
            return value;
        }

    }
}
