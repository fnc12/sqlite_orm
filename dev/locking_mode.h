#pragma once

#include <array>  //  std::array
#include <string>  //  std::string
#include <utility>  //  std::pair
#include <algorithm>  //  std::ranges::transform
#include <cctype>  // std::toupper

#include "serialize_result_type.h"

namespace sqlite_orm {
    enum class locking_mode : signed char {
        NORMAL = 0,
        EXCLUSIVE = 1,
    };

    namespace internal {
        inline const serialize_result_type& locking_mode_to_string(locking_mode value) {
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
            static constexpr std::array<serialize_result_type, 2> idx2str = {
#else
            static const std::array<serialize_result_type, 2> idx2str = {
#endif
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

#if __cpp_lib_ranges >= 201911L
            std::ranges::transform(string, string.begin(), [](unsigned char c) noexcept {
                return std::toupper(c);
            });
            if (auto found = std::ranges::find(lockingModes, string, locking_mode_to_string);
                found != lockingModes.end()) SQLITE_ORM_CPP_LIKELY {
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
}