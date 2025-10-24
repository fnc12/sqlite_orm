#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <atomic>
#ifdef SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
#include <semaphore>
#endif
#include <functional>  //  std::function
#include <string>  //  std::string
#endif

#include "functional/cxx_new.h"
#include "functional/gsl.h"
#include "error_code.h"
#include "vfs_name.h"
#include "db_open_mode.h"
#include "storage_options.h"

namespace sqlite_orm {
    namespace internal {
        struct db_arguments {
            const std::string filename;
            const std::string vfs_name;
            const db_open_mode open_mode;
        };

#ifdef SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
        /*  
            The connection holder should be performant in all variants:
            1. single-threaded use
            2. opened permanently (open forever)
            3. concurrent open/close

            Hence, a light-weight binary semaphore is used to synchronize opening and closing a database connection.
        */
        struct connection_holder {
            struct maybe_lock {
                maybe_lock(std::binary_semaphore& sync, bool shouldLock) noexcept(noexcept(sync.acquire())) :
                    isSynced{shouldLock}, sync{sync} {
                    if (isSynced) {
                        if (nRecursionsPerThread++ == 0) [[likely]] {
                            sync.acquire();
                        }
                    }
                }

                ~maybe_lock() {
                    if (isSynced) {
                        if (--nRecursionsPerThread == 0) [[likely]] {
                            sync.release();
                        }
                    }
                }

                const bool isSynced;
                std::binary_semaphore& sync;

                // guard against recursive locking from the same thread in `on_open` callbacks
                inline static thread_local int nRecursionsPerThread = 0;
            };

            connection_holder(std::string filename,
                              std::function<void(sqlite3*)> didOpenDb,
                              const connection_control& options) :
                _control{.openedForeverHint = options.open_forever},
                dbArgs{.filename = std::move(filename), .vfs_name = options.vfs_name, .open_mode = options.open_mode},
                _didOpenDb{std::move(didOpenDb)} {}

            connection_holder(const connection_holder&) = delete;

            connection_holder(const connection_holder& other, std::function<void(sqlite3*)> didOpenDb) :
                _control{.openedForeverHint = other._control.openedForeverHint}, dbArgs{other.dbArgs},
                _didOpenDb{std::move(didOpenDb)} {}

            void retain() {
                const maybe_lock maybeLock{_control.sync, !_control.openedForeverHint};

                // `maybeLock.isSynced`: the lock above already synchronized everything, so we can just atomically increment the counter
                // `!maybeLock.isSynced`: we presume that the connection is opened once in a single-threaded context [also open forever].
                //                        therefore we can just use an atomic increment but don't need sequencing due to `prevCount > 0`.
                if (int prevCount = _control.retainCount.fetch_add(1, std::memory_order_relaxed); prevCount > 0) {
                    return;
                }

                // first one opens and sets up the connection.

                int openFlags = internal::db_open_mode_to_int_flags(this->dbArgs.open_mode);
#if SQLITE_VERSION_NUMBER >= 3037002
                openFlags |= SQLITE_OPEN_EXRESCODE;
#endif

                if (int rc = sqlite3_open_v2(this->dbArgs.filename.c_str(),
                                             &_control.db,
                                             openFlags,
                                             this->dbArgs.vfs_name.c_str());
                    rc != SQLITE_OK) [[unlikely]] /*possible, but unexpected*/ {
                    throw_translated_sqlite_error(_control.db);
                }

                if (_didOpenDb) {
                    _didOpenDb(_control.db);
                }
            }

            void release() {
                const maybe_lock maybeLock{_control.sync, !_control.openedForeverHint};

                if (int prevCount = _control.retainCount.fetch_sub(
                        1,
                        maybeLock.isSynced
                            // the lock above already synchronized everything, so we can just atomically decrement the counter
                            ? std::memory_order_relaxed
                            // the counter must serve as a synchronization point
                            : std::memory_order_acq_rel);
                    prevCount > 1) {
                    return;
                }

                // last one closes the connection.

                if (int rc = sqlite3_close_v2(_control.db); rc != SQLITE_OK) [[unlikely]] {
                    throw_translated_sqlite_error(_control.db);
                } else {
                    _control.db = nullptr;
                }
            }

