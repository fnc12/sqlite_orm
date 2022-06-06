#pragma once

#include <sqlite3.h>
#include <string>  //  std::string
#include <utility>  //  std::move

#include "error_code.h"

namespace sqlite_orm {

    /** 
     *  Escape the provided character in the given string by doubling it.
     *  @param str A copy of the original string
     *  @param char2Escape The character to escape
     */
    inline std::string sql_escape(std::string str, char char2Escape) {
        for(size_t pos = 0; (pos = str.find(char2Escape, pos)) != str.npos; pos += 2) {
            str.replace(pos, 1, 2, char2Escape);
        }

        return str;
    }

    /** 
     *  Quote the given string value using single quotes,
     *  escape containing single quotes by doubling them.
     */
    inline std::string quote_string_literal(std::string v) {
        constexpr char quoteChar = '\'';
        return quoteChar + sql_escape(move(v), quoteChar) + quoteChar;
    }

    /** 
     *  Quote the given string value using single quotes,
     *  escape containing single quotes by doubling them.
     */
    inline std::string quote_blob_literal(std::string v) {
        constexpr char quoteChar = '\'';
        return std::string{char('x'), quoteChar} + move(v) + quoteChar;
    }

    /** 
     *  Quote the given identifier using double quotes,
     *  escape containing double quotes by doubling them.
     */
    inline std::string quote_identifier(std::string identifier) {
        constexpr char quoteChar = '"';
        return quoteChar + sql_escape(move(identifier), quoteChar) + quoteChar;
    }

    namespace internal {
        // Wrapper to reduce boiler-plate code
        inline sqlite3_stmt* reset_stmt(sqlite3_stmt* stmt) {
            sqlite3_reset(stmt);
            return stmt;
        }

        // note: query is deliberately taken by value, such that it is thrown away early
        inline sqlite3_stmt* prepare_stmt(sqlite3* db, std::string query) {
            sqlite3_stmt* stmt;
            if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                throw_translated_sqlite_error(db);
            }
            return stmt;
        }

        inline void perform_void_exec(sqlite3* db, const std::string& query) {
            int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                throw_translated_sqlite_error(db);
            }
        }

        inline void perform_exec(sqlite3* db,
                                 const char* query,
                                 int (*callback)(void* data, int argc, char** argv, char**),
                                 void* user_data) {
            int rc = sqlite3_exec(db, query, callback, user_data, nullptr);
            if(rc != SQLITE_OK) {
                throw_translated_sqlite_error(db);
            }
        }

        inline void perform_exec(sqlite3* db,
                                 const std::string& query,
                                 int (*callback)(void* data, int argc, char** argv, char**),
                                 void* user_data) {
            return perform_exec(db, query.c_str(), callback, user_data);
        }

        template<int expected = SQLITE_DONE>
        void perform_step(sqlite3_stmt* stmt) {
            int rc = sqlite3_step(stmt);
            if(rc != expected) {
                throw_translated_sqlite_error(stmt);
            }
        }

        template<class L>
        void perform_step(sqlite3_stmt* stmt, L&& lambda) {
            switch(int rc = sqlite3_step(stmt)) {
                case SQLITE_ROW: {
                    lambda(stmt);
                } break;
                case SQLITE_DONE:
                    break;
                default: {
                    throw_translated_sqlite_error(stmt);
                }
            }
        }

        template<class L>
        void perform_steps(sqlite3_stmt* stmt, L&& lambda) {
            int rc;
            do {
                switch(rc = sqlite3_step(stmt)) {
                    case SQLITE_ROW: {
                        lambda(stmt);
                    } break;
                    case SQLITE_DONE:
                        break;
                    default: {
                        throw_translated_sqlite_error(stmt);
                    }
                }
            } while(rc != SQLITE_DONE);
        }
    }
}
