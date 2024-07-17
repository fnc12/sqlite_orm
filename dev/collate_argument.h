#pragma once

_EXPORT_SQLITE_ORM namespace sqlite_orm {

    namespace internal {

        enum class collate_argument {
            binary,
            nocase,
            rtrim,
        };
    }
}
