#pragma once

#include <memory>  //  std::shared_ptr, std::unique_ptr, std::make_shared
#include <sqlite3.h>
#include <type_traits>  //  std::decay
#include <utility>  //  std::move
#include <cstddef>  //  std::ptrdiff_t
#include <iterator>  //  std::input_iterator_tag
#include <system_error>  //  std::system_error
#include <ios>  //  std::make_error_code

#include "row_extractor.h"
#include "statement_finalizer.h"
#include "error_code.h"
#include "object_from_column_builder.h"

namespace sqlite_orm {

    namespace internal {

        template<class V>
        struct iterator_t {
            using view_type = V;
            using value_type = typename view_type::mapped_type;

          protected:
            /**
             *  The double-indirection is so that copies of the iterator
             *  share the same sqlite3_stmt from a sqlite3_prepare_v2()
             *  call. When one finishes iterating it the pointer
             *  inside the shared_ptr is nulled out in all copies.
             */
            std::shared_ptr<statement_finalizer> stmt;

            // only null for the default constructed iterator
            view_type* view;

            /**
             *  shared_ptr is used over unique_ptr here
             *  so that the iterator can be copyable.
             */
            std::shared_ptr<value_type> current;

            void extract_value() {
                auto& storage = this->view->storage;
                auto& impl = storage.template get_impl<value_type>();
                this->current = std::make_shared<value_type>();
                object_from_column_builder<value_type> builder{*this->current, this->stmt->get()};
                impl.table.for_each_column(builder);
            }

          public:
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = std::input_iterator_tag;

            iterator_t() : view(nullptr){};

            iterator_t(sqlite3_stmt* stmt_, view_type& view_) :
                stmt(std::make_shared<statement_finalizer>(stmt_)), view(&view_) {
                next();
            }

            const value_type& operator*() const {
                if(!this->stmt || !this->current) {
                    throw std::system_error(std::make_error_code(orm_error_code::trying_to_dereference_null_iterator));
                }
                return *this->current;
            }

            const value_type* operator->() const {
                return &(this->operator*());
            }

          private:
            void next() {
                this->current.reset();
                if(this->stmt) {
                    auto statementPointer = this->stmt->get();
                    auto ret = sqlite3_step(statementPointer);
                    switch(ret) {
                        case SQLITE_ROW:
                            this->extract_value();
                            break;
                        case SQLITE_DONE:
                            this->stmt.reset();
                            break;
                        default: {
                            auto db = this->view->connection.get();
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                    sqlite3_errmsg(db));
                        }
                    }
                }
            }

          public:
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
