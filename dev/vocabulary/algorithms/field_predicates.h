#pragma once

/** @file Definitions of closed predicates for checking the validity of fields of column nodes.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of, std::is_integral, std::is_signed, std::enable_if
#endif

#include "field_predicates_fwd.h"  // Included to specialize field predicates
#include "../../type_printer.h"

namespace sqlite_orm::internal {
    // Custom integral type:
    // It is the programmer's responsibility to ensure data integrity in the value range of the custom type
    // and in purview of SQLite using a 64-bit signed integer.
    template<class F, class SFINAE>
    constexpr bool is_rowid_alias_capable_v = std::is_base_of<integer_printer, type_printer<F>>::value;

    // For 64-bit signed integer type: capable
    template<class F>
    constexpr bool
        is_rowid_alias_capable_v<F,
                                 std::enable_if_t<std::is_integral<F>::value &&
                                                  (sizeof(F) == sizeof(sqlite_int64) &&
                                                   std::is_signed<F>::value == std::is_signed<sqlite_int64>::value)>> =
            true;

    // Design decision for integral types other than 64-bit signed integer:
    // It is the programmer's responsibility to ensure data integrity in the value range of the integral type
    // and in purview of SQLite using a 64-bit signed integer.
    template<class F>
    constexpr bool
        is_rowid_alias_capable_v<F,
                                 std::enable_if_t<std::is_integral<F>::value &&
                                                  (sizeof(F) != sizeof(sqlite_int64) ||
                                                   std::is_signed<F>::value != std::is_signed<sqlite_int64>::value)>> =
            true;
}
