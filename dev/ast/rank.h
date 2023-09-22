#pragma once

namespace sqlite_orm {
    namespace internal {
        struct rank_t {};
    }

    inline internal::rank_t rank() {
        return {};
    }
}
