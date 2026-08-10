#pragma once

namespace sqlite_orm::internal {
    template<class... Cs>
    struct primary_key_t;

    template<class G, class S>
    struct column_field;

    template<class... Op>
    struct column_constraints;

    template<class T, class F>
    struct column_pointer;

    template<class C>
    struct indexed_column_t;
}
