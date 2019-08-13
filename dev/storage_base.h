#pragma once

#include <functional>   //  std::function, std::bind
#include <sqlite3.h>
#include <string>   //  std::string
#include <sstream>  //  std::stringstream
#include <utility>  //  std::move
#include <system_error> //  std::system_error, std::error_code, std::make_error_code
#include <vector>   //  std::vector
#include <memory>   //  std::make_shared, std::shared_ptr
#include <map>  //  std::map
#include <type_traits>  //  std::decay

#include "pragma.h"
#include "limit_accesor.h"
#include "transaction_guard.h"
#include "statement_finalizer.h"
#include "type_printer.h"
#include "tuple_helper.h"
#include "row_extractor.h"

namespace sqlite_orm {
    
    namespace internal {
        
        struct storage_base {
            using collating_function = std::function<int(int, const void*, int, const void*)>;
            
            std::function<void(sqlite3*)> on_open;
            pragma_t pragma;
            limit_accesor limit;
            
            transaction_guard_t transaction_guard() {
                this->begin_transaction();
                auto commitFunc = std::bind(static_cast<void(storage_base::*)()>(&storage_base::commit), this);
                auto rollbackFunc = std::bind(static_cast<void(storage_base::*)()>(&storage_base::rollback), this);
                return {move(commitFunc), move(rollbackFunc)};
            }
            
