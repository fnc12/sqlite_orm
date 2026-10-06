#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  //  std::unique_ptr
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <type_traits>  //  std::integral_constant, std::bool_constant
#include <utility>  //  std::move, std::exchange
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/gsl.h"
#include "sqlite3/sqlite3_deleters.h"  //  sqlite3_memory_deleter
#include "connection_holder.h"

namespace sqlite_orm::internal {
    struct prepared_statement_base {
        orm_gsl::owner<sqlite3_stmt*> stmt = nullptr;
        connection_ref con;

        ~prepared_statement_base() {
            sqlite3_finalize(this->stmt);
        }

        std::string sql() const {
            // note: sqlite3 internally checks for null before calling
            // sqlite3_normalized_sql() or sqlite3_expanded_sql(), so check here, too, even if superfluous
            if (orm_gsl::czstring sql = sqlite3_sql(this->stmt)) {
                return sql;
            } else {
                return {};
            }
        }

#if SQLITE_VERSION_NUMBER >= 3014000
        std::string expanded_sql() const {
            // note: must check return value due to SQLITE_OMIT_TRACE
            using char_ptr = std::unique_ptr<char[], sqlite3_memory_deleter>;

            if (char_ptr sql{sqlite3_expanded_sql(this->stmt)}) {
                return sql.get();
            } else {
                return {};
            }
        }
#endif
#if SQLITE_VERSION_NUMBER >= 3026000 and defined(SQLITE_ENABLE_NORMALIZE)
        std::string normalized_sql() const {
            if (orm_gsl::czstring sql = sqlite3_normalized_sql(this->stmt)) {
                return sql;
            } else {
                return {};
            }
        }
#endif

        std::string_view column_name(int index) const {
            return sqlite3_column_name(stmt, index);
        }

        /**
         *  sqlite3_stmt_readonly function: whether the statement makes no direct changes
         *  to the content of the database file.
         */
        int readonly() const {
            return sqlite3_stmt_readonly(this->stmt);
        }

        /**
         *  sqlite3_stmt_busy function: whether the statement has been stepped at least once
         *  but has not run to completion or been reset.
         */
        int busy() const {
            return sqlite3_stmt_busy(this->stmt);
        }

#if SQLITE_VERSION_NUMBER >= 3028000
        /**
         *  sqlite3_stmt_isexplain function: 1 if the statement is an EXPLAIN statement,
         *  2 if it is an EXPLAIN QUERY PLAN, 0 for an ordinary statement.
         */
        int is_explain() const {
            return sqlite3_stmt_isexplain(this->stmt);
        }
#endif
    };

    template<class T>
    struct prepared_statement_t : prepared_statement_base {
        using expression_type = T;

        expression_type expression;

        prepared_statement_t(T expression_, sqlite3_stmt* stmt_, connection_ref con_) :
            prepared_statement_base{stmt_, std::move(con_)}, expression(std::move(expression_)) {}

        prepared_statement_t(prepared_statement_t&& prepared_stmt) :
            prepared_statement_base{std::exchange(prepared_stmt.stmt, nullptr), std::move(prepared_stmt.con)},
            expression(std::move(prepared_stmt.expression)) {}
    };

    template<class T>
    constexpr bool is_prepared_statement_v = polyfill::is_specialization_of<T, prepared_statement_t>::value;

    template<class T>
    struct is_prepared_statement : std::bool_constant<is_prepared_statement_v<T>> {};
}
