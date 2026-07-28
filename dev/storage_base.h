#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <cstdlib>  // atoi
#include <memory>  //  std::allocator, std::unique_ptr, std::make_unique
#include <functional>  //  std::function, std::bind, std::bind_front
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <iomanip>  //  std::flush
#include <utility>  //  std::move
#include <system_error>  //  std::system_error
#include <vector>  //  std::vector
#include <list>  //  std::list
#include <map>  //  std::map
#include <type_traits>  //  std::is_same, std::is_aggregate
#include <algorithm>  //  std::find_if, std::ranges::find
#endif

#include "functional/cxx_string_view.h"
#include "functional/cxx_tuple_polyfill.h"  //  std::apply
#include "tuple_helper/tuple_iteration.h"
#include "pragma.h"
#include "limit_accessor.h"
#include "transaction_guard.h"
#include "row_extractor.h"
#include "connection_holder.h"
#include "backup.h"
#include "function.h"
#include "values_to_tuple.h"
#include "arg_values.h"
#include "util.h"
#include "xdestroy_handling.h"
#include "udf_proxy.h"
#include "serializing_util.h"
#include "table_info.h"
#include "storage_options.h"

namespace sqlite_orm::internal {
    struct storage_base {
      public:
        using collating_function = std::function<int(int, const void*, int, const void*)>;

      protected:
        const bool inMemory;
        bool isOpenedForever = false;
        std::unique_ptr<connection_holder> connection;
        std::map<std::string, collating_function> collatingFunctions;
        const int cachedForeignKeysCount;
        std::function<int(int)> _busy_handler;
        std::list<udf_proxy> scalarFunctions;
        std::list<udf_proxy> aggregateFunctions;
        const sqlite_executor executor;

      public:
        /** 
         *  Attention: You must ensure that to set this function only from a single-threaded context.
         */
        std::function<void(sqlite3*)> on_open;
        pragma_t pragma;
        /** 
         *  Attention: You must ensure that to set database limit only from a single-threaded context.
         */
        limit_accessor limit;

        transaction_guard_t transaction_guard() {
            auto connection = this->get_connection();
            sqlite3* db = connection.get();
            this->executor.perform_void_exec(db, "BEGIN TRANSACTION");
            return {std::move(connection),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "COMMIT"),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "ROLLBACK")};
        }

        transaction_guard_t deferred_transaction_guard() {
            auto connection = this->get_connection();
            sqlite3* db = connection.get();
            this->executor.perform_void_exec(db, "BEGIN DEFERRED TRANSACTION");
            return {std::move(connection),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "COMMIT"),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "ROLLBACK")};
        }

        transaction_guard_t immediate_transaction_guard() {
            auto connection = this->get_connection();
            sqlite3* db = connection.get();
            this->executor.perform_void_exec(db, "BEGIN IMMEDIATE TRANSACTION");
            return {std::move(connection),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "COMMIT"),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "ROLLBACK")};
        }

        transaction_guard_t exclusive_transaction_guard() {
            auto connection = this->get_connection();
            sqlite3* db = connection.get();
            this->executor.perform_void_exec(db, "BEGIN EXCLUSIVE TRANSACTION");
            return {std::move(connection),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "COMMIT"),
                    std::bind(&sqlite_executor::perform_void_exec, &executor, db, "ROLLBACK")};
        }

        /**
         *  Drops index with given name. 
         *  Calls `DROP INDEX indexName`.
         *  More info: https://www.sqlite.org/lang_dropindex.html
         */
        void drop_index(const std::string& indexName) {
            auto connection = this->get_connection();
            this->drop_index_internal(connection.get(), indexName, false);
        }

        /**
         *  Drops trigger with given name if trigger exists. 
         *  Calls `DROP INDEX IF EXISTS indexName`.
         *  More info: https://www.sqlite.org/lang_dropindex.html
         */
        void drop_index_if_exists(const std::string& indexName) {
            auto connection = this->get_connection();
            this->drop_index_internal(connection.get(), indexName, true);
        }

        /**
         *  Drops trigger with given name. 
         *  Calls `DROP TRIGGER triggerName`.
         *  More info: https://www.sqlite.org/lang_droptrigger.html
         */
        void drop_trigger(const std::string& triggerName) {
            auto connection = this->get_connection();
            this->drop_trigger_internal(connection.get(), triggerName, false);
        }

        /**
         *  Drops trigger with given name if trigger exists. 
         *  Calls `DROP TRIGGER IF EXISTS triggerName`.
         *  More info: https://www.sqlite.org/lang_droptrigger.html
         */
        void drop_trigger_if_exists(const std::string& triggerName) {
            auto connection = this->get_connection();
            this->drop_trigger_internal(connection.get(), triggerName, true);
        }

        /**
         *  Drops table with given name. 
         *  Calls `DROP TABLE tableName`.
         *  More info: https://www.sqlite.org/lang_droptable.html
         */
        void drop_table(const std::string& tableName) {
            auto connection = this->get_connection();
            this->drop_table_internal(connection.get(), tableName, false);
        }

        /**
         *  Drops table with given name if table exists. 
         *  Calls `DROP TABLE IF EXISTS tableName`.
         *  More info: https://www.sqlite.org/lang_droptable.html
         */
        void drop_table_if_exists(const std::string& tableName) {
            auto connection = this->get_connection();
            this->drop_table_internal(connection.get(), tableName, true);
        }

