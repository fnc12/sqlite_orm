#pragma once

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/mpl.h"
#include "../../type_traits.h"
#include "../constraints/default.h"
#include "is_rowid_alias_capable.h"

namespace sqlite_orm::internal {
    /**
     *  COLUMN PRIMARY KEY INSERTABLE traits.
     *
     *  A column primary key is considered implicitly insertable if:
     *  - it is an INTEGER PRIMARY KEY (and thus an alias for the "rowid" key),
     *  - or has a default value.
     *
     *  In terms of C++ types, this means that the field type must be capable of representing a 64-bit signed integer,
     *  or the column is declared with a DEFAULT constraint.
     *
     *  Implementation note: using a struct template in favor of a template alias so that the stack leading to a deprecation message is shorter.
     */
    template<typename Column>
    struct is_pkcol_implicitly_insertable
        : mpl::invoke_t<
              mpl::disjunction<mpl::always<polyfill::bool_constant<is_rowid_alias_capable_v<field_type_t<Column>>>>,
                               check_if_has_template<default_t>>,
              constraints_type_t<Column>> {};
}
