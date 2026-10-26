#pragma once

#include <sqlite3.h>

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <algorithm>  //  std::ranges::transform
#include <array>  //  std::array
#include <cctype>  // std::toupper
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <system_error>  //  std::system_error
#include <utility>  //  std::pair
#endif

#include "../functional/gsl.h"
#include "../error_code.h"
#include "../marshaling/row_extractor.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {
    enum class locking_mode : signed char {
        NORMAL = 0,
        EXCLUSIVE = 1,
    };
}

namespace sqlite_orm::internal {
    inline std::string_view locking_mode_to_string(locking_mode value) {
        static constexpr std::array<std::string_view, 2> idx2str = {
            "NORMAL",
            "EXCLUSIVE",
        };
        return idx2str.at(static_cast<int>(value));
    }

    inline std::pair<bool, locking_mode> locking_mode_from_string(std::string string) {
        static constexpr std::array<locking_mode, 2> lockingModes = {{
            locking_mode::NORMAL,
            locking_mode::EXCLUSIVE,
        }};

#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
        std::ranges::transform(string, string.begin(), [](unsigned char c) SQLITE_ORM_STATIC_CALLOP noexcept {
            return std::toupper(c);
        });
        if (auto found = std::ranges::find(lockingModes, string, locking_mode_to_string); found != lockingModes.end())
            SQLITE_ORM_CPP_LIKELY {
            return {true, *found};
        }
#else
        std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) noexcept {
            return std::toupper(c);
        });
        for (auto lockingMode: lockingModes) {
            if (locking_mode_to_string(lockingMode) == string) {
                return {true, lockingMode};
            }
        }
#endif
        return {false, locking_mode::NORMAL};
    }
}

// `row_extractor` specialization
namespace sqlite_orm {
    /**
     *  Specialization for locking_mode.
     */
    template<>
    struct row_extractor<locking_mode, void> {
        locking_mode extract(orm_gsl::czstring columnText) const {
            if (columnText) {
                if (auto [found, mode] = internal::locking_mode_from_string(columnText); found) {
                    return mode;
                } else {
                    throw std::system_error{orm_error_code::incorrect_locking_mode_string};
                }
            } else {
                throw std::system_error{orm_error_code::incorrect_locking_mode_string};
            }
        }

        locking_mode extract(sqlite3_stmt* stmt, int columnIndex) const {
            orm_gsl::czstring text = (const char*)sqlite3_column_text(stmt, columnIndex);
            return this->extract(text);
        }

        locking_mode extract(sqlite3_value* value) const = delete;
    };
}
