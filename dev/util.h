#pragma once

#include <sqlite3.h>
#include <system_error>  //  std::system_error, std::error_code

namespace sqlite_orm {

    namespace internal {
        static void perform_step(sqlite3 *db, sqlite3_stmt *stmt) {
            if(sqlite3_step(stmt) == SQLITE_DONE) {
                //  done..
            } else {
                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                        sqlite3_errmsg(db));
            }
        }
    }
}
