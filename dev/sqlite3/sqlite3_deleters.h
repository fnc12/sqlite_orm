#pragma once

/** @file Deleters of what the SQLite C library allocates, for use with `std::unique_ptr`, and the
 *        `statement_finalizer` guard built on one of them.
 *
 *        A deleter is the C library's release function as a function constant; under clang-cl
 *        (`SQLITE_ORM_CLANG_MSVC`) it is a function object calling it instead.
 */

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  //  std::unique_ptr
#include <type_traits>  //  std::integral_constant
#endif

namespace sqlite_orm::internal {
#ifndef SQLITE_ORM_CLANG_MSVC
    /**
     *  Finalizes a statement.
     */
    using statement_deleter = std::integral_constant<decltype(&sqlite3_finalize), sqlite3_finalize>;

    /**
     *  Frees memory allocated by SQLite, e.g. the result of `sqlite3_expanded_sql()`.
     */
    using sqlite3_memory_deleter = std::integral_constant<decltype(&sqlite3_free), sqlite3_free>;
#else
    struct statement_deleter {
        SQLITE_ORM_STATIC_CALLOP void operator()(sqlite3_stmt* stmt) SQLITE_ORM_OR_CONST_CALLOP noexcept {
            sqlite3_finalize(stmt);
        }
    };

    struct sqlite3_memory_deleter {
        SQLITE_ORM_STATIC_CALLOP void operator()(void* mem) SQLITE_ORM_OR_CONST_CALLOP noexcept {
            sqlite3_free(mem);
        }
    };
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Guard class which finalizes `sqlite3_stmt` in dtor
     */
    using statement_finalizer = std::unique_ptr<sqlite3_stmt, internal::statement_deleter>;
}
