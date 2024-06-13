#pragma once

#include <sqlite3.h>
#include <memory>  //  std::shared_ptr, std::make_shared
#include <utility>  //  std::move
#include <iterator>  //  std::input_iterator_tag
#include <system_error>  //  std::system_error
#include <functional>  //  std::bind

#include "functional/cxx_universal.h"  //  ::ptrdiff_t
#include "statement_finalizer.h"
#include "error_code.h"
#include "object_from_column_builder.h"
#include "storage_lookup.h"
#include "util.h"

namespace sqlite_orm {
    namespace internal {

        /*  
         *  (Legacy) Input iterator over a result set for a mapped object.
         */
        template<class O, class DBOs>
        class mapped_iterator {
          public:
            using db_objects_type = DBOs;

            using iterator_category = std::input_iterator_tag;
            using difference_type = ptrdiff_t;
            using value_type = O;
            using reference = O&;
            using pointer = O*;

          private:
            /**
                pointer to the db objects.
                only null for the default constructed iterator.
             */
            const db_objects_type* db_objects = nullptr;

            /**
             *  shared_ptr is used over unique_ptr here
             *  so that the iterator can be copyable.
             */
            std::shared_ptr<sqlite3_stmt> stmt;

            /**
             *  shared_ptr is used over unique_ptr here
             *  so that the iterator can be copyable.
             */
            std::shared_ptr<value_type> current;

            void extract_object() {
                this->current = std::make_shared<value_type>();
                object_from_column_builder<value_type> builder{*this->current, this->stmt.get()};
                auto& table = pick_table<value_type>(*this->db_objects);
                table.for_each_column(builder);
            }

            void step() {
                perform_step(this->stmt.get(), std::bind(&mapped_iterator::extract_object, this));
                if(!this->current) {
                    this->stmt.reset();
                }
            }

            void next() {
                this->current.reset();
                this->step();
            }

          public:
            mapped_iterator() = default;

            mapped_iterator(const db_objects_type& dbObjects, statement_finalizer stmt) :
                db_objects{&dbObjects}, stmt{std::move(stmt)} {
                this->step();
            }

            mapped_iterator(const mapped_iterator&) = default;
            mapped_iterator& operator=(const mapped_iterator&) = default;
            mapped_iterator(mapped_iterator&&) = default;
            mapped_iterator& operator=(mapped_iterator&&) = default;

            value_type& operator*() const {
                if(!this->stmt)
                    SQLITE_ORM_CPP_UNLIKELY {
                        throw std::system_error{orm_error_code::trying_to_dereference_null_iterator};
                    }
                return *this->current;
            }

            // note: should actually be only present for contiguous iterators
            value_type* operator->() const {
                return &(this->operator*());
            }

            mapped_iterator& operator++() {
                next();
                return *this;
            }

            mapped_iterator operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            friend bool operator==(const mapped_iterator& lhs, const mapped_iterator& rhs) {
                return lhs.current == rhs.current;
            }

#ifndef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
            friend bool operator!=(const mapped_iterator& lhs, const mapped_iterator& rhs) {
                return !(lhs == rhs);
            }
#endif
        };
    }
}
