#pragma once

#include <sqlite3.h>
#include <memory>  //  std::shared_ptr, std::unique_ptr, std::make_shared
#include <type_traits>  //  std::decay
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

        template<class V>
        struct iterator_t {
            using view_type = V;
            using value_type = typename view_type::mapped_type;

          protected:
            /**
             *  shared_ptr is used over unique_ptr here
             *  so that the iterator can be copyable.
             */
            std::shared_ptr<sqlite3_stmt> stmt;

            // only null for the default constructed iterator
            view_type* view = nullptr;

            /**
             *  shared_ptr is used over unique_ptr here
             *  so that the iterator can be copyable.
             */
            std::shared_ptr<value_type> current;

            void extract_value() {
                auto& dbObjects = obtain_db_objects(this->view->storage);
                this->current = std::make_shared<value_type>();
                object_from_column_builder<value_type> builder{*this->current, this->stmt.get()};
                pick_table<value_type>(dbObjects).for_each_column(builder);
            }

            void next() {
                this->current.reset();
                if(sqlite3_stmt* stmt = this->stmt.get()) {
                    perform_step(stmt, std::bind(&iterator_t::extract_value, this));
                    if(!this->current) {
                        this->stmt.reset();
                    }
                }
            }

          public:
            using difference_type = ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = std::input_iterator_tag;

            iterator_t(){};

            iterator_t(statement_finalizer stmt_, view_type& view_) : stmt{move(stmt_)}, view{&view_} {
                next();
            }

            const value_type& operator*() const {
                if(!this->stmt || !this->current) {
                    throw std::system_error{orm_error_code::trying_to_dereference_null_iterator};
                }
                return *this->current;
            }

            const value_type* operator->() const {
                return &(this->operator*());
            }

            iterator_t<V>& operator++() {
                next();
                return *this;
            }

            void operator++(int) {
                this->operator++();
            }

            bool operator==(const iterator_t& other) const {
                return this->current == other.current;
            }

            bool operator!=(const iterator_t& other) const {
                return !(*this == other);
            }
        };
    }
}
