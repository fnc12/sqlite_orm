#pragma once

#include <iterator>  //  std::back_inserter
#include <string>  //  std::string
#include <memory>  //  std::unique_ptr
#include <array>  //  std::array
#include <algorithm>  //  std::transform
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

        inline const serialize_result_type& to_string(journal_mode value) {
            static const std::array<serialize_result_type, 6> res = {
                "DELETE",
                "TRUNCATE",
                "PERSIST",
                "MEMORY",
                "WAL",
                "OFF",
            };
            return res.at(static_cast<int>(value));
        }

        inline std::pair<bool, journal_mode> journal_mode_from_string(const std::string& string) {
            std::string upperString;
            std::transform(string.begin(), string.end(), std::back_inserter(upperString), [](char c) {
                return static_cast<char>(std::toupper(static_cast<int>(c)));
            });
            static const std::array<journal_mode, 6> allValues = {{
                journal_mode::DELETE,
                journal_mode::TRUNCATE,
                journal_mode::PERSIST,
                journal_mode::MEMORY,
                journal_mode::WAL,
                journal_mode::OFF,
            }};
            for(auto journalMode: allValues) {
                if(to_string(journalMode) == upperString) {
                    return {true, journalMode};
                }
            }
            return {false, journal_mode::OFF};
        }
    }
}

#if defined(_WINNT_)
#pragma pop_macro("DELETE")
#endif
