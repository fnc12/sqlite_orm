#pragma once

#include <sqlite3.h>
#include <atomic>
#include <string>  //  std::string

#include "storage_options.h"
#include "error_code.h"

namespace sqlite_orm {
    namespace internal {

        struct connection_holder {

            connection_holder(std::string filename_, const storage_options options_) :
                filename(std::move(filename_)), options(std::move(options_)) {}

            void retain() {
                // first one opens the connection.
                // we presume that the connection is opened once in a single-threaded context [also open forever].
                // therefore we can just use an atomic increment but don't need sequencing due to `prevCount > 0`.
                if (this->_retain_count.fetch_add(1, std::memory_order_relaxed) == 0) {

                    const serialize_result_type& vfs_name = internal::vfs_object_to_string(options.vfs_option);
                    const int open_flags = internal::open_mode_to_int_flags(options.open_option);

                    int rc = sqlite3_open_v2(this->filename.c_str(), &this->db, open_flags, vfs_name.c_str());

                    if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY /*possible, but unexpected*/ {
                        throw_translated_sqlite_error(this->db);
                    }
                }
            }

            void release() {
                // last one closes the connection.
                // we assume that this might happen by any thread, therefore the counter must serve as a synchronization point.
                if (this->_retain_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
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
                return this->_retain_count.load(std::memory_order_relaxed);
            }

            const std::string filename;
            const storage_options options;

          protected:
            sqlite3* db = nullptr;

          private:
            std::atomic_int _retain_count{};
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
