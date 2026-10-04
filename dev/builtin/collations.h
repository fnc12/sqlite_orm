#pragma once

/** @file SQLite's built-in collating functions: BINARY, NOCASE and RTRIM.
 *
 *  Like the built-in functions and database objects, they are the stock instances of a kind of named object
 *  that applications can register more of (`storage_base::create_collation()`).
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string_view>  //  std::string_view
#include <system_error>  //  std::system_error
#endif

#include "../error_code.h"

namespace sqlite_orm::internal {
    enum class collate_argument {
        binary,
        nocase,
        rtrim,
    };

    inline std::string_view collate_argument_to_string(collate_argument argument) {
        switch (argument) {
            case collate_argument::binary:
                return "BINARY";
            case collate_argument::nocase:
                return "NOCASE";
            case collate_argument::rtrim:
                return "RTRIM";
        }
        throw std::system_error{orm_error_code::invalid_collate_argument_enum};
    }
}
