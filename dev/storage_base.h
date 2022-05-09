#pragma once

#include <sqlite3.h>
#include <functional>  //  std::function, std::bind
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <utility>  //  std::move
#include <system_error>  //  std::system_error
#include <vector>  //  std::vector
#include <memory>  //  std::make_shared, std::shared_ptr
#include <map>  //  std::map
#include <type_traits>  //  std::decay, std::is_same

#include "pragma.h"
#include "limit_accessor.h"
#include "transaction_guard.h"
#include "statement_finalizer.h"
#include "tuple_helper/tuple_helper.h"
#include "row_extractor.h"
#include "connection_holder.h"
#include "backup.h"
#include "function.h"
#include "values_to_tuple.h"
#include "arg_values.h"
#include "DbConnection.h"
#include "util.h"
#include "serializing_util.h"

namespace sqlite_orm {

    namespace internal {

        struct storage_base {
            using collating_function = std::function<int(int, const void*, int, const void*)>;
            using migrator = std::function<void(const DbConnection&)>;

            std::function<void(sqlite3*)> on_open;
            pragma_t pragma;
            limit_accessor limit;

            transaction_guard_t transaction_guard() {
                this->begin_transaction();
                return {this->get_connection(),
                        std::bind(&storage_base::commit, this),
                        std::bind(&storage_base::rollback, this)};
            }

            void drop_index(const std::string& indexName) {
                std::stringstream ss;
                ss << "DROP INDEX " << quote_identifier(indexName) << std::flush;
                perform_void_exec(this->get_connection().get(), ss.str());
            }

            void drop_trigger(const std::string& triggerName) {
                std::stringstream ss;
                ss << "DROP TRIGGER " << quote_identifier(triggerName) << std::flush;
                perform_void_exec(this->get_connection().get(), ss.str());
            }

            void vacuum() {
                perform_void_exec(this->get_connection().get(), "VACUUM");
            }

            /**
             *  Drops table with given name.
             */
            void drop_table(const std::string& tableName) {
                this->drop_table_internal(this->get_connection().get(), tableName);
            }

            /**
             * Rename table named `from` to `to`.
             */
            void rename_table(const std::string& from, const std::string& to) {
                this->rename_table(this->get_connection().get(), from, to);
            }

          protected:
            void rename_table(sqlite3* db, const std::string& oldName, const std::string& newName) const {
                std::stringstream ss;
                ss << "ALTER TABLE " << quote_identifier(oldName) << " RENAME TO " << quote_identifier(newName)
                   << std::flush;
                perform_void_exec(db, ss.str());
            }

            bool table_exists(sqlite3* db, const std::string& tableName) const {
                bool result = false;
                std::stringstream ss;
                ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = " << quote_string_literal("table")
                   << " AND name = " << quote_string_literal(tableName) << std::flush;
                perform_exec(
                    db,
                    ss.str(),
                    [](void* data, int argc, char** argv, char** /*azColName*/) -> int {
                        auto& res = *(bool*)data;
                        if(argc) {
                            res = !!std::atoi(argv[0]);
                        }
                        return 0;
                    },
                    &result);
                return result;
            }

          public:
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
                auto guard = this->transaction_guard();
                return guard.commit_on_destroy = f();
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
                using data_t = std::vector<std::string>;
                perform_exec(
                    con.get(),
                    "SELECT name FROM sqlite_master WHERE type='table'",
                    [](void* data, int argc, char** argv, char** /*columnName*/) -> int {
                        auto& tableNames_ = *(data_t*)data;
                        for(int i = 0; i < argc; ++i) {
                            if(argv[i]) {
                                tableNames_.emplace_back(argv[i]);
                            }
                        }
                        return 0;
                    },
                    &tableNames);
                return tableNames;
            }

            void open_forever() {
                this->isOpenedForever = true;
                this->connection->retain();
                if(1 == this->connection->retain_count()) {
                    this->on_open_internal(this->connection->get());
                }
            }

