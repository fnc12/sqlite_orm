#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::is_member_object_pointer

#include "functional/static_magic.h"
#include "type_traits.h"
#include "table_reference.h"
#include "row_extractor.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        struct object_from_column_builder_base {
            sqlite3_stmt* stmt = nullptr;
            int columnIndex = -1;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            object_from_column_builder_base(sqlite3_stmt* stmt, int columnIndex = -1) :
                stmt{stmt}, columnIndex{columnIndex} {}
#endif
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
                static_if<std::is_member_object_pointer<G>::value>(
                    [&value, &object = this->object](const auto& column) {
                        object.*column.member_pointer = std::move(value);
                    },
                    [&value, &object = this->object](const auto& column) {
                        (object.*column.setter)(std::move(value));
                    })(column);
            }
        };

        /**
         *  Specialization for a table reference.
         *  
         *  This plays together with `column_result_of_t`, which returns `object_t<O>` as `table_referenece<O>`
         */
        template<class O, class DBOs>
        struct struct_extractor<table_reference<O>, DBOs> {
            const DBOs& db_objects;

            O extract(const char* columnText) const = delete;

            O extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/) const {
                int columnIndex = 0;
                return this->extract(stmt, columnIndex);
            }

            O extract(sqlite3_stmt* stmt, int& columnIndex) const {
                O obj;
                object_from_column_builder<O> builder{obj, stmt, columnIndex};
                auto& table = pick_table<O>(this->db_objects);
                table.for_each_column(builder);
                columnIndex = builder.columnIndex;
                return obj;
            }

            O extract(sqlite3_value* value) const = delete;
        };
    }
}
