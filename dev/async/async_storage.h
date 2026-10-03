#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <coroutine>  //  std::coroutine_handle
#include <cstddef>  //  std::size_t
#include <deque>  //  std::deque
#include <functional>  //  std::function
#include <memory>  //  std::unique_ptr, std::make_unique
#include <stdexcept>  //  std::runtime_error
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <tuple>  //  std::tuple, std::make_tuple, std::apply
#include <type_traits>  //  std::invoke_result_t, std::is_void_v, std::is_same_v, std::decay_t
#include <utility>  //  std::forward, std::move
#include <vector>  //  std::vector
#endif

#include "../storage.h"
#include "async_vfs.h"
#include "io_context.h"
#include "task.h"

namespace sqlite_orm::internal {

    /**
     *  Packs `args` into a callable `function(storage)` that invokes
     *  `call(storage, args...)` with the arguments as rvalues. A tuple is used
     *  instead of a pack init-capture on purpose: GCC 11 initializes pack
     *  init-captures bytewise, which breaks captured strings in their
     *  small-buffer state.
     */
    template<class Call, class... Args>
    auto bind_arguments(Call call, Args&&... args) {
        return
            [call = std::move(call), arguments = std::make_tuple(std::forward<Args>(args)...)](auto& storage) mutable {
                return std::apply(
                    [&call, &storage](auto&... unpacked) {
                        return call(storage, std::move(unpacked)...);
                    },
                    arguments);
            };
    }

    /**
     *  Coroutine-aware mutex: at most one operation uses the storage at a time,
     *  the others wait in order without blocking the thread.
     */
    struct storage_turnstile {
        struct waiter {
            std::coroutine_handle<> handle;
        };

        bool busy = false;
        std::deque<waiter*> waiters;
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Asynchronous front-end over an ordinary storage. Every operation runs the
     *  unchanged synchronous storage code on a fiber, where the file I/O
     *  underneath goes through io_uring, and is awaited with `co_await`.
     *
     *  One storage is one connection, like a network connection in an
     *  asynchronous client: requests do not block the thread, but they are
     *  executed one after another. Operations issued from several coroutines are
     *  queued in order.
     *
     *      io_context io;
     *      auto storage = make_async_storage(io, "app.db",
     *                                        make_table("users", make_column("id", &User::id, primary_key()), ...));
     *
     *      auto users = co_await storage.get_all<User>(where(c(&User::age) > 30));
     *      auto user  = co_await storage.get<User>(42);
     *      co_await storage.transaction([&user](auto& storage) { storage.update(user); return true; });
     *
     *  The asynchronous VFS is injected through connection_control, nothing
     *  global is changed. All coroutines using one async_storage run on the
     *  io_context it was created with.
     */
    template<class Storage>
    class async_storage {
      public:
        using storage_type = Storage;

        async_storage(io_context& context_, Storage storage_) : context(&context_), storage(std::move(storage_)) {}

        async_storage(async_storage&&) noexcept = default;
        async_storage& operator=(async_storage&&) noexcept = default;
        async_storage(const async_storage&) = delete;
        async_storage& operator=(const async_storage&) = delete;

        io_context& get_io_context() noexcept {
            return *this->context;
        }

        /**
         *  True while an operation is executing or queued.
         */
        bool is_busy() const noexcept {
            return this->turnstile.busy;
        }

        /**
         *  The primitive everything else is built on: run `function(storage)` on a
         *  fiber, exclusively, and return its result. Use it for anything not
         *  mirrored below (pragmas, limits, user-defined functions, iteration).
         */
        template<class F>
        auto run(F function) {
            //  The callable is moved to the heap before entering the coroutine: a
            //  closure passed as a coroutine parameter is not copied correctly by
            //  some compilers (GCC 11 relocates it bytewise, which breaks captured
            //  strings in their small-buffer state).
            return this->run_on_fiber(std::make_unique<F>(std::move(function)));
        }

        //  ---- transactions ----

