#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  // std::is_integral
#endif

#include "functional/mpl/conditional.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Helper classes used by statement_binder and row_extractor.
     */
    struct int_or_smaller_tag {};
    struct bigint_tag {};
    struct real_tag {};

    template<class V>
    using arithmetic_tag_t =
        mpl::conditional_t<std::is_integral<V>::value,
                           // Integer class
                           mpl::conditional_t<sizeof(V) <= sizeof(int), int_or_smaller_tag, bigint_tag>,
                           // Floating-point class
                           real_tag>;
}
