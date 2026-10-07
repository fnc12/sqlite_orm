#pragma once

/** @file SQLite's result codes as `std::error_code`s - `sqlite_errc` and its `std::error_category` - and the
 *        translation of a failing C library call into a `std::system_error`.
 */

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <system_error>  //  std::error_code, std::error_category, std::system_error, std::is_error_code_enum
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <type_traits>  //  std::true_type
#include <utility>  //  std::forward
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** @short Enables classifying sqlite error codes.

     *  @note We don't bother listing all possible values;
     *  this also allows for compatibility with
     *  'Construction rules for enum class values (P0138R2)'
     */
    enum class sqlite_errc {};
}

namespace std {
    template<>
    struct is_error_code_enum<::sqlite_orm::sqlite_errc> : true_type {};
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    class sqlite_error_category : public std::error_category {
      public:
        const char* name() const noexcept override final {
            return "SQLite error";
        }

        std::string message(int ev) const override final {
            return sqlite3_errstr(ev);
        }
    };

    inline const sqlite_error_category& get_sqlite_error_category() {
        static sqlite_error_category res;
        return res;
    }

    inline std::error_code make_error_code(sqlite_errc ev) noexcept {
        return {static_cast<int>(ev), get_sqlite_error_category()};
    }

    template<typename... T>
    std::string get_error_message(sqlite3* db, T&&... args) {
        std::stringstream stream;
        using unpack = int[];
        (void)unpack{0, (stream << args, 0)...};
        stream << sqlite3_errmsg(db);
        return stream.str();
    }

    template<typename... T>
    [[noreturn]] void throw_error(sqlite3* db, T&&... args) {
        throw std::system_error{sqlite_errc(sqlite3_errcode(db)), get_error_message(db, std::forward<T>(args)...)};
    }

    inline std::system_error sqlite_to_system_error(int ev) {
        return {sqlite_errc(ev)};
    }

    inline std::system_error sqlite_to_system_error(sqlite3* db) {
        return {sqlite_errc(sqlite3_errcode(db)), sqlite3_errmsg(db)};
    }

    [[noreturn]] inline void throw_translated_sqlite_error(int ev) {
        throw sqlite_to_system_error(ev);
    }

    [[noreturn]] inline void throw_translated_sqlite_error(sqlite3* db) {
        throw sqlite_to_system_error(db);
    }

    [[noreturn]] inline void throw_translated_sqlite_error(sqlite3_stmt* stmt) {
        throw sqlite_to_system_error(sqlite3_db_handle(stmt));
    }
}
