#pragma once

#include <memory>   //  std::shared_ptr, std::unique_ptr, std::make_shared
#include <sqlite3.h>
#include <type_traits>  //  std::decay
#include <utility>  //  std::move
#include <cstddef>  //  std::ptrdiff_t
#include <iterator> //  std::input_iterator_tag
#include <system_error> //  std::system_error
#include <ios>  //  std::make_error_code

#include "row_extractor.h"
#include "statement_finalizer.h"
#include "error_code.h"

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
            std::shared_ptr<sqlite3_stmt *> stmt;
            view_type &view;
            
            /**
             *  shared_ptr is used over unique_ptr here
             *  so that the iterator can be copyable.
             */
            std::shared_ptr<value_type> current;
            
            void extract_value(std::unique_ptr<value_type> &temp) {
                temp = std::make_unique<value_type>();
                auto &storage = this->view.storage;
                auto &impl = storage.template get_impl<value_type>();
                auto index = 0;
                impl.table.for_each_column([&index, &temp, this] (auto &c) {
                    using field_type = typename std::decay<decltype(c)>::type::field_type;
                    auto value = row_extractor<field_type>().extract(*this->stmt, index++);
                    if(c.member_pointer){
                        auto member_pointer = c.member_pointer;
                        (*temp).*member_pointer = std::move(value);
                    }else{
                        ((*temp).*(c.setter))(std::move(value));
                    }
                });
            }
            
        public:
            using difference_type = std::ptrdiff_t;
            using pointer = value_type *;
            using reference = value_type &;
            using iterator_category = std::input_iterator_tag;
            
            iterator_t(sqlite3_stmt *stmt_, view_type &view_): stmt(std::make_shared<sqlite3_stmt *>(stmt_)), view(view_) {
                this->operator++();
            }
            
            iterator_t(const iterator_t &) = default;
            
            iterator_t(iterator_t&&) = default;
            
            iterator_t& operator=(iterator_t &&) = default;
            
            iterator_t& operator=(const iterator_t &) = default;
            
            ~iterator_t() {
                if(this->stmt){
                    statement_finalizer f{*this->stmt};
                }
            }
            
            value_type &operator*() {
                if(!this->stmt) {
                    throw std::system_error(std::make_error_code(orm_error_code::trying_to_dereference_null_iterator));
                }
                if(!this->current){
                    std::unique_ptr<value_type> value;
                    this->extract_value(value);
                    this->current = move(value);
                }
                return *this->current;
            }
            
            value_type *operator->() {
                return &(this->operator*());
            }
            
            void operator++() {
                if(this->stmt && *this->stmt){
                    auto ret = sqlite3_step(*this->stmt);
                    switch(ret){
                        case SQLITE_ROW:
                            this->current = nullptr;
                            break;
                        case SQLITE_DONE:{
                            statement_finalizer f{*this->stmt};
                            *this->stmt = nullptr;
                        }break;
                        default:{
                            auto db = this->view.connection->get_db();
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                        }
                    }
                }
            }
            
            void operator++(int) {
                this->operator++();
            }
            
            bool operator==(const iterator_t &other) const {
                if(this->stmt && other.stmt){
                    return *this->stmt == *other.stmt;
                }else{
                    if(!this->stmt && !other.stmt){
                        return true;
                    }else{
                        return false;
                    }
                }
            }
            
            bool operator!=(const iterator_t &other) const {
                return !(*this == other);
            }
        };
    }
}
