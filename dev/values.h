#pragma once

#include <vector>
#include <initializer_list>

namespace sqlite_orm {

    namespace internal {

        template<class... Args>
        struct values_t {
            std::tuple<Args...> tuple;
        };

    }

    template<class... Args>
    internal::values_t<Args...> values(Args... args) {
        return {{std::forward<Args>(args)...}};
    }

}
