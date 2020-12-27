#pragma once

#include <functional>  //  std::function, std::bind
#include <sqlite3.h>
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <utility>  //  std::move
#include <system_error>  //  std::system_error, std::error_code, std::make_error_code
#include <vector>  //  std::vector
#include <memory>  //  std::make_shared, std::shared_ptr
#include <map>  //  std::map
#include <type_traits>  //  std::decay, std::is_same
#include <algorithm>  //  std::iter_swap

#include "pragma.h"
#include "limit_accesor.h"
#include "transaction_guard.h"
#include "statement_finalizer.h"
#include "type_printer.h"
#include "tuple_helper.h"
#include "row_extractor.h"
#include "util.h"
#include "connection_holder.h"
#include "backup.h"

namespace sqlite_orm {

    namespace internal {

        struct storage_base {
            using collating_function = std::function<int(int, const void*, int, const void*)>;

            std::function<void(sqlite3*)> on_open;
            pragma_t pragma;
            limit_accesor limit;

            transaction_guard_t transaction_guard() {
                this->begin_transaction();
                auto commitFunc = std::bind(static_cast<void (storage_base::*)()>(&storage_base::commit), this);
                auto rollbackFunc = std::bind(static_cast<void (storage_base::*)()>(&storage_base::rollback), this);
                return {this->get_connection(), move(commitFunc), move(rollbackFunc)};
            }

            void drop_index(const std::string& indexName) {
                std::stringstream ss;
                ss << "DROP INDEX '" << indexName + "'";
                perform_void_exec(get_connection().get(), ss.str());
            }

            void vacuum() {
                perform_void_exec(get_connection().get(), "VACUUM");
            }

            /**
             *  Drops table with given name.
             */
            void drop_table(const std::string& tableName) {
                auto con = this->get_connection();
                this->drop_table_internal(tableName, con.get());
            }

            /**
             * Rename table named `from` to `to`.
             */
            void rename_table(const std::string& from, const std::string& to) {
                std::stringstream ss;
                ss << "ALTER TABLE '" << from << "' RENAME TO '" << to << "'";
                perform_void_exec(get_connection().get(), ss.str());
            }

            /**
             *  sqlite3_changes function.
             */
            int changes() {
                auto con = this->get_connection();
                return sqlite3_changes(con.get());
            }

            /**
             *  sqlite3_total_changes function.
             */
            int total_changes() {
                auto con = this->get_connection();
                return sqlite3_total_changes(con.get());
            }

            int64 last_insert_rowid() {
                auto con = this->get_connection();
                return sqlite3_last_insert_rowid(con.get());
            }

            int busy_timeout(int ms) {
                auto con = this->get_connection();
                return sqlite3_busy_timeout(con.get(), ms);
            }

            /**
             *  Returns libsqltie3 lib version, not sqlite_orm
             */
            std::string libversion() {
                return sqlite3_libversion();
            }

            bool transaction(const std::function<bool()>& f) {
                auto guard = transaction_guard();
                auto shouldCommit = f();
                if(shouldCommit) {
                    guard.commit();
                } else {
                    guard.rollback();
                }
                return shouldCommit;
            }

            std::string current_timestamp() {
                auto con = this->get_connection();
                return this->current_timestamp(con.get());
            }

#if SQLITE_VERSION_NUMBER >= 3007010
            /**
             * \fn db_release_memory
             * \brief Releases freeable memory of database. It is function can/should be called periodically by
             * application, if application has less memory usage constraint. \note sqlite3_db_release_memory added
             * in 3.7.10 https://sqlite.org/changes.html
             */
            int db_release_memory() {
                auto con = this->get_connection();
                return sqlite3_db_release_memory(con.get());
            }
#endif

            /**
             *  Returns existing permanent table names in database. Doesn't check storage itself - works only with
             * actual database.
             *  @return Returns list of tables in database.
             */
            std::vector<std::string> table_names() {
                auto con = this->get_connection();
                std::vector<std::string> tableNames;
                std::string sql = "SELECT name FROM sqlite_master WHERE type='table'";
                using data_t = std::vector<std::string>;
                auto db = con.get();
                int res = sqlite3_exec(
                    db,
                    sql.c_str(),
                    [](void* data, int argc, char** argv, char* * /*columnName*/) -> int {
                        auto& tableNames_ = *(data_t*)data;
                        for(int i = 0; i < argc; i++) {
                            if(argv[i]) {
                                tableNames_.push_back(argv[i]);
                            }
                        }
                        return 0;
                    },
                    &tableNames,
                    nullptr);

                if(res != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                            sqlite3_errmsg(db));
                }
                return tableNames;
            }