            void register_migration(size_t from, size_t to, migrator m) {
                auto p = std::make_pair(from, to);
                this->migrations[p] = std::move(m);
            }

            void migrate_to(int target_version, const DbConnection& con) {
                int from_version = this->pragma.user_version();
                auto p = std::make_pair(from_version, target_version);
                if(migrations.find(p) != migrations.end()) {
                    auto& lambda = migrations[p];
                    lambda(con);
                    this->pragma.user_version(target_version);
                    return;
                }

                while(from_version < target_version) {
                    auto p = std::make_pair(from_version, from_version + 1);
                    if(migrations.find(p) != migrations.end()) {
                        auto& lambda = migrations[p];
                        lambda(con);
                        this->pragma.user_version(++from_version);
                    }
                    else {
                        throw std::system_error{ orm_error_code::migration_is_missing};
                    }
                }
            }

            /**
             * Call this to create user defined scalar function. Can be called at any time no matter connection is opened or no.
             * T - function class. T must have operator() overload and static name function like this:
             * ```
             *  struct SqrtFunction {
             *
             *      double operator()(double arg) const {
             *          return std::sqrt(arg);
             *      }
             *
             *      static const char *name() {
             *          return "SQRT";
             *      }
             *  };
             * ```
             * 
             * Note: Currently, a function's name must not contain white-space characters, because it doesn't get quoted.
             */
            template<class F>
            void create_scalar_function() {
                static_assert(is_scalar_function_v<F>, "F can't be an aggregate function");

                std::stringstream ss;
                ss << F::name() << std::flush;
                auto name = ss.str();
                using args_tuple = typename callable_arguments<F>::args_tuple;
                using return_type = typename callable_arguments<F>::return_type;
                auto argsCount = int(std::tuple_size<args_tuple>::value);
                if(std::is_same<args_tuple, std::tuple<arg_values>>::value) {
                    argsCount = -1;
                }
                this->scalarFunctions.emplace_back(new user_defined_scalar_function_t{
                    move(name),
                    argsCount,
                    []() -> int* {
                        return (int*)(new F());
                    },
                    /* call = */
                    [](sqlite3_context* context, void* functionVoidPointer, int argsCount, sqlite3_value** values) {
                        auto& functionPointer = *static_cast<F*>(functionVoidPointer);
                        args_tuple argsTuple;
                        using tuple_size = std::tuple_size<args_tuple>;
                        values_to_tuple<args_tuple, tuple_size::value - 1>().extract(values, argsTuple, argsCount);
                        auto result = call(functionPointer, std::move(argsTuple));
                        statement_binder<return_type>().result(context, result);
                    },
                    delete_function_callback<F>,
                });

                if(this->connection->retain_count() > 0) {
                    sqlite3* db = this->connection->get();
                    try_to_create_function(db,
                                           static_cast<user_defined_scalar_function_t&>(*this->scalarFunctions.back()));
                }
            }