#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
        /**
         *  Drops the view with the specified name.
         *  Calls `DROP VIEW "viewName"`.
         *  More info: https://www.sqlite.org/lang_droptable.html
         */
        void drop_view(std::string_view viewName) {
            auto connection = this->get_connection();
            this->drop_view_internal(connection.get(), viewName, false);
        }

        /**
         *  Drops the view with the specified name if it exists. 
         *  Calls `DROP VIEW IF EXISTS "viewName"`.
         *  More info: https://www.sqlite.org/lang_droptable.html
         */
        void drop_view_if_exists(std::string_view viewName) {
            auto connection = this->get_connection();
            this->drop_view_internal(connection.get(), viewName, true);
        }
#endif

        /**
         *  Rename table named `from` to `to`.
         */
        void rename_table(const std::string& from, const std::string& to) {
            auto connection = this->get_connection();
            this->rename_table_internal(connection.get(), from, to);
        }

        /**
         *  `VACUUM` query.
         *  More info: https://www.sqlite.org/lang_vacuum.html
         */
        void vacuum() {
            auto connection = this->get_connection();
            this->executor.perform_void_exec(connection.get(), "VACUUM");
        }

        /**
         *  Checks whether table exists in db. Doesn't check storage itself - works only with actual database.
         *  Note: table can be not mapped to a storage
         *  @return true if table with a given name exists in db, false otherwise.
         */
        bool table_exists(const std::string& tableName) {
            auto connection = this->get_connection();
            return this->table_exists_internal(connection.get(), tableName);
        }

#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
        /**
         *  Directly checks the actual database whether the specified view exists, bypassing the library's 'storage' mapping.
         *  @return true if view with the specified name exists in the database, false otherwise.
         */
        bool view_exists(std::string_view viewName) {
            auto connection = this->get_connection();
            return this->view_exists_internal(connection.get(), viewName);
        }
#endif

      protected:
        void rename_table_internal(sqlite3* db, const std::string& oldName, const std::string& newName) const {
            std::string sql;
            {
                std::stringstream ss;
                ss << "ALTER TABLE " << streaming_identifier(oldName) << " RENAME TO " << streaming_identifier(newName)
                   << std::flush;
                sql = ss.str();
            }
            this->executor.perform_void_exec(db, sql.c_str());
        }

        bool table_exists_internal(sqlite3* db, const std::string& tableName) const {
            return this->object_exists(db, "table", tableName);
        }

#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
        bool view_exists_internal(sqlite3* db, const std::string_view& viewName) const {
            return this->object_exists(db, "view", viewName);
        }
#endif

        void add_generated_cols(std::vector<const table_xinfo*>& columnsToAdd,
                                const std::vector<table_xinfo>& storageTableInfo) {
            //  iterate through storage columns
            for (const table_xinfo& storageColumnInfo: storageTableInfo) {
                if (storageColumnInfo.hidden) {
                    columnsToAdd.push_back(&storageColumnInfo);
                }
            }
        }

      public:
        /**
         *  sqlite3_changes function.
         */
        int changes() {
            auto connection = this->get_connection();
            return sqlite3_changes(connection.get());
        }

        /**
         *  sqlite3_total_changes function.
         */
        int total_changes() {
            auto connection = this->get_connection();
            return sqlite3_total_changes(connection.get());
        }

        int64 last_insert_rowid() {
            auto connection = this->get_connection();
            return sqlite3_last_insert_rowid(connection.get());
        }

        int busy_timeout(int ms) {
            auto connection = this->get_connection();
            return sqlite3_busy_timeout(connection.get(), ms);
        }

        /**
         *  Returns libsqlite3 version, not sqlite_orm
         */
        std::string libversion() {
            return sqlite3_libversion();
        }

        bool transaction(const std::function<bool()>& function) {
            if (!function) {
                return false;
            }
            auto guard = this->transaction_guard();
            return guard.commit_on_destroy = function();
        }

        std::string current_time() {
            auto connection = this->get_connection();
            return this->current_time(connection.get());
        }

        std::string current_date() {
            auto connection = this->get_connection();
            return this->current_date(connection.get());
        }

        std::string current_timestamp() {
            auto connection = this->get_connection();
            return this->current_timestamp(connection.get());
        }

#if SQLITE_VERSION_NUMBER >= 3007010
        /**
         *  \fn db_release_memory
         *  \brief Releases freeable memory of database. It is function can/should be called periodically by
         *  application, if application has less memory usage constraint. \note sqlite3_db_release_memory added
         *  in 3.7.10 https://sqlite.org/changes.html
         */
        int db_release_memory() {
            auto connection = this->get_connection();
            return sqlite3_db_release_memory(connection.get());
        }
