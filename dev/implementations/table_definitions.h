/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializer_context -> db_objects_tuple -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once

#ifndef _IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_reference
#include <utility>  //  std::move
#include <algorithm>  //  std::find_if, std::ranges::find
#endif

#include "../tuple_helper/tuple_filter.h"
#include "../type_traits.h"
#include "../type_printer.h"
#include "../schema/column.h"
#include "../schema/table.h"

namespace sqlite_orm {
    namespace internal {

        template<class T, bool WithoutRowId, class... Cs>
        std::vector<table_xinfo> table_t<T, WithoutRowId, Cs...>::get_table_info() const {
            std::vector<table_xinfo> res;
            res.reserve(filter_tuple_sequence_t<elements_type, is_column>::size());
            this->for_each_column([&res](auto& column) {
                using field_type = field_type_t<std::remove_reference_t<decltype(column)>>;
                std::string dft;
                if (auto d = column.default_value()) {
                    dft = std::move(*d);
                }
                using constraints_tuple = decltype(column.constraints);
                constexpr bool hasExplicitNull =
                    mpl::invoke_t<mpl::disjunction<check_if_has_type<null_t>>, constraints_tuple>::value;
                constexpr bool hasExplicitNotNull =
                    mpl::invoke_t<mpl::disjunction<check_if_has_type<not_null_t>>, constraints_tuple>::value;
                res.emplace_back(-1,
                                 column.name,
                                 type_printer<field_type>().print(),
                                 !hasExplicitNull && !hasExplicitNotNull
                                     ? column.is_not_null()
                                     : (hasExplicitNull ? !hasExplicitNull : hasExplicitNotNull),
                                 std::move(dft),
                                 column.template is<is_primary_key>(),
                                 column.template is<is_generated_always>());
            });
            auto compositeKeyColumnNames = this->composite_key_columns_names();
#if defined(SQLITE_ORM_INIT_RANGE_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_CPP20_RANGES_SUPPORTED)
            for (int n = 1; const std::string& columnName: compositeKeyColumnNames) {
                if (auto it = std::ranges::find(res, columnName, &table_xinfo::name); it != res.end()) {
                    it->pk = n;
                }
                ++n;
            }
#else
            for (size_t i = 0; i < compositeKeyColumnNames.size(); ++i) {
                const std::string& columnName = compositeKeyColumnNames[i];
                auto it = std::find_if(res.begin(), res.end(), [&columnName](const table_xinfo& ti) {
                    return ti.name == columnName;
                });
                if (it != res.end()) {
                    it->pk = static_cast<int>(i + 1);
                }
            }
#endif
            return res;
        }

    }
}
