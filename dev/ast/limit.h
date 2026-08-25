#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif  //  SQLITE_ORM_IMPORT_STD_MODULE

#include "../type_traits.h"
#include "../optional_container.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "offset.h"

namespace sqlite_orm::internal {
    /**
     *  Stores LIMIT/OFFSET info
     */
    template<class T, bool has_offset, bool offset_is_implicit, class O>
    struct limit_t {
        static constexpr bool has_offset_v = has_offset;
        static constexpr bool offset_is_implicit_v = offset_is_implicit;

        using expression_type = T;
        using offset_expression_type = O;

        expression_type limit;
        optional_container<O> offset;
    };

    template<class T, bool has_offset, bool offset_is_implicit, class O>
    constexpr bool is_limit_v<limit_t<T, has_offset, offset_is_implicit, O>> = true;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  LIMIT clause.
     *  Example: limit(10)
     *  @param limit The maximum number of rows to return.
     *  @return limit_t instance representing LIMIT clause.
     */
    template<class T>
    internal::limit_t<T, false, false, void> limit(T limit) {
        return {std::move(limit)};
    }

    /**
     *  LIMIT with OFFSET clause (implicit offset).
     *  Example: limit(5, 10) // OFFSET 5 LIMIT 10
     *  @param offset The offset value (number of rows to skip).
     *  @param limit The limit value (maximum number of rows to return).
     *  @return limit_t instance representing LIMIT and OFFSET clause.
     */
    template<class T, class O, internal::satisfies_not<internal::is_offset, T> = true>
    internal::limit_t<T, true, true, O> limit(O offset, T limit) {
        return {std::move(limit), {std::move(offset)}};
    }

    /**
     *  LIMIT with explicit OFFSET clause.
     *  Example: limit(10, offset(5))
     *  @param limit The limit value (maximum number of rows to return).
     *  @param offset The offset_t instance representing the offset.
     *  @return limit_t instance representing LIMIT and OFFSET clause.
     */
    template<class T, class O>
    internal::limit_t<T, true, false, O> limit(T limit, internal::offset_t<O> offset) {
        return {std::move(limit), {std::move(offset.offset)}};
    }
}
