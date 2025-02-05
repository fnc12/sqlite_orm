#pragma once

#include <sqlite3.h>
#include <atomic>
#ifdef SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
#include <semaphore>
#endif
#include <string>  //  std::string

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

            explicit connection_holder(std::string filename) : filename(std::move(filename)) {}

            void retain() {
                const maybe_lock maybeLock{this->_sync, !this->_openedForeverHint};

                // `maybeLock.isSynced`: the lock above already synchronized everything, so we can just atomically increment the counter
                // `!maybeLock.isSynced`: we presume that the connection is opened once in a single-threaded context [open forever].
                //                        so we can just use an atomic increment but don't need sequencing due to always `prevCount > 0`.
                if (int prevCount = this->_retain_count.fetch_add(1, std::memory_order_relaxed); prevCount > 0) {
                    return;
                }

                // first one opens the connection.

                if (int rc = sqlite3_open(this->filename.c_str(), &this->db); rc != SQLITE_OK)
                    [[unlikely]] /*possible, but unexpected*/ {
                    throw_translated_sqlite_error(this->db);
                }
            }

            void release() {
                const maybe_lock maybeLock{this->_sync, !this->_openedForeverHint};

                if (int prevCount = this->_retain_count.fetch_sub(
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

                if (int rc = sqlite3_close(this->db); rc != SQLITE_OK) [[unlikely]] {
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
             *  @attention While retrieving the reference count value is well-defined it makes only sense at single-threaded points in code
             */
            int retain_count() const {
                return this->_retain_count.load(std::memory_order_relaxed);
            }

            const std::string filename;

          protected:
            sqlite3* db = nullptr;

          private:
            std::binary_semaphore _sync{1};
            const bool _openedForeverHint = false;
            std::atomic_int _retain_count{};
        };
#else
        struct connection_holder {
            explicit connection_holder(std::string filename) : filename(std::move(filename)) {}

            void retain() {
                // first one opens the connection.
                // we presume that this the connection is opened once in a single-threaded context [also open forever].
                // so we can just use an atomic increment but don't need sequencing due to `prevCount > 0`.
                if (this->_retain_count.fetch_add(1, std::memory_order_relaxed) == 0) {
                    int rc = sqlite3_open(this->filename.c_str(), &this->db);
                    if (rc != SQLITE_OK) {
                        throw_translated_sqlite_error(this->db);
                    }
                }
            }

            void release() {
                // last one closes the connection.
                // we assume that this might happen by any thread.
                if (this->_retain_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    int rc = sqlite3_close(this->db);
                    if (rc != SQLITE_OK) {
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

            int retain_count() const {
                return this->_retain_count.load(std::memory_order_relaxed);
            }

            const std::string filename;

          protected:
            sqlite3* db = nullptr;

          private:
            std::atomic_int _retain_count{};
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
