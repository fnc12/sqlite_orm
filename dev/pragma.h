#pragma once

#include <string>   //  std::string
#include <sqlite3.h>
#include <functional>   //  std::function
#include <memory> // std::shared_ptr

#include "error_code.h"
#include "row_extractor.h"
#include "journal_mode.h"

namespace sqlite_orm {
    
    namespace internal {
        struct database_connection;
        struct storage_base;
    }
    
    struct pragma_t {
        using get_or_create_connection_t = std::function<std::shared_ptr<internal::database_connection>()>;
        
        pragma_t(get_or_create_connection_t get_or_create_connection_):
        get_or_create_connection(std::move(get_or_create_connection_))
        {}
        
        sqlite_orm::journal_mode journal_mode() {
            return this->get_pragma<sqlite_orm::journal_mode>("journal_mode");
        }
        
        void journal_mode(sqlite_orm::journal_mode value) {
            this->_journal_mode = -1;
            this->set_pragma("journal_mode", value);
            this->_journal_mode = static_cast<decltype(this->_journal_mode)>(value);
        }
        
        int synchronous() {
            return this->get_pragma<int>("synchronous");
        }
        
        void synchronous(int value) {
            this->_synchronous = -1;
            this->set_pragma("synchronous", value);
            this->_synchronous = value;
        }
        
        int user_version() {
            return this->get_pragma<int>("user_version");
        }
        
        void user_version(int value) {
            this->set_pragma("user_version", value);
        }
        
        int auto_vacuum() {
            return this->get_pragma<int>("auto_vacuum");
        }
        
        void auto_vacuum(int value) {
            this->set_pragma("auto_vacuum", value);
        }
        
    protected:
        friend struct storage_base;
        
    public:
        int _synchronous = -1;
        signed char _journal_mode = -1; //  if != -1 stores static_cast<sqlite_orm::journal_mode>(journal_mode)
        get_or_create_connection_t get_or_create_connection;
        
        template<class T>
        T get_pragma(const std::string &name) {
            auto connection = this->get_or_create_connection();
            auto query = "PRAGMA " + name;
            T res;
            auto db = connection->get_db();
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv, char **) -> int {
                                       auto &res = *(T*)data;
                                       if(argc){
                                           res = row_extractor<T>().extract(argv[0]);
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc == SQLITE_OK){
                return res;
            }else{
                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
            }
        }
        
        /**
         *  Yevgeniy Zakharov: I wanted to refactored this function with statements and value bindings
         *  but it turns out that bindings in pragma statements are not supported.
         */
        template<class T>
        void set_pragma(const std::string &name, const T &value, sqlite3 *db = nullptr) {
            std::shared_ptr<internal::database_connection> connection;
            if(!db){
                connection = this->get_or_create_connection();
                db = connection->get_db();
            }
            std::stringstream ss;
            ss << "PRAGMA " << name << " = " << value;
            auto query = ss.str();
            auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
            }
        }
        
        void set_pragma(const std::string &name, const sqlite_orm::journal_mode &value, sqlite3 *db = nullptr) {
            std::shared_ptr<internal::database_connection> connection;
            if(!db){
                connection = this->get_or_create_connection();
                db = connection->get_db();
            }
            std::stringstream ss;
            ss << "PRAGMA " << name << " = " << internal::to_string(value);
            auto query = ss.str();
            auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
            }
        }
    };
}
