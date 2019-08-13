#pragma once

#include <memory>   //  std::shared_ptr
#include <string>   //  std::string
#include <utility>  //  std::forward, std::move
#include <sqlite3.h>
#include <system_error> //  std::system_error
#include <tuple>    //  std::tuple, std::make_tuple

#include "database_connection.h"
#include "row_extractor.h"
#include "statement_finalizer.h"
#include "error_code.h"
#include "iterator.h"
#include "ast_iterator.h"

namespace sqlite_orm {
    
    namespace internal {
        
        template<class T, class S, class ...Args>
        struct view_t {
            using mapped_type = T;
            using storage_type = S;
            using self = view_t<T, S, Args...>;
            
            storage_type &storage;
            std::shared_ptr<internal::database_connection> connection;
            std::tuple<Args...> args;
            
            view_t(storage_type &stor, decltype(connection) conn, Args&& ...args_):
            storage(stor),
            connection(std::move(conn)),
            args(std::make_tuple(std::forward<Args>(args_)...)){}
            
            size_t size() {
                return this->storage.template count<T>();
            }
            
            bool empty() {
                return !this->size();
            }
            
            iterator_t<self> end() {
                return {nullptr, *this};
            }
            
            iterator_t<self> begin() {
                sqlite3_stmt *stmt = nullptr;
                auto db = this->connection->get_db();
                std::string query;
                this->storage.template generate_select_asterisk<T>(&query, this->args);
                auto ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
                if(ret == SQLITE_OK){
                    auto index = 1;
                    iterate_ast(this->args, [&index, stmt, db](auto &node){
                        using node_type = typename std::decay<decltype(node)>::type;
                        conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                        if(SQLITE_OK != binder(node)){
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                        }
                    });
                    return {stmt, *this};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
        };
    }
}
