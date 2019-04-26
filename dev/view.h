#pragma once

#include <memory>   //  std::shared_ptr, std::make_shared, std::unique_ptr, std::make_unique
#include <string>   //  std::string
#include <utility>  //  std::forward, std::move
#include <sqlite3.h>
#include <type_traits>  //  std::decay
#include <cstddef>  //  std::ptrdiff_t
#include <ios>  //  std::make_error_code
#include <system_error> //  std::system_error

#include "database_connection.h"
#include "row_extractor.h"
#include "statement_finalizer.h"
#include "error_code.h"

namespace sqlite_orm {
    
    namespace internal {
        
        template<class T, class S, class ...Args>
        struct view_t {
            using mapped_type = T;
            using storage_type = S;
            
            storage_type &storage;
            std::shared_ptr<internal::database_connection> connection;
            
            const std::string query;
            
            view_t(storage_type &stor, decltype(connection) conn, Args&& ...args):
            storage(stor),
            connection(std::move(conn)),
            query([&args..., &stor]{
                std::string q;
                stor.template generate_select_asterisk<T>(&q, std::forward<Args>(args)...);
                return q;
            }()){}
            
            struct iterator_t {
            protected:
                /**
                 *  The double-indirection is so that copies of the iterator
                 *  share the same sqlite3_stmt from a sqlite3_prepare_v2()
                 *  call. When one finishes iterating it, the pointer
                 *  inside the shared_ptr is nulled out in all copies.
                 */
                std::shared_ptr<sqlite3_stmt *> stmt;
                view_t<T, S, Args...> &view;
                
                /**
                 *  shared_ptr is used over unique_ptr here
                 *  so that the iterator can be copyable.
                 */
                std::shared_ptr<T> current;
                
                void extract_value(std::unique_ptr<T> &temp) {
                    temp = std::make_unique<T>();
                    auto &storage = this->view.storage;
                    auto &impl = storage.template get_impl<T>();
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
                using value_type = T;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type *;
                using reference = value_type &;
                using iterator_category = std::input_iterator_tag;
                
                iterator_t(sqlite3_stmt *stmt_, view_t<T, S, Args...> &view_): stmt(std::make_shared<sqlite3_stmt *>(stmt_)), view(view_) {
                    this->operator++();
                }
                
                iterator_t(const iterator_t &) = default;
                
                iterator_t(iterator_t&&) = default;
                
                iterator_t& operator=(iterator_t&&) = default;
                
                iterator_t& operator=(const iterator_t&) = default;
                
                ~iterator_t() {
                    if(this->stmt){
                        statement_finalizer f{*this->stmt};
                    }
                }
                
                T& operator*() {
                    if(!this->stmt) {
                        throw std::system_error(std::make_error_code(orm_error_code::trying_to_dereference_null_iterator));
                    }
                    if(!this->current){
                        std::unique_ptr<T> value;
                        this->extract_value(value);
                        this->current = std::move(value);
                    }
                    return *this->current;
                }
                
                T* operator->() {
                    if(!this->stmt) {
                        throw std::system_error(std::make_error_code(orm_error_code::trying_to_dereference_null_iterator));
                    }
                    if(!this->current){
                        std::unique_ptr<T> value;
                        this->extract_value(value);
                        this->current = std::move(value);
                    }
                    return &*this->current;
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
                                throw std::system_error(std::error_code(sqlite3_errcode(this->view.connection->get_db()), get_sqlite_error_category()));
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
            
            size_t size() {
                return this->storage.template count<T>();
            }
            
            bool empty() {
                return !this->size();
            }
            
            iterator_t end() {
                return {nullptr, *this};
            }
            
            iterator_t begin() {
                sqlite3_stmt *stmt = nullptr;
                auto db = this->connection->get_db();
                auto ret = sqlite3_prepare_v2(db, this->query.c_str(), -1, &stmt, nullptr);
                if(ret == SQLITE_OK){
                    return {stmt, *this};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()));
                }
            }
        };
    }
}
