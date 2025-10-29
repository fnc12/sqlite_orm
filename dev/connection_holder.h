#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <atomic>  // std::atomic_int, memory order flags
#include <mutex>  //  std::mutex, std::lock_guard
#include <thread>  // std::thread::id
#include <utility>  // std::swap, std::exchange
#include <functional>  //  std::function
#include <string>  //  std::string
#endif

#include "functional/cxx_new.h"
#include "functional/cxx_scope_guard.h"
#include "functional/gsl.h"
#include "error_code.h"
#include "vfs_name.h"
#include "db_open_mode.h"
#include "storage_options.h"

namespace sqlite_orm {
    namespace internal {
        struct db_arguments {
            db_arguments(std::string filename, const connection_control& connectionCtrl = {}) :
                filename{std::move(filename)}, vfs_name{connectionCtrl.vfs_name}, open_mode{connectionCtrl.open_mode} {}

            std::string filename;
            std::string vfs_name;
            db_open_mode open_mode;
        };

        /*  
            The connection holder should be performant in all variants:
            1. single-threaded use
            2. opened permanently (open forever)
            3. concurrent open/close
        */
        struct connection_holder {
            explicit connection_holder(bool openedForeverHint,
                                       db_arguments dbArgs,
                                       std::function<void(sqlite3*)> didOpenDb) :
                _control{openedForeverHint}, dbArgs{std::move(dbArgs)}, _didOpenDb{std::move(didOpenDb)} {}

            connection_holder(const connection_holder&) = delete;
            connection_holder& operator=(const connection_holder&) = delete;

            explicit connection_holder(const connection_holder& other, std::function<void(sqlite3*)> didOpenDb) :
                _control{other._control.openedForeverHint}, dbArgs{other.dbArgs}, _didOpenDb{std::move(didOpenDb)} {}

            explicit connection_holder(const connection_holder& other, std::true_type /*openedForeverHint*/) :
                _control{true}, dbArgs{other.dbArgs}, _didOpenDb{other._didOpenDb} {}

            /*  
                Open from a single-threaded context.
             */
            void _do_open() {
                int openFlags = db_open_mode_to_int_flags(this->dbArgs.open_mode);
#if SQLITE_VERSION_NUMBER >= 3037002
                openFlags |= SQLITE_OPEN_EXRESCODE;
#endif

                const int rc = sqlite3_open_v2(this->dbArgs.filename.c_str(),
                                               &_control.db,
                                               openFlags,
                                               this->dbArgs.vfs_name.c_str());

                if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY /*possible, but unexpected*/ {
                    throw_translated_sqlite_error(rc);
                }
            }

            /*  
                Close from a single-threaded context.
             */
            void _do_close() {
                const int rc = sqlite3_close_v2(_control.db);
                if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY {
                    throw_translated_sqlite_error(_control.db);
                } else {
                    _control.db = nullptr;
                }
            }

            /*  
                Open the database once and for all from a single-threaded context when it should be opened permanently.
             */
            void open() {
#ifdef SQLITE_ORM_CONTRACTS_SUPPORTED
                contract_assert(_control.openedForeverHint);
                contract_assert(!_control.db);
#endif
                _control.retainCount.fetch_add(1, std::memory_order_relaxed);
                _do_open();

                if (_didOpenDb) {
                    _didOpenDb(_control.db);
                }
            }

            /*  
                Close the database from a single-threaded context when the database has already been opened permanently.
             */
            void close() {
#ifdef SQLITE_ORM_CONTRACTS_SUPPORTED
                contract_assert(_control.openedForeverHint);
                contract_assert(_control.db);
#endif
                _control.retainCount.fetch_sub(1, std::memory_order_relaxed);
                _do_close();
            }

            sqlite3* retain_if_open() {
                // optional marginal optimization for permanently opened connections;
                if (_control.openedForeverHint) {
#ifdef SQLITE_ORM_CONTRACTS_SUPPORTED
                    contract_assert(_control.db);
#endif
                    return _control.db;
                }

                // required fast path: if connection is already open, just increment counter;
                // it is required otherwise 'retain if open' would be useless;
                // with respect to performance,
                // this can make a difference while a transaction is active where all things happen in memory only;
                // it makes a difference if the `_didOpenDb` callback has a lot of work to do.
                if (int currentCount = _control.retainCount.load(std::memory_order_acquire)) {
                    do {
                        if (_control.retainCount.compare_exchange_weak(currentCount,
                                                                       currentCount + 1,
                                                                       std::memory_order_release,
                                                                       std::memory_order_acquire)) {
                            // successfully incremented, connection is guaranteed to be open
                            return _control.db;
                        }
                        // CAS failed - retry
                    } while (currentCount > 0);
                }
                // test for recursion from the same thread
                else /*currentCount==0*/ {
                    const std::thread::id threadId = _control.initializingThreadId.load(std::memory_order_acquire);
                    if (threadId != std::thread::id{} && std::this_thread::get_id() == threadId)
                        SQLITE_ORM_CPP_UNLIKELY {
                        return _control.db;
                    }
                }

                return nullptr;
            }

