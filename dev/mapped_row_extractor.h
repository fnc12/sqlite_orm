#pragma once

#include <sqlite3.h>

#include "object_from_column_builder.h"

namespace sqlite_orm {

    namespace internal {

        /**
         * This is a private row extractor class. It is used for extracting rows as objects instead of tuple.
         * Main difference from regular `row_extractor` is that this class takes table info which is required
         * for constructing objects by member pointers. To construct please use `make_row_extractor()`.
         * Type arguments:
         * V is value type just like regular `row_extractor` has
         * T is table info class `table_t`
         */
        template<class V, class Table>
        struct mapped_row_extractor {
            using table_type = Table;

            V extract(sqlite3_stmt* stmt, int /*columnIndex*/) const {
                V res;
                object_from_column_builder<V> builder{res, stmt};
                this->tableInfo.for_each_column(builder);
                return res;
            }

            const table_type& tableInfo;
        };

    }

}
