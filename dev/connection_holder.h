#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <atomic>
#include <functional>  //  std::function
#include <string>  //  std::string
#endif

#include "error_code.h"
#include "vfs_name.h"
#include "db_open_mode.h"
#include "storage_options.h"

namespace sqlite_orm {
    namespace internal {

        struct connection_holder {
            connection_holder(std::string filename,
                              std::function<void(sqlite3*)> didOpenDb,
                              connection_control options = {}) :
                _didOpenDb{std::move(didOpenDb)}, filename(std::move(filename)), vfs_name(options.vfs_name),
                open_mode(options.open_mode) {}

            connection_holder(const connection_holder&) = delete;

            connection_holder(const connection_holder& other, std::function<void(sqlite3*)> didOpenDb) :
                _didOpenDb{std::move(didOpenDb)}, filename{other.filename}, vfs_name(other.vfs_name),
                open_mode{other.open_mode} {}

            void retain() {
                // first one opens the connection.
                // we presume that the connection is opened once in a single-threaded context [also open forever].
                // therefore we can just use an atomic increment but don't need sequencing due to `prevCount > 0`.
                if (_retainCount.fetch_add(1, std::memory_order_relaxed) == 0) {

                    const int open_flags = internal::db_open_mode_to_int_flags(open_mode);

                    int rc = sqlite3_open_v2(this->filename.c_str(), &this->db, open_flags, vfs_name.c_str());

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
            sqlite3* db = nullptr;

          private:
            std::atomic_int _retainCount{};

          private:
            const std::function<void(sqlite3* db)> _didOpenDb;

          public:
            const std::string vfs_name;
            const db_open_mode open_mode;
            const std::string filename;
        };

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
