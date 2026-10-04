#pragma once

/** @file Execution of statements through the SQLite C library: preparing, stepping and resetting statements,
 *        with or without the query hooks of a storage.
 */

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#include <functional>  //  std::function
#include <string_view>  //  std::string_view
#endif

#include "../functional/gsl.h"
#include "sqlite3_errors.h"

namespace sqlite_orm::internal {
    // Wrapper to reduce boiler-plate code
    inline sqlite3_stmt* reset_stmt(sqlite3_stmt* stmt) {
        sqlite3_reset(stmt);
        return stmt;
    }

    inline sqlite3_stmt* prepare_stmt(sqlite3* db, std::string_view query) {
        sqlite3_stmt* stmt;
        const int rc = sqlite3_prepare_v2(db, query.data(), int(query.size()), &stmt, nullptr);
        if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
            throw_translated_sqlite_error(rc);
        }
        return stmt;
    }

    template<int expected = SQLITE_DONE>
    void perform_single_step(sqlite3_stmt* stmt) {
        const int rc = sqlite3_step(stmt);
        if (rc != expected) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
            throw_translated_sqlite_error(rc);
        }
    }

    template<class L>
    void perform_step(sqlite3_stmt* stmt, L&& lambda) {
        switch (SQLITE_ORM_SWITCH_MAYBE_UNUSED int rc = sqlite3_step(stmt)) {
            case SQLITE_ROW: {
                lambda(stmt);
            } break;
            case SQLITE_DONE:
                return;
            default:
                SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                    throw_translated_sqlite_error(stmt);
                }
        }
    }

    template<class L>
    void perform_steps(sqlite3_stmt* stmt, L&& lambda) {
        for (;;) {
            switch (SQLITE_ORM_SWITCH_MAYBE_UNUSED int rc = sqlite3_step(stmt)) {
                case SQLITE_ROW: {
                    lambda(stmt);
                } break;
                case SQLITE_DONE:
                    return;
                default:
                    SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                        throw_translated_sqlite_error(stmt);
                    }
            }
        }
    }

    struct sqlite_executor {
        std::function<void(std::string_view sql)> will_run_query;
        std::function<void(std::string_view sql)> did_run_query;

        void perform_void_exec(sqlite3* db, orm_gsl::czstring sql) const {
            if (this->will_run_query) {
                this->will_run_query(sql);
            }

            const int rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
            if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                throw_translated_sqlite_error(rc);
            }

            if (this->did_run_query) {
                this->did_run_query(sql);
            }
        }

        void perform_exec(sqlite3* db,
                          orm_gsl::czstring sql,
                          int (*callback)(void*, int, orm_gsl::zstring*, orm_gsl::zstring*),
                          void* user_data) const {
            if (this->will_run_query) {
                this->will_run_query(sql);
            }

            const int rc = sqlite3_exec(db, sql, callback, user_data, nullptr);
            if (rc != SQLITE_OK) SQLITE_ORM_CPP_UNLIKELY /*possible but unexpected*/ {
                throw_translated_sqlite_error(rc);
            }

            if (this->did_run_query) {
                this->did_run_query(sql);
            }
        }

        void perform_exec(sqlite3* db,
                          const std::string& query,
                          int (*callback)(void*, int, orm_gsl::zstring*, orm_gsl::zstring*),
                          void* user_data) const {
            return perform_exec(db, query.c_str(), callback, user_data);
        }

        template<int expected = SQLITE_DONE>
        void perform_single_step(sqlite3_stmt* stmt) const {
            orm_gsl::czstring sql = nullptr;
            if (this->will_run_query || this->did_run_query) {
                sql = sqlite3_sql(stmt);
            }
            if (this->will_run_query) {
                this->will_run_query(sql);
            }

            internal::perform_single_step<expected>(stmt);

            if (this->did_run_query) {
                this->did_run_query(sql);
            }
        }

        template<class L>
        void perform_step(sqlite3_stmt* stmt, L&& lambda) const {
            orm_gsl::czstring sql = nullptr;
            if (this->will_run_query || this->did_run_query) {
                sql = sqlite3_sql(stmt);
            }
            if (this->will_run_query) {
                this->will_run_query(sql);
            }

            internal::perform_step(stmt, lambda);

            if (this->did_run_query) {
                this->did_run_query(sql);
            }
        }

        template<class L>
        void perform_steps(sqlite3_stmt* stmt, L&& lambda) const {
            orm_gsl::czstring sql = nullptr;
            if (this->will_run_query || this->did_run_query) {
                sql = sqlite3_sql(stmt);
            }
            if (this->will_run_query) {
                this->will_run_query(sql);
            }

            internal::perform_steps(stmt, lambda);

            if (this->did_run_query) {
                this->did_run_query(sql);
            }
        }
    };
}