            void open_forever() {
                this->isOpenedForever = true;
                this->connection->retain();
                if(1 == this->connection->retain_count()) {
                    this->on_open_internal(this->connection->get());
                }
            }

            void create_collation(const std::string& name, collating_function f) {
                collating_function* functionPointer = nullptr;
                if(f) {
                    functionPointer = &(collatingFunctions[name] = std::move(f));
                } else {
                    collatingFunctions.erase(name);
                }

                //  create collations if db is open
                if(this->connection->retain_count() > 0) {
                    auto db = this->connection->get();
                    if(sqlite3_create_collation(db,
                                                name.c_str(),
                                                SQLITE_UTF8,
                                                functionPointer,
                                                f ? collate_callback : nullptr) != SQLITE_OK) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                }
            }

            void begin_transaction() {
                this->connection->retain();
                if(1 == this->connection->retain_count()) {
                    this->on_open_internal(this->connection->get());
                }
                auto db = this->connection->get();
                perform_void_exec(db, "BEGIN TRANSACTION");
            }

            void commit() {
                auto db = this->connection->get();
                perform_void_exec(db, "COMMIT");
                this->connection->release();
                if(this->connection->retain_count() < 0) {
                    throw std::system_error(std::make_error_code(orm_error_code::no_active_transaction));
                }
            }

            void rollback() {
                auto db = this->connection->get();
                perform_void_exec(db, "ROLLBACK");
                this->connection->release();
                if(this->connection->retain_count() < 0) {
                    throw std::system_error(std::make_error_code(orm_error_code::no_active_transaction));
                }
            }

            void backup_to(const std::string& filename) {
                auto backup = this->make_backup_to(filename);
                backup.step(-1);
            }

            void backup_to(storage_base& other) {
                auto backup = this->make_backup_to(other);
                backup.step(-1);
            }

            void backup_from(const std::string& filename) {
                auto backup = this->make_backup_from(filename);
                backup.step(-1);
            }

            void backup_from(storage_base& other) {
                auto backup = this->make_backup_from(other);
                backup.step(-1);
            }

            backup_t make_backup_to(const std::string& filename) {
                auto holder = std::make_unique<connection_holder>(filename);
                connection_ref conRef{*holder};
                return {conRef, "main", this->get_connection(), "main", move(holder)};
            }

            backup_t make_backup_to(storage_base& other) {
                return {other.get_connection(), "main", this->get_connection(), "main", {}};
            }

            backup_t make_backup_from(const std::string& filename) {
                auto holder = std::make_unique<connection_holder>(filename);
                connection_ref conRef{*holder};
                return {this->get_connection(), "main", conRef, "main", move(holder)};
            }

            backup_t make_backup_from(storage_base& other) {
                return {this->get_connection(), "main", other.get_connection(), "main", {}};
            }

            const std::string& filename() const {
                return this->connection->filename;
            }

            /**
             * Checks whether connection to database is opened right now.
             * Returns always `true` for in memory databases.
             */
            bool is_opened() const {
                return this->connection->retain_count() > 0;
            }

            int busy_handler(std::function<int(int)> handler) {
                _busy_handler = move(handler);
                if(this->is_opened()) {
                    if(_busy_handler) {
                        return sqlite3_busy_handler(this->connection->get(), busy_handler_callback, this);
                    } else {
                        return sqlite3_busy_handler(this->connection->get(), nullptr, nullptr);
                    }
                } else {
                    return SQLITE_OK;
                }
            }

          protected:
            storage_base(const std::string& filename_, int foreignKeysCount) :
                pragma(std::bind(&storage_base::get_connection, this)),
                limit(std::bind(&storage_base::get_connection, this)),
                inMemory(filename_.empty() || filename_ == ":memory:"),
                connection(std::make_unique<connection_holder>(filename_)), cachedForeignKeysCount(foreignKeysCount) {
                if(this->inMemory) {
                    this->connection->retain();
                    this->on_open_internal(this->connection->get());
                }
            }