            /**
             * Call this to create user defined aggregate function. Can be called at any time no matter connection is opened or no.
             * T - function class. T must have step member function, fin member function and static name function like this:
             * ```
             *   struct MeanFunction {
             *       double total = 0;
             *       int count = 0;
             *
             *       void step(double value) {
             *           total += value;
             *           ++count;
             *       }
             *
             *       int fin() const {
             *           return total / count;
             *       }
             *
             *       static std::string name() {
             *           return "MEAN";
             *       }
             *   };
             * ```
             * 
             * Note: Currently, a function's name must not contain white-space characters, because it doesn't get quoted.
             */
            template<class F>
            void create_aggregate_function() {
                static_assert(is_aggregate_function_v<F>, "F can't be a scalar function");

                std::stringstream ss;
                ss << F::name() << std::flush;
                auto name = ss.str();
                using args_tuple = typename callable_arguments<F>::args_tuple;
                using return_type = typename callable_arguments<F>::return_type;
                auto argsCount = int(std::tuple_size<args_tuple>::value);
                if(std::is_same<args_tuple, std::tuple<arg_values>>::value) {
                    argsCount = -1;
                }
                this->aggregateFunctions.emplace_back(new user_defined_aggregate_function_t{
                    move(name),
                    argsCount,
                    /* create = */
                    []() -> int* {
                        return (int*)(new F());
                    },
                    /* step = */
                    [](sqlite3_context* context, void* functionVoidPointer, int argsCount, sqlite3_value** values) {
                        auto& functionPointer = *static_cast<F*>(functionVoidPointer);
                        args_tuple argsTuple;
                        using tuple_size = std::tuple_size<args_tuple>;
                        values_to_tuple<args_tuple, tuple_size::value - 1>().extract(values, argsTuple, argsCount);
                        call(functionPointer, &F::step, move(argsTuple));
                    },
                    /* finalCall = */
                    [](sqlite3_context* context, void* functionVoidPointer) {
                        auto& functionPointer = *static_cast<F*>(functionVoidPointer);
                        auto result = functionPointer.fin();
                        statement_binder<return_type>().result(context, result);
                    },
                    delete_function_callback<F>,
                });

                if(this->connection->retain_count() > 0) {
                    sqlite3* db = this->connection->get();
                    try_to_create_function(
                        db,
                        static_cast<user_defined_aggregate_function_t&>(*this->aggregateFunctions.back()));
                }
            }

            /**
             *  Use it to delete scalar function you created before. Can be called at any time no matter connection is open or no.
             */
            template<class F>
            void delete_scalar_function() {
                static_assert(is_scalar_function_v<F>, "F cannot be an aggregate function");
                std::stringstream ss;
                ss << F::name() << std::flush;
                this->delete_function_impl(ss.str(), this->scalarFunctions);
            }

            /**
             *  Use it to delete aggregate function you created before. Can be called at any time no matter connection is open or no.
             */
            template<class F>
            void delete_aggregate_function() {
                static_assert(is_aggregate_function_v<F>, "F cannot be a scalar function");
                std::stringstream ss;
                ss << F::name() << std::flush;
                this->delete_function_impl(ss.str(), this->aggregateFunctions);
            }

            template<class C>
            void create_collation() {
                collating_function func = [](int leftLength, const void* lhs, int rightLength, const void* rhs) {
                    C collatingObject;
                    return collatingObject(leftLength, lhs, rightLength, rhs);
                };
                std::stringstream ss;
                ss << C::name() << std::flush;
                this->create_collation(ss.str(), move(func));
            }

            void create_collation(const std::string& name, collating_function f) {
                collating_function* functionPointer = nullptr;
                const auto functionExists = bool(f);
                if(functionExists) {
                    functionPointer = &(collatingFunctions[name] = std::move(f));
                } else {
                    collatingFunctions.erase(name);
                }

                //  create collations if db is open
                if(this->connection->retain_count() > 0) {
                    sqlite3* db = this->connection->get();
                    auto resultCode = sqlite3_create_collation(db,
                                                               name.c_str(),
                                                               SQLITE_UTF8,
                                                               functionPointer,
                                                               functionExists ? collate_callback : nullptr);
                    if(resultCode != SQLITE_OK) {
                        throw_translated_sqlite_error(db);
                    }
                }
            }

            template<class C>
            void delete_collation() {
                std::stringstream ss;
                ss << C::name() << std::flush;
                this->create_collation(ss.str(), {});
            }

            void begin_transaction() {
                this->begin_transaction_internal("BEGIN TRANSACTION");
            }

            void begin_deferred_transaction() {
                this->begin_transaction_internal("BEGIN DEFERRED TRANSACTION");
            }

            void begin_immediate_transaction() {
                this->begin_transaction_internal("BEGIN IMMEDIATE TRANSACTION");
            }

