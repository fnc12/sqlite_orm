#pragma once

namespace sqlite_orm {
    namespace internal {
        struct current_time_t {};
        struct current_date_t {};
        struct current_timestamp_t {};
    }

    inline internal::current_time_t current_time() {
        return {};
    }

    inline internal::current_date_t current_date() {
        return {};
    }

    inline internal::current_timestamp_t current_timestamp() {
        return {};
    }
}