            sqlite3* retain() {
                // optional fast path: if connection is already open, just increment counter;
                if (sqlite3* db = retain_if_open()) {
                    return db;
                }

                // slow path: need to open connection or wait for it

                const std::lock_guard _{_sync};

                // double-check: another thread might have opened it
                const bool needsToBeOpened = _control.retainCount == 0;
                if (needsToBeOpened) {
                    _do_open();
                    if (_didOpenDb) {
                        _control.initializingThreadId.store(std::this_thread::get_id(), std::memory_order_release);
                        const scope_guard threadIdGuard{[&threadId = _control.initializingThreadId] {
                            threadId.store(std::thread::id{}, std::memory_order_release);
                        }};
                        // note: may incur recursion in user-provided `on_open` callback
                        _didOpenDb(_control.db);
                    }
                }

                // attention: only increase the reference count after successful open in order to propagate a fully setup connection to other threads
                _control.retainCount.fetch_add(1, std::memory_order_release);
                return _control.db;
            }

            void release() {
                // optional marginal optimization for permanently opened connections;
                if (_control.openedForeverHint) {
#ifdef SQLITE_ORM_CONTRACTS_SUPPORTED
                    contract_assert(_control.db);
#endif
                    return;
                }

                // test for recursion from the same thread;
                // testing against an empty thread id is sufficient because recursion is only possible while calling the `_didOpenDb` callback in `retain()`
                if (_control.initializingThreadId.load(std::memory_order_acquire) != std::thread::id{})
                    SQLITE_ORM_CPP_UNLIKELY {
                    return;
                }

                const int previousCount = _control.retainCount.fetch_sub(1, std::memory_order_release);
                if (previousCount == 1) {
                    // last one closes the connection

                    const std::lock_guard _{_sync};

                    // double-check: another thread might have acquired in the meantime
                    if (_control.retainCount.load(std::memory_order_acquire) == 0) {
                        _do_close();
                    }
                }
            }

            // note: members of the `control_block` are deliberately put on the same cache-line
            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            struct control_block {
                // the optimization gain is very small;
                // at some design point it served as a flag to not use a mutex at all;
                // now it merely saves all the atomic operations, which actually perform without noticeable difference;
                // however it may be kept for conveying logic or future optimizations.
                const bool openedForeverHint = false;
                std::atomic_int retainCount{};
                // `db` synchronizes with `retainCount`
                orm_gsl::owner<sqlite3*> db = nullptr;
                // we don't know what the user-provided `on_open` callback might do, so we need to track recursion;
                std::atomic<std::thread::id> initializingThreadId{};
            } _control;

            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            std::mutex _sync;
            const db_arguments dbArgs;
            const std::function<void(sqlite3* db)> _didOpenDb;
        };

        /*  
            Acquires a database connection upon construction and releases it upon destruction.

            Note: It is important to cache the `sqlite3*` pointer for cache-friendliness (thus avoiding to access the holder on each `get()` call).
         */
        struct connection_ref {
            connection_ref(connection_holder& holder) : holder{&holder}, db{holder.retain()} {}

            connection_ref(connection_ref&& other) : holder{other.holder}, db{this->holder->retain()} {}

            /*
                Rebind connection reference;
                This function is actually unused in the library, but required for concepts compliance (moveable type).
             */
            connection_ref& operator=(connection_ref&& other) noexcept {
                std::swap(this->holder, other.holder);
                std::swap(this->db, other.db);
                return *this;
            }

            ~connection_ref() {
                this->holder->release();
            }

            sqlite3* get() const {
                return this->db;
            }

          private:
            connection_holder* holder;
            sqlite3* db;
        };

        /*  
            Increases the reference count of an existing open connection upon construction and releases it upon destruction.

            Note: It is important to cache the `sqlite3*` pointer for cache-friendliness (thus avoiding to access the holder on each `get()` call).
         */
        struct connection_ptr {
            connection_ptr(connection_holder& holder) : holder{&holder}, db{holder.retain_if_open()} {}

            connection_ptr(connection_ptr&& other) noexcept :
                holder{other.holder}, db{std::exchange(other.db, nullptr)} {}

            /*
                Rebind connection pointer;
             */
            connection_ptr& operator=(connection_ptr&& other) noexcept {
                std::swap(this->holder, other.holder);
                std::swap(this->db, other.db);
                return *this;
            }

            ~connection_ptr() {
                if (this->db) {
                    this->holder->release();
                }
            }

            explicit operator bool() const {
                return this->db || false;
            }

            sqlite3* get() const {
                return this->db;
            }

          private:
            connection_holder* holder;
            sqlite3* db;
        };
    }
}
