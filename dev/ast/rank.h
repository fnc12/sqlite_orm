#pragma once

namespace sqlite_orm {
    namespace internal {
        struct rank_t {};
    }
}

_EXPORT_SQLITE_ORM namespace sqlite_orm {
    inline internal::rank_t rank() {
        return {};
    }
}
