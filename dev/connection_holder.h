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
#include "error_code.h"

namespace sqlite_orm {
    namespace internal {

#ifdef SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
        /** 
         *  The connection holder should be performant in all variants:
            1. single-threaded use
            2. opened once (open forever)
            3. concurrent open/close

            Hence, a light-weight binary semaphore is used to synchronize opening and closing a database connection.
        */
        struct connection_holder {
            struct maybe_lock {
                maybe_lock(std::binary_semaphore& sync, bool shouldLock) noexcept(noexcept(sync.acquire())) :
                    isSynced{shouldLock}, sync{sync} {
                    if (shouldLock) {
                        sync.acquire();
                    }
                }

                ~maybe_lock() {
                    if (isSynced) {
                        sync.release();
                    }
                }

                const bool isSynced;
                std::binary_semaphore& sync;
            };

            connection_holder(std::string filename, bool openedForeverHint, std::function<void(sqlite3*)> didOpenDb) :
                _openedForeverHint{openedForeverHint}, _didOpenDb{std::move(didOpenDb)}, filename(std::move(filename)) {
            }

            connection_holder(const connection_holder&) = delete;

            connection_holder(const connection_holder& other, std::function<void(sqlite3*)> didOpenDb) :
                _openedForeverHint{other._openedForeverHint}, _didOpenDb{std::move(didOpenDb)},
                filename{other.filename} {}

            void retain() {
                const maybe_lock maybeLock{_sync, !_openedForeverHint};

                // `maybeLock.isSynced`: the lock above already synchronized everything, so we can just atomically increment the counter
                // `!maybeLock.isSynced`: we presume that the connection is opened once in a single-threaded context [also open forever].
                //                        therefore we can just use an atomic increment but don't need sequencing due to `prevCount > 0`.
                if (int prevCount = _retainCount.fetch_add(1, std::memory_order_relaxed); prevCount > 0) {
                    return;
                }

                // first one opens and sets up the connection.

                if (int rc = sqlite3_open_v2(this->filename.c_str(),
                                             &this->db,
                                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                             nullptr);
                    rc != SQLITE_OK) [[unlikely]] /*possible, but unexpected*/ {
                    throw_translated_sqlite_error(this->db);
                }

                if (_didOpenDb) {
                    _didOpenDb(this->db);
                }
            }

            void release() {
                const maybe_lock maybeLock{_sync, !_openedForeverHint};

                if (int prevCount = _retainCount.fetch_sub(
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

                if (int rc = sqlite3_close_v2(this->db); rc != SQLITE_OK) [[unlikely]] {
                    throw_translated_sqlite_error(this->db);
                } else {
                    this->db = nullptr;
                }
            }

            sqlite3* get() const {
                // note: ensuring a valid DB handle was already memory ordered with `retain()`
                return this->db;
            }

            /** 
             *  @attention While retrieving the reference count value is atomic it makes only sense at single-threaded points in code.
             */
            int retain_count() const {
                return _retainCount.load(std::memory_order_relaxed);
            }

          protected:
            alignas(polyfill::hardware_destructive_interference_size) sqlite3* db = nullptr;

          private:
            std::atomic_int _retainCount{};
            const bool _openedForeverHint = false;
            std::binary_semaphore _sync{1};

          private:
            alignas(polyfill::hardware_destructive_interference_size) const std::function<void(sqlite3* db)> _didOpenDb;

          public:
            const std::string filename;
        };
#else
        struct connection_holder {
            connection_holder(std::string filename,
                              bool /*openedForeverHint*/,
                              std::function<void(sqlite3*)> didOpenDb) :
                _didOpenDb{std::move(didOpenDb)}, filename(std::move(filename)) {}

            connection_holder(const connection_holder&) = delete;

            connection_holder(const connection_holder& other, std::function<void(sqlite3*)> didOpenDb) :
                _didOpenDb{std::move(didOpenDb)}, filename{other.filename} {}

            void retain() {
                // first one opens the connection.
                // we presume that the connection is opened once in a single-threaded context [also open forever].
                // therefore we can just use an atomic increment but don't need sequencing due to `prevCount > 0`.
                if (_retainCount.fetch_add(1, std::memory_order_relaxed) == 0) {
                    int rc = sqlite3_open_v2(this->filename.c_str(),
                                             &this->db,
                                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                             nullptr);
                    if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY /*possible, but unexpected*/ {
                        throw_translated_sqlite_error(this->db);
                    }

                    if (_didOpenDb) {
                        _didOpenDb(this->db);
                    }
                }
            }

            void release() {
                // last one closes the connection.
                // we assume that this might happen by any thread, therefore the counter must serve as a synchronization point.
                if (_retainCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    int rc = sqlite3_close_v2(this->db);
                    if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY {
                        throw_translated_sqlite_error(this->db);
                    } else {
                        this->db = nullptr;
                    }
                }
            }

            sqlite3* get() const {
                // note: ensuring a valid DB handle was already memory ordered with `retain()`
                return this->db;
            }

            /** 
             *  @attention While retrieving the reference count value is atomic it makes only sense at single-threaded points in code.
             */
            int retain_count() const {
                return _retainCount.load(std::memory_order_relaxed);
            }

          protected:
#ifdef SQLITE_ORM_ALIGNED_NEW_SUPPORTED
            alignas(polyfill::hardware_destructive_interference_size)
#endif
                sqlite3* db = nullptr;

          private:
            std::atomic_int _retainCount{};

          private:
#ifdef SQLITE_ORM_ALIGNED_NEW_SUPPORTED
            alignas(polyfill::hardware_destructive_interference_size)
#endif
                const std::function<void(sqlite3* db)> _didOpenDb;

          public:
            const std::string filename;
        };
#endif

        struct connection_ref {
            connection_ref(connection_holder& holder) : holder(&holder) {
                this->holder->retain();
            }

            connection_ref(const connection_ref& other) : holder(other.holder) {
                this->holder->retain();
            }

            // rebind connection reference
            connection_ref& operator=(const connection_ref& other) {
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
