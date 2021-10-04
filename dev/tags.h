#pragma once

namespace sqlite_orm {
    namespace internal {
        struct negatable_t {};

        /**
         *  Inherit from this class if target class can be chained with other conditions with '&&' and '||' operators
         */
        struct condition_t {};
    }
}
