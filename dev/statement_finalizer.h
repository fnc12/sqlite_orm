#pragma once

#include <sqlite3.h>
#ifndef _IMPORT_STD_MODULE
#include <memory>  // std::unique_ptr
#include <type_traits>  // std::integral_constant
#endif

_EXPORT_SQLITE_ORM namespace sqlite_orm {

    /**
     *  Guard class which finalizes `sqlite3_stmt` in dtor
     */
    using statement_finalizer =
        std::unique_ptr<sqlite3_stmt, std::integral_constant<decltype(&sqlite3_finalize), sqlite3_finalize>>;
}