        /**
         *  `function(storage)` returns true to commit, false to roll back. An
         *  exception rolls back and propagates.
         */
        template<class F>
        task<bool> transaction(F function) {
            return this->run([function = std::move(function)](Storage& storage) mutable {
                return storage.transaction([&function, &storage] {
                    return function(storage);
                });
            });
        }

        template<class F>
        task<bool> savepoint(std::string savepointName, F function) {
            return this->run(
                [savepointName = std::move(savepointName), function = std::move(function)](Storage& storage) mutable {
                    return storage.savepoint(savepointName, [&function, &storage] {
                        return function(storage);
                    });
                });
        }

        //  ---- CRUD, mirroring storage_t. Arguments are moved into the operation. ----

        template<class O, class R = std::vector<O>, class... Args>
        auto get_all(Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... args) {
                    return storage.template get_all<O, R>(std::move(args)...);
                },
                std::forward<Args>(args)...));
        }

        template<class O, class R = std::vector<std::unique_ptr<O>>, class... Args>
        auto get_all_pointer(Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... args) {
                    return storage.template get_all_pointer<O, R>(std::move(args)...);
                },
                std::forward<Args>(args)...));
        }

        template<class O, class R = std::vector<std::optional<O>>, class... Args>
        auto get_all_optional(Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... args) {
                    return storage.template get_all_optional<O, R>(std::move(args)...);
                },
                std::forward<Args>(args)...));
        }

        template<class O, class... Ids>
        auto get(Ids&&... ids) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... ids) {
                    return storage.template get<O>(std::move(ids)...);
                },
                std::forward<Ids>(ids)...));
        }

        template<class O, class... Ids>
        auto get_pointer(Ids&&... ids) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... ids) {
                    return storage.template get_pointer<O>(std::move(ids)...);
                },
                std::forward<Ids>(ids)...));
        }

        template<class O, class... Ids>
        auto get_optional(Ids&&... ids) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... ids) {
                    return storage.template get_optional<O>(std::move(ids)...);
                },
                std::forward<Ids>(ids)...));
        }

        /**
         *  Both `insert(object)` and the raw `insert(into<T>(), columns(...), values(...))`.
         */
        template<class... Args>
        auto insert(Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... args) {
                    return storage.insert(std::move(args)...);
                },
                std::forward<Args>(args)...));
        }

        template<class It, class Projection = polyfill::identity>
        task<void> insert_range(It from, It to, Projection project = {}) {
            return this->run([from, to, project = std::move(project)](Storage& storage) mutable {
                storage.insert_range(from, to, std::move(project));
            });
        }

        template<class... Args>
        auto replace(Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... args) {
                    return storage.replace(std::move(args)...);
                },
                std::forward<Args>(args)...));
        }

        template<class It, class Projection = polyfill::identity>
        task<void> replace_range(It from, It to, Projection project = {}) {
            return this->run([from, to, project = std::move(project)](Storage& storage) mutable {
                storage.replace_range(from, to, std::move(project));
            });
        }

        template<class O>
        task<void> update(O&& object) {
            return this->run([object = std::forward<O>(object)](Storage& storage) {
                storage.update(object);
            });
        }

        template<class S, class... Wargs>
        task<void> update_all(S set, Wargs&&... conditions) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& set, auto&&... conditions) {
                    storage.update_all(std::move(set), std::move(conditions)...);
                },
                std::move(set),
                std::forward<Wargs>(conditions)...));
        }

        template<class O, class... Ids>
        task<void> remove(Ids&&... ids) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... ids) {
                    storage.template remove<O>(std::move(ids)...);
                },
                std::forward<Ids>(ids)...));
        }

        template<class O, class... Args>
        task<void> remove_all(Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... args) {
                    storage.template remove_all<O>(std::move(args)...);
                },
                std::forward<Args>(args)...));
        }

        //  ---- aggregates ----

        template<class O, class... Args>
        task<int> count(Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&&... args) {
                    return storage.template count<O>(std::move(args)...);
                },
                std::forward<Args>(args)...));
        }

        template<class F, class... Args>
        task<int> count(F field, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& field, auto&&... args) {
                    return storage.count(std::move(field), std::move(args)...);
                },
                std::move(field),
                std::forward<Args>(args)...));
        }

        template<class F, class... Args>
        task<double> avg(F field, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& field, auto&&... args) {
                    return storage.avg(std::move(field), std::move(args)...);
                },
                std::move(field),
                std::forward<Args>(args)...));
        }

        template<class F, class... Args>
        auto sum(F field, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& field, auto&&... args) {
                    return storage.sum(std::move(field), std::move(args)...);
                },
                std::move(field),
                std::forward<Args>(args)...));
        }

        template<class F, class... Args>
        task<double> total(F field, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& field, auto&&... args) {
                    return storage.total(std::move(field), std::move(args)...);
                },
                std::move(field),
                std::forward<Args>(args)...));
        }

        template<class F, class... Args>
        auto max(F field, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& field, auto&&... args) {
                    return storage.max(std::move(field), std::move(args)...);
                },
                std::move(field),
                std::forward<Args>(args)...));
        }

        template<class F, class... Args>
        auto min(F field, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& field, auto&&... args) {
                    return storage.min(std::move(field), std::move(args)...);
                },
                std::move(field),
                std::forward<Args>(args)...));
        }

        template<class F, class... Args>
        task<std::string> group_concat(F field, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& field, auto&&... args) {
                    return storage.group_concat(std::move(field), std::move(args)...);
                },
                std::move(field),
                std::forward<Args>(args)...));
        }

        //  ---- queries ----

        template<class T, class... Args>
        auto select(T expression, Args&&... args) {
            return this->run(internal::bind_arguments(
                [](Storage& storage, auto&& expression, auto&&... args) {
                    return storage.select(std::move(expression), std::move(args)...);
                },
                std::move(expression),
                std::forward<Args>(args)...));
        }

        template<class CTE, class E>
        auto with(CTE cte, E expression) {
            return this->run([cte = std::move(cte), expression = std::move(expression)](Storage& storage) mutable {
                return storage.with(std::move(cte), std::move(expression));
            });
        }

        template<class CTE, class E>
        auto with_recursive(CTE cte, E expression) {
            return this->run([cte = std::move(cte), expression = std::move(expression)](Storage& storage) mutable {
                return storage.with_recursive(std::move(cte), std::move(expression));
            });
        }

        //  ---- prepared statements ----

        /**
         *  Prepared statements belong to this storage's connection and are executed
         *  on it: `auto statement = co_await storage.prepare(select(...)); co_await storage.execute(statement);`.
         *  Binding parameters with `get<N>(statement) = value` needs no await.
         */
        template<class T>
        auto prepare(T statement) {
            return this->run([statement = std::move(statement)](Storage& storage) mutable {
                return storage.prepare(std::move(statement));
            });
        }

        /**
         *  `statement` must outlive the awaited operation.
         */
        template<class T>
        auto execute(const T& statement) {
            return this->run([&statement](Storage& storage) {
                return storage.execute(statement);
            });
        }

        //  ---- schema ----

        auto sync_schema(bool preserve = false) {
            return this->run([preserve](Storage& storage) {
                return storage.sync_schema(preserve);
            });
        }

        auto sync_schema_simulate(bool preserve = false) {
            return this->run([preserve](Storage& storage) {
                return storage.sync_schema_simulate(preserve);
            });
        }

        task<bool> table_exists(std::string tableName) {
            return this->run([tableName = std::move(tableName)](Storage& storage) {
                return storage.table_exists(tableName);
            });
        }

        task<bool> view_exists(std::string viewName) {
            return this->run([viewName = std::move(viewName)](Storage& storage) {
                return storage.view_exists(viewName);
            });
        }

        task<void> drop_table(std::string tableName) {
            return this->run([tableName = std::move(tableName)](Storage& storage) {
                storage.drop_table(tableName);
            });
        }

        task<void> drop_table_if_exists(std::string tableName) {
            return this->run([tableName = std::move(tableName)](Storage& storage) {
                storage.drop_table_if_exists(tableName);
            });
        }

        template<class O>
        task<void> rename_table(std::string name) {
            return this->run([name = std::move(name)](Storage& storage) mutable {
                storage.template rename_table<O>(std::move(name));
            });
        }

        task<void> rename_table(std::string oldName, std::string newName) {
            return this->run([oldName = std::move(oldName), newName = std::move(newName)](Storage& storage) {
                storage.rename_table(oldName, newName);
            });
        }

        task<void> drop_index(std::string indexName) {
            return this->run([indexName = std::move(indexName)](Storage& storage) {
                storage.drop_index(indexName);
            });
        }

        task<void> drop_index_if_exists(std::string indexName) {
            return this->run([indexName = std::move(indexName)](Storage& storage) {
                storage.drop_index_if_exists(indexName);
            });
        }

        task<void> drop_view(std::string viewName) {
            return this->run([viewName = std::move(viewName)](Storage& storage) {
                storage.drop_view(viewName);
            });
        }

        task<void> drop_view_if_exists(std::string viewName) {
            return this->run([viewName = std::move(viewName)](Storage& storage) {
                storage.drop_view_if_exists(viewName);
            });
        }

        task<void> drop_trigger(std::string triggerName) {
            return this->run([triggerName = std::move(triggerName)](Storage& storage) {
                storage.drop_trigger(triggerName);
            });
        }

        task<void> drop_trigger_if_exists(std::string triggerName) {
            return this->run([triggerName = std::move(triggerName)](Storage& storage) {
                storage.drop_trigger_if_exists(triggerName);
            });
        }

        task<std::vector<std::string>> table_names() {
            return this->run([](Storage& storage) {
                return storage.table_names();
            });
        }

        task<std::vector<std::string>> view_names() {
            return this->run([](Storage& storage) {
                return storage.view_names();
            });
        }

        task<std::vector<std::string>> trigger_names() {
            return this->run([](Storage& storage) {
                return storage.trigger_names();
            });
        }

        task<void> vacuum() {
            return this->run([](Storage& storage) {
                storage.vacuum();
            });
        }

        task<void> analyze() {
            return this->run([](Storage& storage) {
                storage.analyze();
            });
        }

        task<void> analyze(std::string name) {
            return this->run([name = std::move(name)](Storage& storage) {
                storage.analyze(name);
            });
        }

        //  ---- connection information ----

        task<std::string> current_time() {
            return this->run([](Storage& storage) {
                return storage.current_time();
            });
        }

        task<std::string> current_date() {
            return this->run([](Storage& storage) {
                return storage.current_date();
            });
        }

        task<std::string> current_timestamp() {
            return this->run([](Storage& storage) {
                return storage.current_timestamp();
            });
        }

        task<int> busy_timeout(int milliseconds) {
            return this->run([milliseconds](Storage& storage) {
                return storage.busy_timeout(milliseconds);
            });
        }

        task<int64> last_insert_rowid() {
            return this->run([](Storage& storage) {
                return storage.last_insert_rowid();
            });
        }

        task<int> changes() {
            return this->run([](Storage& storage) {
                return storage.changes();
            });
        }

        task<int> total_changes() {
            return this->run([](Storage& storage) {
                return storage.total_changes();
            });
        }

        task<int64> changes64() {
            return this->run([](Storage& storage) {
                return storage.changes64();
            });
        }

        task<int64> total_changes64() {
            return this->run([](Storage& storage) {
                return storage.total_changes64();
            });
        }

        task<bool> db_readonly() {
            return this->run([](Storage& storage) {
                return storage.db_readonly();
            });
        }

        task<bool> get_autocommit() {
            return this->run([](Storage& storage) {
                return storage.get_autocommit();
            });
        }

        auto txn_state() {
            return this->run([](Storage& storage) {
                return storage.txn_state();
            });
        }

        auto db_name(int index) {
            return this->run([index](Storage& storage) {
                return storage.db_name(index);
            });
        }

        task<void> backup_to(std::string fileName) {
            return this->run([fileName = std::move(fileName)](Storage& storage) {
                storage.backup_to(fileName);
            });
        }

        task<void> backup_from(std::string fileName) {
            return this->run([fileName = std::move(fileName)](Storage& storage) {
                storage.backup_from(fileName);
            });
        }

        //  ---- no I/O: available directly ----

        const std::string& filename() const {
            return this->storage.filename();
        }

        const std::string& vfs_name() const {
            return this->storage.vfs_name();
        }

        db_open_mode open_mode() const {
            return this->storage.open_mode();
        }

        bool is_opened() const {
            return this->storage.is_opened();
        }

        template<class... Args>
        std::string dump(Args&&... args) const {
            return this->storage.dump(std::forward<Args>(args)...);
        }

        /**
         *  Thread-safe: abort the statement currently executing, if any.
         */
        void interrupt() {
            this->storage.interrupt();
        }

        std::string libversion() {
            return this->storage.libversion();
        }

      private:
        template<class F>
        auto run_on_fiber(std::unique_ptr<F> function) -> task<std::invoke_result_t<F&, Storage&>> {
            using result_type = std::invoke_result_t<F&, Storage&>;
            co_await this->acquire();
            release_guard guard{this};
            Storage& storage = this->storage;
            F& callable = *function;
            if constexpr (std::is_void_v<result_type>) {
                co_await this->context->async([&storage, &callable] {
                    storage.open_forever();  //  no-op after the first call
                    callable(storage);
                });
            } else {
                co_return co_await this->context->async([&storage, &callable] {
                    storage.open_forever();
                    return callable(storage);
                });
            }
        }

        struct acquire_awaitable {
            async_storage* self;
            internal::storage_turnstile::waiter waiter;

            bool await_ready() noexcept {
                if (this->self->turnstile.busy) {
                    return false;
                }
                this->self->turnstile.busy = true;
                return true;
            }

            void await_suspend(std::coroutine_handle<> handle) {
                this->waiter.handle = handle;
                this->self->turnstile.waiters.push_back(&this->waiter);
            }

            void await_resume() noexcept {}
        };

        acquire_awaitable acquire() {
            return {this, {}};
        }

        /**
         *  The turn is handed to the first waiter directly, so a newcomer cannot
         *  overtake it between release and resumption.
         */
        void release() {
            if (this->turnstile.waiters.empty()) {
                this->turnstile.busy = false;
                return;
            }
            internal::storage_turnstile::waiter* next = this->turnstile.waiters.front();
            this->turnstile.waiters.pop_front();
            this->context->schedule(next->handle);
        }

        struct release_guard {
            async_storage* self;

            ~release_guard() {
                this->self->release();
            }
        };

        io_context* context;
        Storage storage;
        internal::storage_turnstile turnstile;
    };
}

