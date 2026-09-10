#pragma once

/** @file The INTO clause naming the table a raw INSERT or REPLACE targets.
 */

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../table_reference.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct into_t {
        using type = T;
    };

    template<class T>
    constexpr bool is_into_v = polyfill::is_specialization_of<T, into_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    internal::into_t<T> into() {
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_table_reference auto table>
    auto into() {
        return into<internal::auto_decay_table_ref_t<table>>();
    }
#endif
}
