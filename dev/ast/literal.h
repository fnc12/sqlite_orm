#pragma once

/** @file
 *  Purpose: protect an otherwise bindable element so that it is always serialized as a literal value.
 *
 *  Intention: a node header of `ast/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#include "../functional/cxx_polyfill/type_traits.h"
#include "../vocabulary/traits/structural_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct literal_holder {
        using type = T;

        type value;
    };

    template<class T>
    constexpr bool is_literal_v = polyfill::is_specialization_of_v<T, literal_holder>;
}