#endif

        /**
         *  Returns the names of existing permanent views in the database. Doesn't check storage itself - works only with
         *  actual database.
         *  @return Returns a list of views in the database.
         */
        std::vector<std::string> view_names() {
            return this->object_names("view");
        }

        /**
         *  Returns the names of existing permanent tables in the database. Doesn't check storage itself - works only with
         *  actual database.
         *  @return Returns a list of tables in the database.
         */
        std::vector<std::string> table_names() {
            return this->object_names("table");
        }

        /**
         *  Returns the names of existing permanent triggers in the database. Doesn't check storage itself - works only with
         *  actual database.
         *  @return Returns a list of triggers in the database.
         */
        std::vector<std::string> trigger_names() {
            return this->object_names("trigger");
        }

        /**
         *  Call it once during storage lifetime to make it keeping its connection opened till dtor call.
         *  By default if storage is not in-memory it calls `sqlite3_open` only when the connection is really
         *  needed and closes when it is not needed. This function establishes a permanent connection.
         *  In-memory storage always establishes a permanent connection, so calling this method is a no-op.
         *  
         *  Attention: You must ensure to call this method only in a single-threaded context.
         *  An alternative way to establish a permanent connection is to specify control options to `make_storage()`.
         */
        void open_forever() {
            if (!this->isOpenedForever) {
                this->isOpenedForever = true;
                this->connection = std::make_unique<connection_holder>(*this->connection, std::true_type{});
                this->connection->open();
            }
        }

        /**
         *  Create an application-defined scalar SQL function.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is opened or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         *  
         *  Note: `create_scalar_function()` merely creates a closure to generate an instance of the scalar function object,
         *  together with a copy of the passed initialization arguments.
         *  If `F` is a stateless function object, an instance of the function object is created once, otherwise
         *  an instance of the function object is repeatedly recreated for each result row,
         *  ensuring that the calculations always start with freshly initialized values.
         *  
         *  T - function class. T must have a single call operator and static name function like this:
         *  ```
         *  struct SqrtFunction {
         *    double operator()(double arg) const {
         *      return std::sqrt(arg);
         *    }
         *    
         *    static const char* name() {
         *      return "SQRT";
         *    }
         *  };
         *  ```
         */
        template<class F, class... Args>
        void create_scalar_function(Args&&... constructorArgs) {
            static_assert(is_scalar_udf_v<F>, "F must be a scalar function");

            this->create_scalar_function_impl(
                udf_holder<F>{},
#ifdef SQLITE_ORM_PACK_EXPANSION_IN_INIT_CAPTURE_SUPPORTED
                /* constructAt */ [... constructorArgs = std::move(constructorArgs)](void* location) {
#else
                /* constructAt */
                [constructorArgs...](void* location) {
#endif
                    std::allocator<F> allocator;
                    using traits = std::allocator_traits<decltype(allocator)>;
                    traits::construct(allocator, (F*)location, constructorArgs...);
                });
        }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /**
         *  Create an application-defined scalar function.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is opened or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         *  
         *  Note: `create_scalar_function()` merely creates a closure to generate an instance of the scalar function object,
         *  together with a copy of the passed initialization arguments.
         *  If `F` is a stateless function object, an instance of the function object is created once, otherwise
         *  an instance of the function object is repeatedly recreated for each result row,
         *  ensuring that the calculations always start with freshly initialized values.
         */
        template<orm_scalar_function auto f, std::copy_constructible... Args>
        void create_scalar_function(Args&&... constructorArgs) {
            return this->create_scalar_function<auto_udf_type_t<f>>(std::forward<Args>(constructorArgs)...);
        }

        /**
         *  Create an application-defined scalar function.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is opened or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         *  
         *  If `quotedF` contains a freestanding function, stateless lambda or stateless function object,
         *  `quoted_scalar_function::_callable()` uses the original function object, assuming it is free of side effects;
         *  otherwise, it repeatedly uses a copy of the contained function object, assuming possible side effects.
         */
        template<decltype(auto) quotedF>
            requires (orm_quoted_scalar_function<decltype(quotedF)>)
        void create_scalar_function() {
            using signature_type = auto_udf_type_t<(quotedF)>;
            using args_tuple = typename callable_arguments<signature_type>::args_tuple;
            using return_type = typename callable_arguments<signature_type>::return_type;
            constexpr int argsCount =
                std::is_same<args_tuple, std::tuple<arg_values>>::value ? -1 : int(std::tuple_size<args_tuple>::value);
            this->scalarFunctions.emplace_back(
                std::string{quotedF.name()},
                argsCount,
                /* constructAt = */
                nullptr,
                /* destroy = */
                nullptr,
                /* call = */
                [](sqlite3_context* context, int nValues, sqlite3_value** values) {
                    proxy_assert_args_count(context, nValues);
                    args_tuple argsTuple = tuple_from_values<args_tuple>{}(values, nValues);
                    auto result = polyfill::apply(quotedF._callable(), std::move(argsTuple));
                    statement_binder<return_type>().result(context, result);
                },
                /* finalCall = */
                nullptr,
                std::pair{nullptr, null_xdestroy_f});

            if (connection_ptr maybeConnection = *this->connection) {
                try_to_create_scalar_function(maybeConnection.get(), this->scalarFunctions.back());
            }
        }
#endif

        /**
         *  Create an application-defined aggregate SQL function.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is opened or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         *  
         *  Note: `create_aggregate_function()` merely creates a closure to generate an instance of the aggregate function object,
         *  together with a copy of the passed initialization arguments.
         *  An instance of the function object is repeatedly recreated for each result row,
         *  ensuring that the calculations always start with freshly initialized values.
         *  
         *  T - function class. T must have step member function, fin member function and static name function like this:
         *  ```
         *  struct MeanFunction {
         *    double total = 0;
         *    int count = 0;
         *    
         *    void step(double value) {
         *      total += value;
         *      ++count;
         *    }
         *    
         *    int fin() const {
         *      return total / count;
         *    }
         *    
         *    static std::string name() {
         *      return "MEAN";
         *    }
         *  };
         *  ```
         */
        template<class F, class... Args>
        void create_aggregate_function(Args&&... constructorArgs) {
            static_assert(is_aggregate_udf_v<F>, "F must be an aggregate function");

            this->create_aggregate_function_impl(
                udf_holder<F>{}, /* constructAt = */
#ifdef SQLITE_ORM_PACK_EXPANSION_IN_INIT_CAPTURE_SUPPORTED
                /* constructAt */ [... constructorArgs = std::move(constructorArgs)](void* location) {
#else
                /* constructAt */
                [constructorArgs...](void* location) {
#endif
                    std::allocator<F> allocator;
                    using traits = std::allocator_traits<decltype(allocator)>;
                    traits::construct(allocator, (F*)location, constructorArgs...);
                });
        }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /**
         *  Create an application-defined aggregate function.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is opened or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         *  
         *  Note: `create_aggregate_function()` merely creates a closure to generate an instance of the aggregate function object,
         *  together with a copy of the passed initialization arguments.
         *  An instance of the function object is repeatedly recreated for each result row,
         *  ensuring that the calculations always start with freshly initialized values.
         */
        template<orm_aggregate_function auto f, std::copy_constructible... Args>
        void create_aggregate_function(Args&&... constructorArgs) {
            return this->create_aggregate_function<auto_udf_type_t<f>>(std::forward<Args>(constructorArgs)...);
        }
#endif

        /**
         *  Delete a scalar function you created before.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is open or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        template<class F>
        void delete_scalar_function() {
            static_assert(is_scalar_udf_v<F>, "F must be a scalar function");
            udf_holder<F> udfName;
            this->delete_function_impl(udfName(), this->scalarFunctions);
        }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /**
         *  Delete a scalar function you created before.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is open or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        template<orm_scalar_function auto f>
        void delete_scalar_function() {
            this->delete_function_impl(f.name(), this->scalarFunctions);
        }

        /**
         *  Delete a quoted scalar function you created before.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is open or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        template<decltype(auto) quotedF>
            requires (orm_quoted_scalar_function<decltype(quotedF)>)
        void delete_scalar_function() {
            this->delete_function_impl(quotedF.name(), this->scalarFunctions);
        }
#endif

        /**
         *  Delete aggregate function you created before.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is open or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        template<class F>
        void delete_aggregate_function() {
            static_assert(is_aggregate_udf_v<F>, "F must be an aggregate function");
            udf_holder<F> udfName;
            this->delete_function_impl(udfName(), this->aggregateFunctions);
        }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /**
         *  Delete aggregate function you created before.
         *  Can be called at any time (in a single-threaded context) no matter whether the database connection is open or not.
         *  
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        template<orm_aggregate_function auto f>
        void delete_aggregate_function() {
            this->delete_function_impl(f.name(), this->aggregateFunctions);
        }
#endif

        /** 
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        template<class C>
        void create_collation() {
            collating_function func = [](int leftLength, const void* lhs, int rightLength, const void* rhs)
                                          SQLITE_ORM_STATIC_CALLOP {
                                              C collatingObject;
                                              return collatingObject(leftLength, lhs, rightLength, rhs);
                                          };
            std::stringstream ss;
            ss << C::name() << std::flush;
            this->create_collation(ss.str(), std::move(func));
        }

        /** 
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        void create_collation(const std::string& name, collating_function f) {
            const auto functionExists = bool(f);
            collating_function* function = nullptr;
            if (functionExists) {
                function = &(collatingFunctions[name] = std::move(f));
            }

            //  create collations if db is open
            if (connection_ptr maybeConnection = *this->connection) {
                const int rc = sqlite3_create_collation(maybeConnection.get(),
                                                        name.c_str(),
                                                        SQLITE_UTF8,
                                                        function,
                                                        functionExists ? collate_callback : nullptr);
                if (rc != SQLITE_OK) {
                    throw_translated_sqlite_error(rc);
                }
            }

            if (!functionExists) {
                collatingFunctions.erase(name);
            }
        }

        /** 
         *  Attention: You must ensure that to call this method only in a single-threaded context.
         */
        template<class C>
        void delete_collation() {
            std::stringstream ss;
            ss << C::name() << std::flush;
            this->create_collation(ss.str(), {});
        }

        void begin_transaction() {
            sqlite3* db = this->connection->retain();
            this->executor.perform_void_exec(db, "BEGIN TRANSACTION");
        }

        void begin_deferred_transaction() {
            sqlite3* db = this->connection->retain();
            this->executor.perform_void_exec(db, "BEGIN DEFERRED TRANSACTION");
        }

        void begin_immediate_transaction() {
            sqlite3* db = this->connection->retain();
            this->executor.perform_void_exec(db, "BEGIN IMMEDIATE TRANSACTION");
        }

        void begin_exclusive_transaction() {
            sqlite3* db = this->connection->retain();
            this->executor.perform_void_exec(db, "BEGIN EXCLUSIVE TRANSACTION");
        }

        void commit() {
            if (connection_ptr maybeConnection = *this->connection) {
                this->executor.perform_void_exec(maybeConnection.get(), "COMMIT");
            }
            // check for programming error on user's side not having called `begin_transaction()` before
            else {
                throw std::system_error{orm_error_code::no_active_transaction};
            }
        }

        void rollback() {
            if (connection_ptr maybeConnection = *this->connection) {
                this->executor.perform_void_exec(maybeConnection.get(), "ROLLBACK");
            }
            // check for programming error on user's side not having called `begin_transaction()` before
            else {
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
            auto connection = std::make_unique<connection_holder>(false, db_arguments{filename}, nullptr);
            connection_ref conRef{*connection};
            return {std::move(conRef), "main", this->get_connection(), "main", std::move(connection)};
        }

        backup_t make_backup_to(storage_base& other) {
            return {other.get_connection(), "main", this->get_connection(), "main", {}};
        }

        backup_t make_backup_from(const std::string& filename) {
            auto connection = std::make_unique<connection_holder>(false, db_arguments{filename}, nullptr);
            connection_ref conRef{*connection};
            return {this->get_connection(), "main", std::move(conRef), "main", std::move(connection)};
        }

        backup_t make_backup_from(storage_base& other) {
            return {this->get_connection(), "main", other.get_connection(), "main", {}};
        }

        const std::string& filename() const {
            return this->connection->dbArgs.filename;
        }

        /** 
         *  Checks whether connection to database is opened right now.
         *  Returns always `true` for in memory databases.
         *  @attention While retrieving the reference count value is atomic it makes only sense in single-threaded contexts.
         */
        bool is_opened() const {
            connection_ptr maybeConnection = *this->connection;
            return maybeConnection || false;
        }

        /**
         *  Return the name of the VFS object used by the database connection.
         */
        const std::string& vfs_name() const {
            return this->connection->dbArgs.vfs_name;
        }

        /**
         *  Return the current open_mode for this storage object. 
         */
        db_open_mode open_mode() const {
            return this->connection->dbArgs.open_mode;
        }

        /**
         *  Return true if this database object is opened in a readonly state. 
         */
        bool db_readonly() {
            auto connection = this->get_connection();
            return static_cast<bool>(sqlite3_db_readonly(connection.get(), "main"));
        }

        /* 
         *  returning false when there is a transaction in place
         *  otherwise true; function is not const because it has to call get_connection()
         */
        bool get_autocommit() {
            auto connection = this->get_connection();
            return sqlite3_get_autocommit(connection.get());
        }

        int busy_handler(std::function<int(int)> handler) {
            _busy_handler = std::move(handler);
            if (connection_ptr maybeConnection = *this->connection) {
                if (_busy_handler) {
                    return sqlite3_busy_handler(maybeConnection.get(), busy_handler_callback, this);
                } else {
                    return sqlite3_busy_handler(maybeConnection.get(), nullptr, nullptr);
                }
            } else {
                return SQLITE_OK;
            }
        }

      protected:
        storage_base(std::string filename,
                     connection_control connectionCtrl,
                     on_open_spec onOpenSpec,
                     will_run_query_spec willRunQuerySpec,
                     did_run_query_spec didRunQuerySpec,
                     int foreignKeysCount) :
            inMemory{filename.empty() || filename == ":memory:"},
            isOpenedForever{connectionCtrl.open_forever || this->inMemory},
            connection{std::make_unique<connection_holder>(
                this->isOpenedForever,
                db_arguments{std::move(filename), connectionCtrl},
                std::bind(&storage_base::on_open_internal, this, std::placeholders::_1))},
            cachedForeignKeysCount(foreignKeysCount),
            executor{std::move(willRunQuerySpec.willRunQuery), std::move(didRunQuerySpec.didRunQuery)},
            on_open{std::move(onOpenSpec.onOpen)}, pragma(std::bind(&storage_base::get_connection, this), executor),
            limit{std::ref(storage_base::connection)} {
            if (this->isOpenedForever) {
                this->connection->open();
            }
        }

        storage_base(const storage_base& other) :
            inMemory{other.inMemory}, isOpenedForever{other.isOpenedForever},
            connection{std::make_unique<connection_holder>(
                *other.connection,
                std::bind(&storage_base::on_open_internal, this, std::placeholders::_1))},
            cachedForeignKeysCount(other.cachedForeignKeysCount),
            executor{other.executor.will_run_query, other.executor.did_run_query}, on_open(other.on_open),
            pragma(std::bind(&storage_base::get_connection, this), executor),
            limit{std::ref(storage_base::connection)} {
            if (this->isOpenedForever) {
                this->connection->open();
            }
        }

        ~storage_base() {
            if (this->isOpenedForever) {
                this->connection->close();
            }
        }

        connection_ref get_connection() {
            return {*this->connection};
        }

        std::vector<std::string> object_names(string_constant_type type) {
            using data_t = std::vector<std::string>;

            auto connection = this->get_connection();
            data_t objectNames;
            std::stringstream ss;
            ss << "SELECT name FROM sqlite_master WHERE type=" << quote_string_literal(std::string(type)) << std::flush;
            this->executor.perform_exec(
                connection.get(),
                ss.str(),
                [](void* userData, int /*argc*/, orm_gsl::zstring* argv, orm_gsl::zstring* /*columnName*/) -> int {
                    auto& objectNames = *(data_t*)userData;
                    objectNames.emplace_back(argv[0]);
                    return 0;
                },
                &objectNames);
            objectNames.shrink_to_fit();
            return objectNames;
        }

#if SQLITE_VERSION_NUMBER >= 3006019
        void foreign_keys(sqlite3* db, bool value) {
            std::string sql;
            {
                std::stringstream ss;
                ss << "PRAGMA foreign_keys = " << value << std::flush;
                sql = ss.str();
            }
            this->executor.perform_void_exec(db, sql.c_str());
        }

        bool foreign_keys(sqlite3* db) {
            bool result = false;
            this->executor.perform_exec(db, "PRAGMA foreign_keys", extract_single_value<bool>, &result);
            return result;
        }
#endif

        void on_open_internal(sqlite3* db) {
#if SQLITE_VERSION_NUMBER >= 3006019
            if (this->cachedForeignKeysCount) {
                this->foreign_keys(db, true);
            }
#endif

            if (this->pragma.synchronous_ != -1) {
                this->pragma.set_pragma("synchronous", this->pragma.synchronous_, db);
            }

            if (this->pragma.journal_mode_ != -1) {
                this->pragma.set_pragma("journal_mode", static_cast<journal_mode>(this->pragma.journal_mode_), db);
            }

            for (auto& [name, collatingFunction]: this->collatingFunctions) {
                const int rc =
                    sqlite3_create_collation(db, name.c_str(), SQLITE_UTF8, &collatingFunction, collate_callback);
                if (rc != SQLITE_OK) {
                    throw_translated_sqlite_error(rc);
                }
            }

            for (auto [id, value]: this->limit.limits) {
                sqlite3_limit(db, id, value);
            }

            if (_busy_handler) {
                sqlite3_busy_handler(db, busy_handler_callback, this);
            }

            for (auto& udfProxy: this->scalarFunctions) {
                try_to_create_scalar_function(db, udfProxy);
            }

            for (auto& udfProxy: this->aggregateFunctions) {
                try_to_create_aggregate_function(db, udfProxy);
            }

            if (this->on_open) {
                this->on_open(db);
            }
        }

        template<class F>
        void create_scalar_function_impl(udf_holder<F> udfName, std::function<void(void* location)> constructAt) {
            using args_tuple = typename callable_arguments<F>::args_tuple;
            using return_type = typename callable_arguments<F>::return_type;
            constexpr int argsCount =
                std::is_same<args_tuple, std::tuple<arg_values>>::value ? -1 : int(std::tuple_size<args_tuple>::value);
            using is_stateless = std::is_empty<F>;
            auto udfMemorySpace = preallocate_udf_memory<F>();
            if constexpr (is_stateless::value) {
                constructAt(udfMemorySpace.first);
            }
            this->scalarFunctions.emplace_back(
                udfName(),
                argsCount,
                is_stateless::value ? nullptr : std::move(constructAt),
                /* destroy = */
                obtain_xdestroy_for<F>(udf_destruct_only_deleter{}),
                /* call = */
                [](sqlite3_context* context, int nValues, sqlite3_value** values) {
                    auto udfPointer = proxy_get_scalar_udf<F>(is_stateless{}, context, nValues);
                    args_tuple argsTuple = tuple_from_values<args_tuple>{}(values, nValues);
                    auto result = polyfill::apply(*udfPointer, std::move(argsTuple));
                    statement_binder<return_type>().result(context, result);
                },
                udfMemorySpace);

            if (connection_ptr maybeConnection = *this->connection) {
                try_to_create_scalar_function(maybeConnection.get(), this->scalarFunctions.back());
            }
        }

        template<class F>
        void create_aggregate_function_impl(udf_holder<F> udfName, std::function<void(void* location)> constructAt) {
            using args_tuple = typename callable_arguments<F>::args_tuple;
            using return_type = typename callable_arguments<F>::return_type;
            constexpr int argsCount =
                std::is_same<args_tuple, std::tuple<arg_values>>::value ? -1 : int(std::tuple_size<args_tuple>::value);
            this->aggregateFunctions.emplace_back(
                udfName(),
                argsCount,
                std::move(constructAt),
                /* destroy = */
                obtain_xdestroy_for<F>(udf_destruct_only_deleter{}),
                /* step = */
                [](sqlite3_context* context, int nValues, sqlite3_value** values) {
                    F* udfPointer;
                    try {
                        udfPointer = proxy_get_aggregate_step_udf<F>(context, nValues);
                    } catch (const std::bad_alloc&) {
                        sqlite3_result_error_nomem(context);
                        return;
                    }
                    args_tuple argsTuple = tuple_from_values<args_tuple>{}(values, nValues);
#if __cpp_lib_bind_front >= 201907L
                    std::apply(std::bind_front(&F::step, udfPointer), std::move(argsTuple));
#else
                    polyfill::apply(
                        [udfPointer](auto&&... args) {
                            udfPointer->step(std::forward<decltype(args)>(args)...);
                        },
                        std::move(argsTuple));
#endif
                },
                /* finalCall = */
                [](void* udfHandle, sqlite3_context* context) {
                    F& udf = *static_cast<F*>(udfHandle);
                    auto result = udf.fin();
                    statement_binder<return_type>().result(context, result);
                },
                obtain_udf_allocator<F>());

            if (connection_ptr maybeConnection = *this->connection) {
                try_to_create_aggregate_function(maybeConnection.get(), this->aggregateFunctions.back());
            }
        }

        void delete_function_impl(const std::string& name, std::list<udf_proxy>& functions) const {
#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
            auto it = std::ranges::find(functions, name, &udf_proxy::name);
#else
            auto it = std::find_if(functions.begin(), functions.end(), [&name](const udf_proxy& udfProxy) {
                return udfProxy.name == name;
            });
#endif
            if (it == functions.end()) {
                throw std::system_error{orm_error_code::function_not_found};
            }

            if (connection_ptr maybeConnection = *this->connection) {
                const int rc = sqlite3_create_function_v2(maybeConnection.get(),
                                                          name.c_str(),
                                                          it->argumentsCount,
                                                          SQLITE_UTF8,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr);
                if (rc != SQLITE_OK) {
                    throw_translated_sqlite_error(rc);
                }
            }
            it = functions.erase(it);
        }

        static void try_to_create_scalar_function(sqlite3* db, udf_proxy& udfProxy) {
            const int rc = sqlite3_create_function_v2(db,
                                                      udfProxy.name.c_str(),
                                                      udfProxy.argumentsCount,
                                                      SQLITE_UTF8,
                                                      &udfProxy,
                                                      udfProxy.func,
                                                      nullptr,
                                                      nullptr,
                                                      nullptr);
            if (rc != SQLITE_OK) {
                throw_translated_sqlite_error(rc);
            }
        }

        static void try_to_create_aggregate_function(sqlite3* db, udf_proxy& udfProxy) {
            const int rc = sqlite3_create_function(db,
                                                   udfProxy.name.c_str(),
                                                   udfProxy.argumentsCount,
                                                   SQLITE_UTF8,
                                                   &udfProxy,
                                                   nullptr,
                                                   udfProxy.func,
                                                   aggregate_function_final_callback);
            if (rc != SQLITE_OK) {
                throw_translated_sqlite_error(rc);
            }
        }

        std::string current_time(sqlite3* db) {
            std::string result;
            this->executor.perform_exec(db, "SELECT CURRENT_TIME", extract_single_value<std::string>, &result);
            return result;
        }

        std::string current_date(sqlite3* db) {
            std::string result;
            this->executor.perform_exec(db, "SELECT CURRENT_DATE", extract_single_value<std::string>, &result);
            return result;
        }

        std::string current_timestamp(sqlite3* db) {
            std::string result;
            this->executor.perform_exec(db, "SELECT CURRENT_TIMESTAMP", extract_single_value<std::string>, &result);
            return result;
        }

        void drop_table_internal(sqlite3* db, serialize_arg_type tableName, bool ifExists) {
            std::stringstream ss;
            ss << "DROP TABLE";
            if (ifExists) {
                ss << " IF EXISTS";
            }
            ss << ' ' << streaming_identifier(tableName) << std::flush;
            this->executor.perform_void_exec(db, ss.str().c_str());
        }

        void drop_view_internal(sqlite3* db, serialize_arg_type viewName, bool ifExists) {
            std::stringstream ss;
            ss << "DROP VIEW";
            if (ifExists) {
                ss << " IF EXISTS";
            }
            ss << ' ' << streaming_identifier(viewName) << std::flush;
            this->executor.perform_void_exec(db, ss.str().c_str());
        }

        void drop_index_internal(sqlite3* db, serialize_arg_type indexName, bool ifExists) {
            std::stringstream ss;
            ss << "DROP INDEX";
            if (ifExists) {
                ss << " IF EXISTS";
            }
            ss << ' ' << streaming_identifier(indexName) << std::flush;
            this->executor.perform_void_exec(db, ss.str().c_str());
        }

        void drop_trigger_internal(sqlite3* db, serialize_arg_type triggerName, bool ifExists) {
            std::stringstream ss;
            ss << "DROP TRIGGER";
            if (ifExists) {
                ss << " IF EXISTS";
            }
            ss << ' ' << streaming_identifier(triggerName) << std::flush;
            this->executor.perform_void_exec(db, ss.str().c_str());
        }

        bool object_exists(sqlite3* db, serialize_arg_type type, serialize_arg_type name) const {
            bool result = false;
            std::stringstream ss;
            ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = " << quote_string_literal(std::string{type})
               << " AND name = " << quote_string_literal(std::string{name}) << std::flush;
            this->executor.perform_exec(
                db,
                ss.str(),
                [](void* userData, int /*argc*/, orm_gsl::zstring* argv, orm_gsl::zstring* /*azColName*/) -> int {
                    auto& res = *(bool*)userData;
                    res = !!atoi(argv[0]);
                    return 0;
                },
                &result);
            return result;
        }

        std::string retrieve_object_sql(sqlite3* db, serialize_arg_type type, const std::string& name) const {
            std::string result;
            std::stringstream ss;
            ss << "SELECT sql FROM sqlite_master WHERE type = " << quote_string_literal(std::string{type})
               << " AND name = " << quote_string_literal(name) << std::flush;
            this->executor.perform_exec(
                db,
                ss.str(),
                [](void* userData, int /*argc*/, orm_gsl::zstring* argv, orm_gsl::zstring* /*columnName*/) -> int {
                    *static_cast<std::string*>(userData) = argv[0];
                    return 0;
                },
                &result);
            return result;
        }

        static int collate_callback(void* argument, int leftLength, const void* lhs, int rightLength, const void* rhs) {
            auto& function = *(collating_function*)argument;
            return function(leftLength, lhs, rightLength, rhs);
        }

        static int busy_handler_callback(void* selfPointer, int triesCount) {
            auto& storage = *static_cast<storage_base*>(selfPointer);
#ifdef SQLITE_ORM_CONTRACTS_SUPPORTED
            // `sqlite3_busy_handler()` was called properly before so `busy_handler_callback()` will not be called.
            contract_assert(storage._busy_handler);
#endif
            return storage._busy_handler(triesCount);
        }

        bool calculate_remove_add_columns(std::vector<const table_xinfo*>& columnsToAdd,
                                          std::vector<table_xinfo>& storageTableInfo,
                                          std::vector<table_xinfo>& dbTableInfo) const {
            bool notEqual = false;

            //  iterate through storage columns
            for (size_t storageColumnInfoIndex = 0; storageColumnInfoIndex < storageTableInfo.size();
                 ++storageColumnInfoIndex) {

                //  get storage's column info
                table_xinfo& storageColumnInfo = storageTableInfo[storageColumnInfoIndex];
                const std::string& columnName = storageColumnInfo.name;

                //  search for a column in db with the same name
#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
                auto dbColumnInfoIt = std::ranges::find(dbTableInfo, columnName, &table_xinfo::name);
#else
                auto dbColumnInfoIt = std::find_if(dbTableInfo.begin(), dbTableInfo.end(), [&columnName](auto& ti) {
                    return ti.name == columnName;
                });
#endif
                if (dbColumnInfoIt != dbTableInfo.end()) {
                    table_xinfo& dbColumnInfo = *dbColumnInfoIt;
                    bool columnsAreEqual =
                        dbColumnInfo.name == storageColumnInfo.name &&
                        dbColumnInfo.notnull == storageColumnInfo.notnull &&
                        (!dbColumnInfo.dflt_value.empty()) == (!storageColumnInfo.dflt_value.empty()) &&
                        dbColumnInfo.pk == storageColumnInfo.pk &&
                        (dbColumnInfo.hidden == 0) == (storageColumnInfo.hidden == 0);
                    if (!columnsAreEqual) {
                        notEqual = true;
                    } else {
                        dbTableInfo.erase(dbColumnInfoIt);
                        storageTableInfo.erase(storageTableInfo.begin() + storageColumnInfoIndex);
                        --storageColumnInfoIndex;
                    }
                } else {
                    columnsToAdd.push_back(&storageColumnInfo);
                }
            }
            return notEqual;
        }
    };
}
