#pragma once

#include <sqlite3.h>

#include "object_from_column_builder.h"

namespace sqlite_orm {

    namespace internal {

        /**
         * This is a private row extractor class. It is used for extracting rows as objects instead of tuple.
         * Main difference from regular `row_extractor` is that this class takes table info which is required
         * for constructing objects by member pointers. To construct please use `row_extractor_builder` class
         * Type arguments:
         * V is value type just like regular `row_extractor` has
         * T is table info class `table_t`
         */
        template<class V, class T>
        struct mapped_row_extractor {
            using table_info_t = T;

            mapped_row_extractor(const table_info_t& tableInfo_) : tableInfo(tableInfo_) {}

            V extract(sqlite3_stmt* stmt, int /*columnIndex*/) {
                V res;
                object_from_column_builder<V> builder{res, stmt};
                this->tableInfo.for_each_column(builder);
                return res;
            }

            const table_info_t& tableInfo;
        };

    }

}
