#pragma once

_EXPORT_SQLITE_ORM namespace sqlite_orm {
    namespace internal {
        struct rank_t {};
    }

    inline internal::rank_t rank() {
        return {};
    }
}
