#pragma once

#include <sqlite3.h>
#include <string>  //  std::string
#include <system_error>  //  std::system_error, std::error_code

namespace sqlite_orm {

    namespace internal {
        inline void perform_step(sqlite3* db, sqlite3_stmt* stmt) {
            auto rc = sqlite3_step(stmt);
            if(rc == SQLITE_DONE) {
                //  done..
            } else {
                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                        sqlite3_errmsg(db));
            }
        }

        static void perform_void_exec(sqlite3* db, const std::string& query) {
            int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                        sqlite3_errmsg(db));
            }
        }

        template<class T>
        inline auto call_insert_impl_and_catch_constraint_failed(const T& insert_impl) {
            try {
                return insert_impl();
            } catch(const std::system_error& e) {
                if(e.code() == std::error_code(SQLITE_CONSTRAINT, get_sqlite_error_category())) {
                    std::stringstream ss;
                    ss << "Attempting to execute 'insert' request resulted in an error like \"" << e.what()
                       << "\". Perhaps ordinary 'insert' is not acceptable for this table and you should try "
                          "'replace' or 'insert' with explicit column listing?";
                    throw std::system_error(e.code(), ss.str());
                }
                throw;
            }
        }
    }
}
