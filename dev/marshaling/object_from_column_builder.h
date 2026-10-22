#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_member_object_pointer
#include <utility>  //  std::move
#endif

#include "../functional/type_traits.h"
#include "../functional/member_traits.h"
#include "../vocabulary/node_fwd.h"  // column_field
#include "row_extractor.h"

namespace sqlite_orm::internal {
    struct object_from_column_builder_base {
        sqlite3_stmt* stmt = nullptr;
        int columnIndex = -1;
    };

    /**
     *  Function object for building an object from a result row.
     */
    template<class O>
    struct object_from_column_builder : object_from_column_builder_base {
        using object_type = O;

        object_type& object;

        object_from_column_builder(object_type& object_, sqlite3_stmt* stmt_, int nextColumnIndex = 0) :
            object_from_column_builder_base{stmt_, nextColumnIndex - 1}, object(object_) {}

        template<class G, class S>
        void operator()(const column_field<G, S>& column) {
            const auto rowExtractor = row_value_extractor<member_field_type_t<G>>();
            auto value = rowExtractor.extract(this->stmt, ++this->columnIndex);
            if constexpr (std::is_member_object_pointer<G>::value) {
                object.*column.member_pointer = std::move(value);
            } else {
                (object.*column.setter)(std::move(value));
            };
        }
    };
}
