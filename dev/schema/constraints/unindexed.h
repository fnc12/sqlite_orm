#pragma once

namespace sqlite_orm::internal {
    struct unindexed_t {};
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
    /**
     *  UNINDEXED column constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#the_unindexed_column_option
     */
    constexpr internal::unindexed_t unindexed() {
        return {};
    }
#endif
}
