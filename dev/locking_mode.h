#include <array>  //  std::array
#include <string>  //  std::string
#include <utility>  //  std::pair
#include <iterator>  //  std::back_inserter

#include "serialize_result_type.h"

namespace sqlite_orm {
    enum class locking_mode : signed char {
        NORMAL = 0,
        EXCLUSIVE = 1,
    };

    namespace internal {
        inline const serialize_result_type& to_string(locking_mode value) {
            static const std::array<serialize_result_type, 2> res = {
                "NORMAL",
                "EXCLUSIVE",
            };
            return res.at(static_cast<int>(value));
        }

        inline std::pair<bool, locking_mode> locking_mode_from_string(const std::string& string) {
            std::string upperString;
            std::transform(string.begin(), string.end(), std::back_inserter(upperString), [](char c) {
                return static_cast<char>(std::toupper(static_cast<int>(c)));
            });
            static const std::array<locking_mode, 2> allValues = {{
                locking_mode::NORMAL,
                locking_mode::EXCLUSIVE,
            }};
            for(auto lockingMode: allValues) {
                if(to_string(lockingMode) == upperString) {
                    return {true, lockingMode};
                }
            }
            return {false, locking_mode::NORMAL};
        }
    }
}