            void drop_index(const std::string &indexName) {
                auto connection = this->get_or_create_connection();
                std::stringstream ss;
                ss << "DROP INDEX '" << indexName + "'";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                auto db = connection->get_db();
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::system_error(std::error_code(sqlite3_errcode(connection->get_db()), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }else {
                    throw std::system_error(std::error_code(sqlite3_errcode(connection->get_db()), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            void vacuum() {
                auto connection = this->get_or_create_connection();
                std::string query = "VACUUM";
                sqlite3_stmt *stmt;
                auto db = connection->get_db();
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::system_error(std::error_code(sqlite3_errcode(connection->get_db()), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }else {
                    throw std::system_error(std::error_code(sqlite3_errcode(connection->get_db()), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            /**
             *  Drops table with given name.
             */
            void drop_table(const std::string &tableName) {
                auto connection = this->get_or_create_connection();
                this->drop_table_internal(tableName, connection->get_db());
            }
            
            /**
             *  sqlite3_changes function.
             */
            int changes() {
                auto connection = this->get_or_create_connection();
                return sqlite3_changes(connection->get_db());
            }
            
            /**
             *  sqlite3_total_changes function.
             */
            int total_changes() {
                auto connection = this->get_or_create_connection();
                return sqlite3_total_changes(connection->get_db());
            }
            
            int64 last_insert_rowid() {
                auto connection = this->get_or_create_connection();
                return sqlite3_last_insert_rowid(connection->get_db());
            }
            
            int busy_timeout(int ms) {
                auto connection = this->get_or_create_connection();
                return sqlite3_busy_timeout(connection->get_db(), ms);
            }
            
            /**
             *  Returns libsqltie3 lib version, not sqlite_orm
             */
            std::string libversion() {
                return sqlite3_libversion();
            }
            
            bool transaction(std::function<bool()> f) {
                this->begin_transaction();
                auto db = this->currentTransaction->get_db();
                auto shouldCommit = f();
                if(shouldCommit){
                    this->commit(db);
                }else{
                    this->rollback(db);
                }
                if(!this->inMemory && !this->isOpenedForever){
                    this->currentTransaction = nullptr;
                }
                return shouldCommit;
            }
            
            std::string current_timestamp() {
                auto connection = this->get_or_create_connection();
                return this->current_timestamp(connection->get_db());
            }
            
#if SQLITE_VERSION_NUMBER >= 3007010
            /**
             * \fn db_release_memory
             * \brief Releases freeable memory of database. It is function can/should be called periodically by application,
             * if application has less memory usage constraint.
             * \note sqlite3_db_release_memory added in 3.7.10 https://sqlite.org/changes.html
             */
            int db_release_memory() {
                auto connection = this->get_or_create_connection();
                return sqlite3_db_release_memory(connection->get_db());
            }
#endif
            
            /**
             *  Returns existing permanent table names in database. Doesn't check storage itself - works only with actual database.
             *  @return Returns list of tables in database.
             */
            std::vector<std::string> table_names() {
                auto connection = this->get_or_create_connection();
                std::vector<std::string> tableNames;
                std::string sql = "SELECT name FROM sqlite_master WHERE type='table'";
                using Data = std::vector<std::string>;
                auto db = connection->get_db();
                int res = sqlite3_exec(db, sql.c_str(),
                                       [] (void *data, int argc, char **argv, char ** /*columnName*/) -> int {
                                           auto& tableNames = *(Data*)data;
                                           for(int i = 0; i < argc; i++) {
                                               if(argv[i]){
                                                   tableNames.push_back(argv[i]);
                                               }
                                           }
                                           return 0;
                                       }, &tableNames,nullptr);
                
                if(res != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(connection->get_db()), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
                return tableNames;
            }
            
            void open_forever() {
                this->isOpenedForever = true;
                if(!this->currentTransaction){
                    this->currentTransaction = std::make_shared<internal::database_connection>(this->filename);
                    this->on_open_internal(this->currentTransaction->get_db());
                }
            }
            
            void create_collation(const std::string &name, collating_function f) {
                collating_function *functionPointer = nullptr;
                if(f){
                    functionPointer = &(collatingFunctions[name] = std::move(f));
                }else{
                    collatingFunctions.erase(name);
                }
                
                //  create collations if db is open
                if(this->currentTransaction){
                    auto db = this->currentTransaction->get_db();
                    if(sqlite3_create_collation(db,
                                                name.c_str(),
                                                SQLITE_UTF8,
                                                functionPointer,
                                                f ? collate_callback : nullptr) != SQLITE_OK)
                    {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }
            }
            
            void begin_transaction() {
                if(!this->inMemory){
                    if(!this->isOpenedForever){
                        if(this->currentTransaction) {
                            throw std::system_error(std::make_error_code(orm_error_code::cannot_start_a_transaction_within_a_transaction));
                        }
                        this->currentTransaction = std::make_shared<internal::database_connection>(this->filename);
                        this->on_open_internal(this->currentTransaction->get_db());
                    }
                }
                auto db = this->currentTransaction->get_db();
                this->begin_transaction(db);
            }
            
            void commit() {
                if(!this->inMemory){
                    if(!this->currentTransaction){
                        throw std::system_error(std::make_error_code(orm_error_code::no_active_transaction));
                    }
                }
                auto db = this->currentTransaction->get_db();
                this->commit(db);
                if(!this->inMemory && !this->isOpenedForever){
                    this->currentTransaction = nullptr;
                }
            }
            
            void rollback() {
                if(!this->inMemory){
                    if(!this->currentTransaction){
                        throw std::system_error(std::make_error_code(orm_error_code::no_active_transaction));
                    }
                }
                auto db = this->currentTransaction->get_db();
                this->rollback(db);
                if(!this->inMemory && !this->isOpenedForever){
                    this->currentTransaction = nullptr;
                }
            }
            
        protected:
            
            storage_base(const std::string &filename_, int foreignKeysCount):
            pragma(std::bind(&storage_base::get_or_create_connection, this)),
            limit(std::bind(&storage_base::get_or_create_connection, this)),
            filename(filename_),
            inMemory(filename.empty() || filename == ":memory:"),
            cachedForeignKeysCount(foreignKeysCount)
            {}
            
            storage_base(const storage_base &other):
            on_open(other.on_open),
            pragma(std::bind(&storage_base::get_or_create_connection, this)),
            limit(std::bind(&storage_base::get_or_create_connection, this)),
            filename(other.filename),
            inMemory(other.inMemory),
            cachedForeignKeysCount(other.cachedForeignKeysCount)
            {}
            
            std::string filename;
            const bool inMemory;
            bool isOpenedForever = false;
            std::shared_ptr<internal::database_connection> currentTransaction;
            std::map<std::string, collating_function> collatingFunctions;
            const int cachedForeignKeysCount;
            
#if SQLITE_VERSION_NUMBER >= 3006019
            
            void foreign_keys(sqlite3 *db, bool value) {
                std::stringstream ss;
                ss << "PRAGMA foreign_keys = " << value;
                auto query = ss.str();
                auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            bool foreign_keys(sqlite3 *db) {
                std::string query = "PRAGMA foreign_keys";
                auto res = false;
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **) -> int {
                                           auto &res = *(bool*)data;
                                           if(argc){
                                               res = row_extractor<bool>().extract(argv[0]);
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
                return res;
            }
            
#endif
            /**
             *  Check whether connection exists and returns it if yes or creates a new one
             *  and returns it.
             */
            std::shared_ptr<internal::database_connection> get_or_create_connection() {
                decltype(this->currentTransaction) connection;
                if(!this->currentTransaction){
                    connection = std::make_shared<internal::database_connection>(this->filename);
                    this->on_open_internal(connection->get_db());
                }else{
                    connection = this->currentTransaction;
                }
                return connection;
            }
            
            template<class O, class T, class G, class S, class ...Op>
            std::string serialize_column_schema(const internal::column_t<O, T, G, S, Op...> &c) {
                std::stringstream ss;
                ss << "'" << c.name << "' ";
                using column_type = typename std::decay<decltype(c)>::type;
                using field_type = typename column_type::field_type;
                using constraints_type = typename column_type::constraints_type;
                ss << type_printer<field_type>().print() << " ";
                tuple_helper::iterator<std::tuple_size<constraints_type>::value - 1, Op...>()(c.constraints, [&ss](auto &v){
                    ss << static_cast<std::string>(v) << ' ';
                });
                if(c.not_null()){
                    ss << "NOT NULL ";
                }
                return ss.str();
            }
            
            void on_open_internal(sqlite3 *db) {
                
#if SQLITE_VERSION_NUMBER >= 3006019
                if(this->cachedForeignKeysCount){
                    this->foreign_keys(db, true);
                }
#endif
                if(this->pragma._synchronous != -1) {
                    this->pragma.synchronous(this->pragma._synchronous);
                }
                
                if(this->pragma._journal_mode != -1) {
                    this->pragma.set_pragma("journal_mode", static_cast<journal_mode>(this->pragma._journal_mode), db);
                }
                
                for(auto &p : this->collatingFunctions){
                    if(sqlite3_create_collation(db,
                                                p.first.c_str(),
                                                SQLITE_UTF8,
                                                &p.second,
                                                collate_callback) != SQLITE_OK)
                    {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }
                
                for(auto &p : this->limit.limits) {
                    sqlite3_limit(db, p.first, p.second);
                }
                
                if(this->on_open){
                    this->on_open(db);
                }
            }
            
            void begin_transaction(sqlite3 *db) {
                std::stringstream ss;
                ss << "BEGIN TRANSACTION";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }else {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            void commit(sqlite3 *db) {
                std::stringstream ss;
                ss << "COMMIT";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }else {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            void rollback(sqlite3 *db) {
                std::stringstream ss;
                ss << "ROLLBACK";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }else {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            std::string current_timestamp(sqlite3 *db) {
                std::string res;
                std::stringstream ss;
                ss << "SELECT CURRENT_TIMESTAMP";
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv, char **) -> int {
                                           auto &res = *(std::string*)data;
                                           if(argc){
                                               if(argv[0]){
                                                   res = row_extractor<std::string>().extract(argv[0]);
                                               }
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
                return res;
            }
            
            void drop_table_internal(const std::string &tableName, sqlite3 *db) {
                std::stringstream ss;
                ss << "DROP TABLE '" << tableName + "'";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }else {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class S>
            std::string process_order_by(const conditions::dynamic_order_by_t<S> &orderBy) const {
                std::vector<std::string> expressions;
                for(auto &entry : orderBy){
                    std::string entryString;
                    {
                        std::stringstream ss;
                        ss << entry.name << " ";
                        if(!entry._collate_argument.empty()){
                            ss << "COLLATE " << entry._collate_argument << " ";
                        }
                        switch(entry.asc_desc){
                            case 1:
                                ss << "ASC";
                                break;
                            case -1:
                                ss << "DESC";
                                break;
                        }
                        entryString = ss.str();
                    }
                    expressions.push_back(move(entryString));
                };
                std::stringstream ss;
                ss << static_cast<std::string>(orderBy) << " ";
                for(size_t i = 0; i < expressions.size(); ++i) {
                    ss << expressions[i];
                    if(i < expressions.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << " ";
                return ss.str();
            }
            
            static int collate_callback(void *arg, int leftLen, const void *lhs, int rightLen, const void *rhs) {
                auto &f = *(collating_function*)arg;
                return f(leftLen, lhs, rightLen, rhs);
            }
            
            //  returns foreign keys count in storage definition
            template<class T>
            static int foreign_keys_count(T &storageImpl) {
                auto res = 0;
                storageImpl.for_each([&res](auto impl){
                    res += impl->foreign_keys_count();
                });
                return res;
            }
        };
    }
}