            storage_base(const storage_base& other) :
                on_open(other.on_open), pragma(std::bind(&storage_base::get_connection, this)),
                limit(std::bind(&storage_base::get_connection, this)), inMemory(other.inMemory),
                connection(std::make_unique<connection_holder>(other.connection->filename)),
                cachedForeignKeysCount(other.cachedForeignKeysCount) {
                if(this->inMemory) {
                    this->connection->retain();
                    this->on_open_internal(this->connection->get());
                }
            }

            ~storage_base() {
                if(this->isOpenedForever) {
                    this->connection->release();
                }
                if(this->inMemory) {
                    this->connection->release();
                }
            }

            const bool inMemory;
            bool isOpenedForever = false;
            std::unique_ptr<connection_holder> connection;
            std::map<std::string, collating_function> collatingFunctions;
            const int cachedForeignKeysCount;
            std::function<int(int)> _busy_handler;

            connection_ref get_connection() {
                connection_ref res{*this->connection};
                if(1 == this->connection->retain_count()) {
                    this->on_open_internal(this->connection->get());
                }
                return res;
            }

#if SQLITE_VERSION_NUMBER >= 3006019

            void foreign_keys(sqlite3* db, bool value) {
                std::stringstream ss;
                ss << "PRAGMA foreign_keys = " << value;
                perform_void_exec(db, ss.str());
            }

            bool foreign_keys(sqlite3* db) {
                std::string query = "PRAGMA foreign_keys";
                auto result = false;
                auto rc = sqlite3_exec(
                    db,
                    query.c_str(),
                    [](void* data, int argc, char** argv, char**) -> int {
                        auto& res = *(bool*)data;
                        if(argc) {
                            res = row_extractor<bool>().extract(argv[0]);
                        }
                        return 0;
                    },
                    &result,
                    nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                            sqlite3_errmsg(db));
                }
                return result;
            }

#endif
            void on_open_internal(sqlite3* db) {

#if SQLITE_VERSION_NUMBER >= 3006019
                if(this->cachedForeignKeysCount) {
                    this->foreign_keys(db, true);
                }
#endif
                if(this->pragma._synchronous != -1) {
                    this->pragma.synchronous(this->pragma._synchronous);
                }

                if(this->pragma._journal_mode != -1) {
                    this->pragma.set_pragma("journal_mode", static_cast<journal_mode>(this->pragma._journal_mode), db);
                }

                for(auto& p: this->collatingFunctions) {
                    if(sqlite3_create_collation(db, p.first.c_str(), SQLITE_UTF8, &p.second, collate_callback) !=
                       SQLITE_OK) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                }

                for(auto& p: this->limit.limits) {
                    sqlite3_limit(db, p.first, p.second);
                }

                if(_busy_handler) {
                    sqlite3_busy_handler(this->connection->get(), busy_handler_callback, this);
                }

                if(this->on_open) {
                    this->on_open(db);
                }
            }

            std::string current_timestamp(sqlite3* db) {
                std::string result;
                std::stringstream ss;
                ss << "SELECT CURRENT_TIMESTAMP";
                auto query = ss.str();
                auto rc = sqlite3_exec(
                    db,
                    query.c_str(),
                    [](void* data, int argc, char** argv, char**) -> int {
                        auto& res = *(std::string*)data;
                        if(argc) {
                            if(argv[0]) {
                                res = row_extractor<std::string>().extract(argv[0]);
                            }
                        }
                        return 0;
                    },
                    &result,
                    nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                            sqlite3_errmsg(db));
                }
                return result;
            }

            void drop_table_internal(const std::string& tableName, sqlite3* db) {
                std::stringstream ss;
                ss << "DROP TABLE '" << tableName + "'";
                perform_void_exec(db, ss.str());
            }

            static int collate_callback(void* arg, int leftLen, const void* lhs, int rightLen, const void* rhs) {
                auto& f = *(collating_function*)arg;
                return f(leftLen, lhs, rightLen, rhs);
            }

            static int busy_handler_callback(void* selfPointer, int triesCount) {
                auto& storage = *static_cast<storage_base*>(selfPointer);
                if(storage._busy_handler) {
                    return storage._busy_handler(triesCount);
                } else {
                    return 0;
                }
            }

            //  returns foreign keys count in storage definition
            template<class T>
            static int foreign_keys_count(T& storageImpl) {
                auto res = 0;
                storageImpl.for_each([&res](auto& impl) {
                    res += impl.foreign_keys_count();
                });
                return res;
            }
        };
    }
}
