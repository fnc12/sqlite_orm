#pragma once

#include <type_traits>

#include "start_macros.h"
#include "cxx_polyfill.h"

namespace sqlite_orm {
    namespace internal {
        template<class T, template<typename...> class Primary>
        using match_specialization_of = std::enable_if_t<polyfill::is_specialization_of_v<T, Primary>>;
    }
}
