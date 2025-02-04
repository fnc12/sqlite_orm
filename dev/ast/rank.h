#pragma once

namespace sqlite_orm {
    namespace internal {
        struct rank_t {};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline internal::rank_t rank() {
        return {};
    }
}
