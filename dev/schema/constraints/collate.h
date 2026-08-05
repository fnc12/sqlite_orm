#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <system_error>  //  std::system_error
#include <string>  //  std::string
#endif

#include "../../collate_argument.h"
#include "../../error_code.h"

namespace sqlite_orm::internal {
    struct collate_constraint_t {
        collate_argument argument = collate_argument::binary;

        static std::string string_from_collate_argument(collate_argument argument) {
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
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    constexpr internal::collate_constraint_t collate_nocase() {
        return {internal::collate_argument::nocase};
    }

    constexpr internal::collate_constraint_t collate_binary() {
        return {internal::collate_argument::binary};
    }

    constexpr internal::collate_constraint_t collate_rtrim() {
        return {internal::collate_argument::rtrim};
    }
}
