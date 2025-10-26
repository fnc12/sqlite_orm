#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <atomic>  // memory order flags
#ifdef SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
#include <semaphore>
#else
#include <mutex>
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

        /*  
            The connection holder should be performant in all variants:
            1. single-threaded use
            2. opened permanently (open forever)
            3. concurrent open/close

            Hence, a light-weight binary semaphore is used to synchronize opening and closing a database connection.
        */
        struct connection_holder {
#ifdef SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
            struct maybe_lock {
                explicit maybe_lock(std::binary_semaphore& sync) : sync{sync} {
                    if (nRecursionsPerThread++ == 0) [[unlikely]] {
                        sync.acquire();
                    }
                }

                ~maybe_lock() {
                    if (--nRecursionsPerThread == 0) [[unlikely]] {
                        sync.release();
                    }
                }

                std::binary_semaphore& sync;

                // guard against recursive locking from the same thread in `on_open` callbacks
                inline static thread_local int nRecursionsPerThread = 0;
            };
#else
            struct maybe_lock {
                explicit maybe_lock(std::mutex& sync) : sync{sync} {
                    if (nRecursionsPerThread++ == 0) SQLITE_ORM_CPP_UNLIKELY {
                        sync.lock();
                    }
                }

                ~maybe_lock() {
                    if (--nRecursionsPerThread == 0) SQLITE_ORM_CPP_UNLIKELY {
                        sync.unlock();
                    }
                }

                std::mutex& sync;

                // guard against recursive locking from the same thread in `on_open` callbacks
                inline static thread_local int nRecursionsPerThread = 0;
            };
#endif

            explicit connection_holder(std::string filename,
                                       std::function<void(sqlite3*)> didOpenDb,
                                       const connection_control& options) :
                _control{options.open_forever}, dbArgs{std::move(filename), options.vfs_name, options.open_mode},
                _didOpenDb{std::move(didOpenDb)} {}

            connection_holder(const connection_holder&) = delete;
            connection_holder& operator=(const connection_holder&) = delete;

            explicit connection_holder(const connection_holder& other, std::function<void(sqlite3*)> didOpenDb) :
                _control{other._control.openedForeverHint}, dbArgs{other.dbArgs}, _didOpenDb{std::move(didOpenDb)} {}

            explicit connection_holder(const connection_holder& other, std::true_type /*openForever*/) :
                _control{true}, dbArgs{other.dbArgs}, _didOpenDb{other._didOpenDb} {}

            sqlite3* open() {
                // we can presume that this method gets called under a lock or in a single-threaded context (due to `openedForeverHint==true`)
                if (_control.retainCount++ > 0) {
                    return _control.db;
                }

                // first one opens and sets up the connection.

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

                if (_didOpenDb) {
                    _didOpenDb(_control.db);
                }

                return _control.db;
            }

            void close() {
                // we can presume that this method gets called under a lock or in a single-threaded context (due to `openedForeverHint==true`)
                if (--_control.retainCount > 0) {
                    return;
                }

                // last one closes the connection.

                const int rc = sqlite3_close_v2(_control.db);
                if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY {
                    throw_translated_sqlite_error(_control.db);
                } else {
                    _control.db = nullptr;
                }
            }

            sqlite3* retain() {
                if (_control.openedForeverHint) {
                    return _control.db;
                }

                const maybe_lock _{_control.sync};
                return open();
            }

            void release() {
                if (_control.openedForeverHint) {
                    return;
                }

                const maybe_lock _{_control.sync};
                close();
            }

            sqlite3* get() const {
                // note: ensuring a valid DB handle was already memory ordered with `retain()`
                return _control.db;
            }

            /** 
             *  @attention While retrieving the reference count value is atomic it makes only sense at single-threaded points in code.
             */
            int retain_count() const {
                return _control.retainCount;
            }

            // note: members of the `control_block` are deliberately put on the same cache-line
            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            struct control_block {
                const bool openedForeverHint = false;
                orm_gsl::owner<sqlite3*> db = nullptr;
                int retainCount = 0;
#ifdef SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
                std::binary_semaphore sync{1};
#else
                std::mutex sync;
#endif
            } _control;

            SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(alignas(polyfill::hardware_destructive_interference_size))
            db_arguments dbArgs;
            const std::function<void(sqlite3* db)> _didOpenDb;
        };

        struct connection_ref {
            connection_ref(connection_holder& holder) : holder{&holder}, db{holder.retain()} {}

            connection_ref(connection_ref&& other) : holder{other.holder}, db{this->holder->retain()} {}

            /*
                Rebind connection reference;
                This function is actually unused in the library, but required for concepts compliance (moveable type).
                Unfortunately it is not `noexcept` because of the `release()` call.
             */
            connection_ref& operator=(connection_ref&& other) {
                this->holder->release();
                this->holder = other.holder;
                this->db = other.db;
                this->holder->retain();
            }

            ~connection_ref() {
                this->holder->release();
            }

            sqlite3* get() const {
                return this->db;
            }

          private:
            connection_holder* holder;
            sqlite3* db = nullptr;
        };
    }
}
