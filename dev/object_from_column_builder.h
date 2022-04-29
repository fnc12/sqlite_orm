#pragma once

#include <sqlite3.h>

#include <type_traits>  //  std::is_member_object_pointer
#include "static_magic.h"
#include "row_extractor.h"

namespace sqlite_orm {

    namespace internal {

        struct object_from_column_builder_base {
            sqlite3_stmt* stmt = nullptr;
            mutable int index = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            object_from_column_builder_base(sqlite3_stmt* stmt) : stmt{stmt} {}
#endif
        };

        /**
         * This is a cute lambda replacement which is used in several places.
         */
        template<class O>
        struct object_from_column_builder : object_from_column_builder_base {
            using object_type = O;

            object_type& object;

            object_from_column_builder(object_type& object_, sqlite3_stmt* stmt_) :
                object_from_column_builder_base{stmt_}, object(object_) {}

            template<class C>
            void operator()(const C& c) const {
                using field_type = typename C::field_type;
                auto value = row_extractor<field_type>().extract(this->stmt, this->index++);
                static_if<std::is_member_object_pointer<typename C::member_pointer_t>::value>(
                    [&value, &object = this->object](const auto& c) {
                        object.*c.member_pointer = std::move(value);
                    },
                    [&value, &object = this->object](const auto& c) {
                        (object.*c.setter)(std::move(value));
                    })(c);
            }
        };
    }
}
