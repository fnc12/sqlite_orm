#pragma once

/** @file Definitions of closed predicates for checking the validity of fields of column nodes.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of, std::is_integral, std::is_signed, std::enable_if, std::is_same, std::disjunction
#include <string>  //  std::string, std::wstring
#include <string_view>  //  std::string_view, std::wstring_view
#endif

#include "../../functional/gsl.h"  // orm_gsl::czstring, orm_gsl::cwzstring
#include "../../type_printer.h"
#include "field_predicates_fwd.h"  // Included to specialize field predicates

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

    template<class T>
    constexpr bool is_text_value_v = std::disjunction<std::is_same<T, orm_gsl::czstring>,
                                                      std::is_same<T, std::string_view>,
                                                      std::is_same<T, std::string>,
                                                      std::is_same<T, orm_gsl::cwzstring>,
                                                      std::is_same<T, std::wstring_view>,
                                                      std::is_same<T, std::wstring>>::value;
}
