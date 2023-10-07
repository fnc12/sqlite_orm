#pragma once

#include "functional/cxx_functional_polyfill.h"

namespace sqlite_orm {
    namespace internal {
        struct negatable_t {};

        /**
         *  Inherit from this class to support arithmetic types overloading
         */
        struct arithmetic_t {};

        /**
         *  Inherit from this class if target class can be chained with other conditions with '&&' and '||' operators
         */
        struct condition_t {};

        /**
         *  Specialize if a type participates as an argument to overloaded operators (arithmetic, conditional, negation, chaining)
         */
        template<class T, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_operator_argument_v = false;

        template<class T>
        using is_operator_argument = polyfill::bool_constant<is_operator_argument_v<T>>;
    }
}