namespace sqlite_orm::internal {

    /**
     *  Force the asynchronous VFS and lazy opening on a user-supplied connection_control.
     */
    inline connection_control async_connection_control(connection_control control) {
        control.vfs_name = std::string(async_vfs_name);
        control.open_forever = false;  //  opened on first use, on a fiber
        return control;
    }

    template<class A>
    A&& async_connection_control(A&& argument) {
        return std::forward<A>(argument);
    }

    inline void ensure_async_vfs() {
        if (async_vfs::register_vfs() != SQLITE_OK) {
            throw std::runtime_error("sqlite_orm async: cannot register the VFS");
        }
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Same arguments as make_storage, io_context first. The asynchronous VFS is
     *  injected automatically; a connection_control among the arguments keeps its
     *  other fields.
     *
     *      auto storage = make_async_storage(io, "app.db", make_table(...));
     */
    template<class... Args>
    auto make_async_storage(io_context& context, std::string filename, Args&&... args) {
        internal::ensure_async_vfs();
        constexpr bool hasConnectionControl = (std::is_same_v<std::decay_t<Args>, connection_control> || ...);
        if constexpr (hasConnectionControl) {
            auto storage =
                make_storage(std::move(filename), internal::async_connection_control(std::forward<Args>(args))...);
            return async_storage<decltype(storage)>(context, std::move(storage));
        } else {
            auto storage = make_storage(std::move(filename),
                                        internal::async_connection_control(connection_control{}),
                                        std::forward<Args>(args)...);
            return async_storage<decltype(storage)>(context, std::move(storage));
        }
    }
}
