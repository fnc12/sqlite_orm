/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializator_context -> storage_impl -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once

#include "../default_value_extractor.h"
#include "../column.h"

namespace sqlite_orm {
    namespace internal {

        template<class O, class T, class G, class S, class... Op>
        std::unique_ptr<std::string> column_t<O, T, G, S, Op...>::default_value() const {
            std::unique_ptr<std::string> res;
            iterate_tuple(this->constraints, [&res](auto& v) {
                if(auto dft = default_value_extractor{}(v)) {
                    res = move(dft);
                }
            });
            return res;
        }

    }
}
