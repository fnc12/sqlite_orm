#pragma once

#include <memory>  // std::unique_ptr
#include <sqlite3.h>
#include <type_traits>  // std::integral_constant

#include "start_macros.h"

namespace sqlite_orm {

    /**
     *  Guard class which finalizes `sqlite3_stmt` in dtor
     */
    using statement_finalizer =
        std::unique_ptr<sqlite3_stmt, std::integral_constant<decltype(&sqlite3_finalize), sqlite3_finalize>>;

    using void_fn_t = void (*)();
    using xdestroy_fn_t = void (*)(void*);
    using null_xdestroy_t = std::integral_constant<xdestroy_fn_t, nullptr>;
    SQLITE_ORM_INLINE_VAR constexpr null_xdestroy_t null_xdestroy{};
}
