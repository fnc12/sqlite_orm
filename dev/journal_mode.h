#pragma once

#include <array>  //  std::array
#include <string>  //  std::string
#include <utility>  //  std::pair
#include <algorithm>  //  std::ranges::transform
#include <cctype>  // std::toupper

#include "serialize_result_type.h"

#if defined(_WINNT_)
// DELETE is a macro defined in the Windows SDK (winnt.h)
#pragma push_macro("DELETE")
#undef DELETE
#endif

namespace sqlite_orm {

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

    namespace internal {

        inline const serialize_result_type& journal_mode_to_string(journal_mode value) {
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
            static constexpr std::array<serialize_result_type, 6> idx2str = {
#else
            static const std::array<serialize_result_type, 6> idx2str = {
#endif
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
#if __cpp_lib_ranges >= 201911L
            std::ranges::transform(string, string.begin(), [](unsigned char c) noexcept {
                return std::toupper(c);
            });
            if (auto found = std::ranges::find(journalModes, string, journal_mode_to_string);
                found != journalModes.end()) SQLITE_ORM_CPP_LIKELY {
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
}

#if defined(_WINNT_)
#pragma pop_macro("DELETE")
#endif
