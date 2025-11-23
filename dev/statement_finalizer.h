#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  // std::unique_ptr
#include <type_traits>  // std::integral_constant
#endif

#ifdef SQLITE_ORM_CLANG_ON_WIN
namespace sqlite_orm::internal {
    struct statement_deleter {
        SQLITE_ORM_STATIC_CALLOP void operator()(sqlite3_stmt* stmt) SQLITE_ORM_OR_CONST_CALLOP noexcept {
            sqlite3_finalize(stmt);
        }
    };
}
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {

#ifndef SQLITE_ORM_CLANG_ON_WIN
    /**
     *  Guard class which finalizes `sqlite3_stmt` in dtor
     */
    using statement_finalizer =
        std::unique_ptr<sqlite3_stmt, std::integral_constant<decltype(&sqlite3_finalize), sqlite3_finalize>>;
#else
    using statement_finalizer = std::unique_ptr<sqlite3_stmt, internal::statement_deleter>;
#endif
}
