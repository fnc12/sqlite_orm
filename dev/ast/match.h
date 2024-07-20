#pragma once

#include <utility>  // std::move

namespace sqlite_orm {
    namespace internal {

        template<class T, class X>
        struct match_t {
            using mapped_type = T;
            using argument_type = X;

            argument_type argument;

            match_t(argument_type argument) : argument(std::move(argument)) {}
        };
    }

    template<class T, class X>
    internal::match_t<T, X> match(X argument) {
        return {std::move(argument)};
    }
}
