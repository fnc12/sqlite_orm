/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializer_context -> storage_impl -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once
#include <type_traits>  //  std::decay_t
#include <utility>  //  std::move
#include <algorithm>  //  std::find_if

#include "../type_printer.h"
#include "../column.h"
#include "../table.h"

namespace sqlite_orm {
    namespace internal {

        template<class T, bool WithoutRowId, class... Cs>
        std::vector<table_xinfo> table_t<T, WithoutRowId, Cs...>::get_table_info() const {
            std::vector<table_xinfo> res;
            res.reserve(size_t(this->elements_count));
            this->for_each_column([&res](auto& col) {
                using field_type = field_type_t<std::decay_t<decltype(col)>>;
                std::string dft;
                if(auto d = col.default_value()) {
                    dft = move(*d);
                }
                res.emplace_back(-1,
                                 col.name,
                                 type_printer<field_type>().print(),
                                 col.not_null(),
                                 dft,
                                 col.template has<primary_key_t<>>(),
								col.is_generated());
            });
            auto compositeKeyColumnNames = this->composite_key_columns_names();
            for(size_t i = 0; i < compositeKeyColumnNames.size(); ++i) {
                auto& columnName = compositeKeyColumnNames[i];
                auto it = std::find_if(res.begin(), res.end(), [&columnName](const table_xinfo& ti) {
                    return ti.name == columnName;
                });
                if(it != res.end()) {
                    it->pk = static_cast<int>(i + 1);
                }
            }
            return res;
        }

    }
}
