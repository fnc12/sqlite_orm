#pragma once

/** @file Declarations of closed predicates for checking the validity of fields of column nodes.
 */

#include "../../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {
    template<class F, class SFINAE = void>
    extern const bool is_rowid_alias_capable_v;

    template<class F>
    using is_rowid_alias_capable = polyfill::bool_constant<is_rowid_alias_capable_v<F>>;
}
