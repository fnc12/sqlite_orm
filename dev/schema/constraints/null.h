#pragma once

namespace sqlite_orm::internal {
    struct null_t {};

    struct not_null_t {};
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    constexpr internal::null_t null() {
        return {};
    }

    constexpr internal::not_null_t not_null() {
        return {};
    }
}
