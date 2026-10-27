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

#if defined(_WINNT_)
// DELETE is a macro defined in the Windows SDK (winnt.h)
#pragma push_macro("DELETE")
#undef DELETE
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Caps case because of:
     *  1) delete keyword;
     *  2) https://www.sqlite.org/pragma.html#pragma_journal_mode original spelling
     */
    enum class journal_mode : signed char {
        DELETE = 0,
        // An alternate enumeration value when using the Windows SDK that defines DELETE as a macro.
        DELETE_ = DELETE,
        TRUNCATE = 1,
        PERSIST = 2,
        MEMORY = 3,
        WAL = 4,
        OFF = 5,
    };
}

namespace sqlite_orm::internal {
    inline std::string_view journal_mode_to_string(journal_mode value) {
        static constexpr std::array<std::string_view, 6> idx2str = {
            "DELETE",
            "TRUNCATE",
            "PERSIST",
            "MEMORY",
            "WAL",
            "OFF",
        };
        return idx2str.at(static_cast<int>(value));
    }

    inline std::pair<bool, journal_mode> journal_mode_from_string(std::string string) {
        static constexpr std::array<journal_mode, 6> journalModes = {{
            journal_mode::DELETE,
            journal_mode::TRUNCATE,
            journal_mode::PERSIST,
            journal_mode::MEMORY,
            journal_mode::WAL,
            journal_mode::OFF,
        }};
#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
        std::ranges::transform(string, string.begin(), [](unsigned char c) SQLITE_ORM_STATIC_CALLOP noexcept {
            return std::toupper(c);
        });
        if (auto found = std::ranges::find(journalModes, string, journal_mode_to_string); found != journalModes.end())
            SQLITE_ORM_CPP_LIKELY {
            return {true, *found};
        }
#else
        std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) noexcept {
            return std::toupper(c);
        });
        for (auto journalMode: journalModes) {
            if (journal_mode_to_string(journalMode) == string) {
                return {true, journalMode};
            }
        }
#endif
        return {false, journal_mode::OFF};
    }
}

// `row_extractor` specialization
namespace sqlite_orm {
    /**
     *  Specialization for journal_mode.
     */
    template<>
    struct row_extractor<journal_mode, void> {
        journal_mode extract(orm_gsl::czstring columnText) const {
            if (columnText) {
                if (auto [found, mode] = internal::journal_mode_from_string(columnText); found) {
                    return mode;
                } else {
                    throw std::system_error{orm_error_code::incorrect_journal_mode_string};
                }
            } else {
                throw std::system_error{orm_error_code::incorrect_journal_mode_string};
            }
        }

        journal_mode extract(sqlite3_stmt* stmt, int columnIndex) const {
            orm_gsl::czstring text = (const char*)sqlite3_column_text(stmt, columnIndex);
            return this->extract(text);
        }

        journal_mode extract(sqlite3_value* value) const = delete;
    };
}

#if defined(_WINNT_)
#pragma pop_macro("DELETE")
#endif
