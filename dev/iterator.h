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
        class iterator_t {
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
                perform_step(this->stmt.get(), std::bind(&iterator_t::extract_object, this));
                if(!this->current) {
                    this->stmt.reset();
                }
            }

            void next() {
                this->current.reset();
                this->step();
            }

          public:
            iterator_t() = default;

            iterator_t(const db_objects_type& dbObjects, statement_finalizer stmt) :
                db_objects{&dbObjects}, stmt{std::move(stmt)} {
                this->step();
            }

            iterator_t(const iterator_t&) = default;
            iterator_t& operator=(const iterator_t&) = default;
            iterator_t(iterator_t&&) = default;
            iterator_t& operator=(iterator_t&&) = default;

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

            iterator_t& operator++() {
                next();
                return *this;
            }

            iterator_t operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            friend bool operator==(const iterator_t& lhs, const iterator_t& rhs) {
                return lhs.current == rhs.current;
            }

#ifndef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
            friend bool operator!=(const iterator_t& lhs, const iterator_t& rhs) {
                return !(lhs == rhs);
            }
#endif
        };
    }
}