            sqlite3* get() const {
                // note: ensuring a valid DB handle was already memory ordered with `retain()`
                return _control.db;
            }

            // attention: must only ever be called from a single-threaded context
            void propagate_open_forever_hint() {
                _control.openedForeverHint = true;
            }

            /** 
             *  @note While retrieving the reference count value is atomic it makes only sense in single-threaded contexts.
             */
            int retain_count() const {
                return _control.retainCount.load(std::memory_order_relaxed);
            }

            // note: members of the `control_block` are deliberately put on the same cache-line
            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            struct control_block {
                orm_gsl::owner<sqlite3*> db = nullptr;
                std::atomic_int retainCount{};
                bool openedForeverHint = false;
                std::binary_semaphore sync{1};
            } _control;

            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            db_arguments dbArgs;
            const std::function<void(sqlite3* db)> _didOpenDb;
        };
#else
        struct connection_holder {
            connection_holder(std::string filename,
                              std::function<void(sqlite3*)> didOpenDb,
                              const connection_control& options) :
                _control{}, dbArgs{std::move(filename), options.vfs_name, options.open_mode},
                _didOpenDb{std::move(didOpenDb)} {}

            connection_holder(const connection_holder&) = delete;

            connection_holder(const connection_holder& other, std::function<void(sqlite3*)> didOpenDb) :
                _control{}, dbArgs{other.dbArgs}, _didOpenDb{std::move(didOpenDb)} {}

            void retain() {
                // first one opens the connection.
                // we presume that the connection is opened once in a single-threaded context [also open forever].
                // therefore we can just use an atomic increment but don't need sequencing due to `prevCount > 0`.
                if (_control.retainCount.fetch_add(1, std::memory_order_relaxed) == 0) {
                    int openFlags = internal::db_open_mode_to_int_flags(this->dbArgs.open_mode);
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

                    if (_didOpenDb) {
                        _didOpenDb(_control.db);
                    }
                }
            }

            void release() {
                // last one closes the connection.
                // we assume that this might happen by any thread, therefore the counter must serve as a synchronization point.
                if (_control.retainCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    int rc = sqlite3_close_v2(_control.db);
                    if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY {
                        throw_translated_sqlite_error(_control.db);
                    } else {
                        _control.db = nullptr;
                    }
                }
            }

            sqlite3* get() const {
                // note: ensuring a valid DB handle was already memory ordered with `retain()`
                return _control.db;
            }

            void propagate_open_forever_hint() {}

            /** 
             *  @attention While retrieving the reference count value is atomic it makes only sense at single-threaded points in code.
             */
            int retain_count() const {
                return _control.retainCount.load(std::memory_order_relaxed);
            }

            // note: members of the `control_block` are deliberately put on the same cache-line
            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            struct control_block {
                orm_gsl::owner<sqlite3*> db = nullptr;
                std::atomic_int retainCount{};
            } _control;

            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            db_arguments dbArgs;
            const std::function<void(sqlite3* db)> _didOpenDb;
        };
#endif

        struct connection_ref {
            connection_ref(connection_holder& holder) : holder(&holder) {
                this->holder->retain();
            }

            connection_ref(const connection_ref& other) noexcept : holder(other.holder) {
                this->holder->retain();
            }

            // rebind connection reference;
            // this function is actually unused in the library, but required for concepts compliance (moveable type).
            // Unfortunately it is not `noexcept` because of the `release()` call.
            connection_ref& operator=(const connection_ref& other) & {
                if (other.holder != this->holder) {
                    this->holder->release();
                    this->holder = other.holder;
                    this->holder->retain();
                }
                return *this;
            }

            ~connection_ref() {
                this->holder->release();
            }

            sqlite3* get() const {
                return this->holder->get();
            }

          private:
            connection_holder* holder = nullptr;
        };
    }
}