            void begin_exclusive_transaction() {
                this->begin_transaction_internal("BEGIN EXCLUSIVE TRANSACTION");
            }

            void commit() {
                sqlite3* db = this->connection->get();
                perform_void_exec(db, "COMMIT");
                this->connection->release();
                if(this->connection->retain_count() < 0) {
                    throw std::system_error{orm_error_code::no_active_transaction};
                }
            }

            void rollback() {
                sqlite3* db = this->connection->get();
                perform_void_exec(db, "ROLLBACK");
                this->connection->release();
                if(this->connection->retain_count() < 0) {
                    throw std::system_error{orm_error_code::no_active_transaction};
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

            /*
             * returning false when there is a transaction in place
             * otherwise true; function is not const because it has to call get_connection()
             */
            bool get_autocommit() {
                auto con = this->get_connection();
                return sqlite3_get_autocommit(con.get());
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

            int retain_count() const {
                return this->get_connection_holder()->retain_count();
            }

          protected:
            storage_base(const DbConnection& con, int foreignKeysCount) :
                pragma(std::bind(&storage_base::get_connection, this)),
                limit(std::bind(&storage_base::get_connection, this)),
                inMemory(con.get_filename().empty() || con.get_filename() == ":memory:"), connection(con.connection),
                cachedForeignKeysCount(foreignKeysCount), initByConnection{true} {}

            storage_base(const std::string& filename_, int foreignKeysCount) :
                pragma(std::bind(&storage_base::get_connection, this)),
                limit(std::bind(&storage_base::get_connection, this)),
                inMemory(filename_.empty() || filename_ == ":memory:"),
                connection(std::make_shared<connection_holder>(filename_)),
                cachedForeignKeysCount(foreignKeysCount), initByConnection{false} {
                if(this->inMemory) {
                    this->connection->retain();
                    this->on_open_internal(this->connection->get());
                }
            }

            storage_base(const storage_base& other) :
                on_open(other.on_open), pragma(std::bind(&storage_base::get_connection, this)),
                limit(std::bind(&storage_base::get_connection, this)), inMemory(other.inMemory),
                connection(other.connection), cachedForeignKeysCount(other.cachedForeignKeysCount),
                initByConnection(other.initByConnection) {
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

            connection_holder* get_connection_holder() const {
                return this->connection.get();
            }

            void begin_transaction_internal(const std::string& query) {
                this->connection->retain();
                if(1 == this->connection->retain_count()) {
                    this->on_open_internal(this->connection->get());
                }
                sqlite3* db = this->connection->get();
                perform_void_exec(db, query);
            }

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
                ss << "PRAGMA foreign_keys = " << value << std::flush;
                perform_void_exec(db, ss.str());
            }

            bool foreign_keys(sqlite3* db) {
                bool result = false;
                perform_exec(
                    db,
                    "PRAGMA foreign_keys",
                    [](void* data, int argc, char** argv, char**) -> int {
                        auto& res = *(bool*)data;
                        if(argc) {
                            res = row_extractor<bool>().extract(argv[0]);
                        }
                        return 0;
                    },
                    &result);
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
                    auto resultCode =
                        sqlite3_create_collation(db, p.first.c_str(), SQLITE_UTF8, &p.second, collate_callback);
                    if(resultCode != SQLITE_OK) {
                        throw_translated_sqlite_error(db);
                    }
                }

                for(auto& p: this->limit.limits) {
                    sqlite3_limit(db, p.first, p.second);
                }

                if(_busy_handler) {
                    sqlite3_busy_handler(this->connection->get(), busy_handler_callback, this);
                }

                for(auto& functionPointer: this->scalarFunctions) {
                    try_to_create_function(db, static_cast<user_defined_scalar_function_t&>(*functionPointer));
                }

                for(auto& functionPointer: this->aggregateFunctions) {
                    try_to_create_function(db, static_cast<user_defined_aggregate_function_t&>(*functionPointer));
                }

                if(this->on_open) {
                    this->on_open(db);
                }
            }

            void delete_function_impl(const std::string& name,
                                      std::vector<std::unique_ptr<user_defined_function_base>>& functionsVector) const {
                auto it = find_if(functionsVector.begin(), functionsVector.end(), [&name](auto& functionPointer) {
                    return functionPointer->name == name;
                });
                if(it != functionsVector.end()) {
                    functionsVector.erase(it);
                    it = functionsVector.end();

                    if(this->connection->retain_count() > 0) {
                        sqlite3* db = this->connection->get();
                        auto resultCode = sqlite3_create_function_v2(db,
                                                                     name.c_str(),
                                                                     0,
                                                                     SQLITE_UTF8,
                                                                     nullptr,
                                                                     nullptr,
                                                                     nullptr,
                                                                     nullptr,
                                                                     nullptr);
                        if(resultCode != SQLITE_OK) {
                            throw_translated_sqlite_error(db);
                        }
                    }
                } else {
                    throw std::system_error{orm_error_code::function_not_found};
                }
            }

            void try_to_create_function(sqlite3* db, user_defined_scalar_function_t& function) {
                auto resultCode = sqlite3_create_function_v2(db,
                                                             function.name.c_str(),
                                                             function.argumentsCount,
                                                             SQLITE_UTF8,
                                                             &function,
                                                             scalar_function_callback,
                                                             nullptr,
                                                             nullptr,
                                                             nullptr);
                if(resultCode != SQLITE_OK) {
                    throw_translated_sqlite_error(db);
                }
            }

            void try_to_create_function(sqlite3* db, user_defined_aggregate_function_t& function) {
                auto resultCode = sqlite3_create_function(db,
                                                          function.name.c_str(),
                                                          function.argumentsCount,
                                                          SQLITE_UTF8,
                                                          &function,
                                                          nullptr,
                                                          aggregate_function_step_callback,
                                                          aggregate_function_final_callback);
                if(resultCode != SQLITE_OK) {
                    throw_translated_sqlite_error(resultCode);
                }
            }

            static void
            aggregate_function_step_callback(sqlite3_context* context, int argsCount, sqlite3_value** values) {
                auto functionVoidPointer = sqlite3_user_data(context);
                auto functionPointer = static_cast<user_defined_aggregate_function_t*>(functionVoidPointer);
                auto aggregateContextVoidPointer = sqlite3_aggregate_context(context, sizeof(int**));
                auto aggregateContextIntPointer = static_cast<int**>(aggregateContextVoidPointer);
                if(*aggregateContextIntPointer == nullptr) {
                    *aggregateContextIntPointer = functionPointer->create();
                }
                functionPointer->step(context, *aggregateContextIntPointer, argsCount, values);
            }

            static void aggregate_function_final_callback(sqlite3_context* context) {
                auto functionVoidPointer = sqlite3_user_data(context);
                auto functionPointer = static_cast<user_defined_aggregate_function_t*>(functionVoidPointer);
                auto aggregateContextVoidPointer = sqlite3_aggregate_context(context, sizeof(int**));
                auto aggregateContextIntPointer = static_cast<int**>(aggregateContextVoidPointer);
                functionPointer->finalCall(context, *aggregateContextIntPointer);
                functionPointer->destroy(*aggregateContextIntPointer);
            }

            static void scalar_function_callback(sqlite3_context* context, int argsCount, sqlite3_value** values) {
                auto functionVoidPointer = sqlite3_user_data(context);
                auto functionPointer = static_cast<user_defined_scalar_function_t*>(functionVoidPointer);
                std::unique_ptr<int, void (*)(int*)> callablePointer(functionPointer->create(),
                                                                     functionPointer->destroy);
                if(functionPointer->argumentsCount != -1 && functionPointer->argumentsCount != argsCount) {
                    throw std::system_error{orm_error_code::arguments_count_does_not_match};
                }
                functionPointer->run(context, functionPointer, argsCount, values);
            }

            template<class F>
            static void delete_function_callback(int* pointer) {
                auto voidPointer = static_cast<void*>(pointer);
                auto fPointer = static_cast<F*>(voidPointer);
                delete fPointer;
            }

            std::string current_timestamp(sqlite3* db) {
                std::string result;
                perform_exec(
                    db,
                    "SELECT CURRENT_TIMESTAMP",
                    [](void* data, int argc, char** argv, char**) -> int {
                        auto& res = *(std::string*)data;
                        if(argc) {
                            if(argv[0]) {
                                res = row_extractor<std::string>().extract(argv[0]);
                            }
                        }
                        return 0;
                    },
                    &result);
                return result;
            }

            void drop_table_internal(sqlite3* db, const std::string& tableName) {
                std::stringstream ss;
                ss << "DROP TABLE " << streaming_identifier(tableName) << std::flush;
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
            template<class S>
            static int foreign_keys_count(const S& storageImpl) {
                auto res = 0;

                storageImpl.for_each([&res](const auto& tImpl) {
                    using qualified_type = std::decay_t<decltype(tImpl)>;
                    static_if<std::is_base_of<basic_table, table_type_or_none_t<qualified_type>>::value>(
                        [&res](const auto& tImpl) {
                            res += tImpl.table.foreign_keys_count();
                        })(tImpl);
                });
                return res;
            }

            bool calculate_remove_add_columns(std::vector<const table_xinfo*>& columnsToAdd,
                                              std::vector<table_xinfo>& storageTableInfo,
                                              std::vector<table_xinfo>& dbTableInfo) const {
                bool notEqual = false;

                //  iterate through storage columns
                for(size_t storageColumnInfoIndex = 0; storageColumnInfoIndex < storageTableInfo.size();
                    ++storageColumnInfoIndex) {

                    //  get storage's column info
                    auto& storageColumnInfo = storageTableInfo[storageColumnInfoIndex];
                    auto& columnName = storageColumnInfo.name;

                    //  search for a column in db eith the same name
                    auto dbColumnInfoIt = std::find_if(dbTableInfo.begin(), dbTableInfo.end(), [&columnName](auto& ti) {
                        return ti.name == columnName;
                    });
                    if(dbColumnInfoIt != dbTableInfo.end()) {
                        auto& dbColumnInfo = *dbColumnInfoIt;
                        auto columnsAreEqual =
                            dbColumnInfo.name == storageColumnInfo.name &&
                            dbColumnInfo.notnull == storageColumnInfo.notnull &&
                            (!dbColumnInfo.dflt_value.empty()) == (!storageColumnInfo.dflt_value.empty()) &&
                            dbColumnInfo.pk == storageColumnInfo.pk &&
                            (dbColumnInfo.hidden == 0) == (storageColumnInfo.hidden == 0);
                        if(!columnsAreEqual) {
                            notEqual = true;
                            break;
                        }
                        dbTableInfo.erase(dbColumnInfoIt);
                        storageTableInfo.erase(storageTableInfo.begin() +
                                               static_cast<ptrdiff_t>(storageColumnInfoIndex));
                        --storageColumnInfoIndex;
                    } else {
                        columnsToAdd.push_back(&storageColumnInfo);
                    }
                }
                return notEqual;
            }

            const bool initByConnection;
            const bool inMemory;
            bool isOpenedForever = false;
            std::shared_ptr<connection_holder> connection;
            std::map<std::string, collating_function> collatingFunctions;
            const int cachedForeignKeysCount;
            std::function<int(int)> _busy_handler;
            std::vector<std::unique_ptr<user_defined_function_base>> scalarFunctions;
            std::vector<std::unique_ptr<user_defined_function_base>> aggregateFunctions;
            std::map<std::pair<size_t, size_t>, migrator> migrations;  // <version pair, lambda>
        };
    }
}
