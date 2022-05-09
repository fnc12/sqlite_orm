#pragma once
#include <type_traits>

namespace sqlite_orm {

    /**
     *  Helper classes used by statement_binder and row_extractor.
     */
    struct int_or_smaller_tag {};
    struct bigint_tag {};
    struct real_tag {};

    template<class V>
    using arithmetic_tag_t =
        std::conditional_t<std::is_integral<V>::value,
                           // Integer class
                           std::conditional_t<sizeof(V) <= sizeof(int), int_or_smaller_tag, bigint_tag>,
                           // Floating-point class
                           real_tag>;
}
