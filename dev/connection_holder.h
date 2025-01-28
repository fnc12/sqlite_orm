#pragma once

#include <sqlite3.h>
#ifndef _IMPORT_STD_MODULE
#include <atomic>
#include <string>  //  std::string
#endif

#include "error_code.h"

namespace sqlite_orm {

    namespace internal {

        struct connection_holder {

            connection_holder(std::string filename_) : filename(std::move(filename_)) {}

            void retain() {
                if (1 == ++this->_retain_count) {
                    auto rc = sqlite3_open(this->filename.c_str(), &this->db);
                    if (rc != SQLITE_OK) {
                        throw_translated_sqlite_error(db);
                    }
                }
            }

            void release() {
                if (0 == --this->_retain_count) {
                    auto rc = sqlite3_close(this->db);
                    if (rc != SQLITE_OK) {
                        throw_translated_sqlite_error(db);
                    }
                }
            }

            sqlite3* get() const {
                return this->db;
            }

            int retain_count() const {
                return this->_retain_count;
            }

            const std::string filename;

          protected:
            sqlite3* db = nullptr;
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
