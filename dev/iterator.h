#pragma once

#include <sqlite3.h>
#include <cassert>  //  assert
#include <memory>  //  std::shared_ptr, std::make_shared
#include <utility>  //  std::move
#include <iterator>  //  std::input_iterator_tag
#include <system_error>  //  std::system_error
#include <functional>  //  std::bind

#include "functional/cxx_universal.h"
#include "statement_finalizer.h"
#include "error_code.h"
#include "object_from_column_builder.h"
#include "storage_lookup.h"
#include "util.h"

namespace sqlite_orm {
    namespace internal {

        template<class O, class DBOs>
        struct iterator_t {
            using value_type = O;
            using db_objects_type = DBOs;

          private:
            /**
                pointer to the view's db objects member variable.
                only null for the default constructed iterator.
             */
            const db_objects_type** db_objects = nullptr;

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
                assert(*this->db_objects);
                pick_table<value_type>(**this->db_objects).for_each_column(builder);
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
            using difference_type = ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = std::input_iterator_tag;

            iterator_t() = default;

            iterator_t(const db_objects_type*& dbObjects, statement_finalizer stmt) :
                db_objects{&dbObjects}, stmt{std::move(stmt)} {
                this->step();
            }

            value_type& operator*() const {
                if(!this->stmt) {
                    throw std::system_error{orm_error_code::trying_to_dereference_null_iterator};
                }
                return *this->current;
            }

            value_type* operator->() const {
                return &(this->operator*());
            }

            iterator_t& operator++() {
                next();
                return *this;
            }

            iterator_t operator++(int) {
                auto tmp = *this;
                this->operator++();
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
