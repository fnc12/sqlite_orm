#pragma once

#include <sqlite3.h>
#include <string>  //  std::string
#include <system_error>  //  std::system_error

namespace sqlite_orm {

    namespace internal {
        inline void perform_step(sqlite3* db, sqlite3_stmt* stmt) {
            auto rc = sqlite3_step(stmt);
            if(rc == SQLITE_DONE) {
                //  done..
            } else {
                throw std::system_error{sqlite3_errcode(db), get_sqlite_error_category(), sqlite3_errmsg(db)};
            }
        }

        static void perform_void_exec(sqlite3* db, const std::string& query) {
            int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                throw std::system_error{sqlite3_errcode(db), get_sqlite_error_category(), sqlite3_errmsg(db)};
            }
        }
    }
}
