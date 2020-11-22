#pragma once

#include <sqlite3.h>
#include <string>  //  std::string
#include <system_error>  //  std::system_error

#include "error_code.h"

namespace sqlite_orm {

    namespace internal {

        struct connection_holder {

            connection_holder(std::string filename_) : filename(move(filename_)) {}

            void retain() {
                ++this->_retain_count;
                if(1 == this->_retain_count) {
                    auto rc = sqlite3_open(this->filename.c_str(), &this->db);
                    if(rc != SQLITE_OK) {
                        throw std::system_error(std::error_code(sqlite3_errcode(this->db), get_sqlite_error_category()),
                                                sqlite3_errmsg(this->db));
                    }
                }
            }

            void release() {
                --this->_retain_count;
                if(0 == this->_retain_count) {
                    auto rc = sqlite3_close(this->db);
                    if(rc != SQLITE_OK) {
                        throw std::system_error(std::error_code(sqlite3_errcode(this->db), get_sqlite_error_category()),
                                                sqlite3_errmsg(this->db));
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
            int _retain_count = 0;
        };

        struct connection_ref {
            connection_ref(connection_holder& holder_) : holder(holder_) {
                this->holder.retain();
            }

            connection_ref(const connection_ref& other) : holder(other.holder) {
                this->holder.retain();
            }

            connection_ref(connection_ref&& other) : holder(other.holder) {
                this->holder.retain();
            }

            ~connection_ref() {
                this->holder.release();
            }

            sqlite3* get() const {
                return this->holder.get();
            }

          protected:
            connection_holder& holder;
        };
    }
}
