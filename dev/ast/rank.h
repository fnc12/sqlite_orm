#pragma once

namespace sqlite_orm {
    namespace internal {
        struct rank_t {};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    [[deprecated("Use the hidden FTS5 rank column instead")]]
    inline internal::rank_t rank() {
        return {};
    }
}
