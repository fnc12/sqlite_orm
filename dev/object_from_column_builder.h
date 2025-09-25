#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_member_object_pointer
#include <utility>  //  std::move
#if defined(SQLITE_ORM_WITH_VIEW) && (BOOST_PFR_ENABLED == 1)
#include <cstddef>  //  std::byte
#endif
#endif

#include "functional/gsl.h"
#include "member_traits/member_traits.h"
#include "type_traits.h"
#include "table_reference.h"
#include "row_extractor.h"
#include "schema/column.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

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

#if defined(SQLITE_ORM_WITH_VIEW) && (BOOST_PFR_ENABLED == 1)
            template<class C>
                requires (is_column_pointer_v<C>)
            void operator()(const column_field<C, empty_setter>& column) {
                using field_type = field_type_t<column_field<C, empty_setter>>;
                const auto rowExtractor = row_value_extractor<field_type>();
                auto value = rowExtractor.extract(this->stmt, ++this->columnIndex);
                // calculate absolute address of member from relative address
                const std::byte* fieldAddress = (std::byte*)(uintptr_t(&object) + size_t(column.member_pointer.field));
                field_type* field = (field_type*)fieldAddress;
                *field = std::move(value);
            }
#endif
        };

        /**
         *  Specialization for a table reference.
         *  
         *  This plays together with `column_result_of_t`, which returns `object_t<O>` as `table_reference<O>`
         */
        template<class O, class DBOs>
        struct struct_extractor<table_reference<O>, DBOs> {
            const DBOs& db_objects;

            O extract(orm_gsl::czstring columnText) const = delete;

            // note: expects to be called only from the top level, and therefore discards the index
            O extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/ = 0) const {
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
