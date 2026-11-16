#pragma once

namespace sqlite_orm::internal::mpl {
    /*
     *  Binary quoted metafunction selecting the first or the second type,
     *  i.e. the ternary operator with its condition fixed.
     *
     *  Because `ternary` is only typed on a single bool non-type template parameter,
     *  the compiler only ever needs to memoize 2 instances of this class template.
     *  The type selection is a nested cheap alias template.
     */
    template<bool>
    struct ternary {
        template<typename A, typename>
        using fn = A;
    };

    template<>
    struct ternary<false> {
        template<typename, typename B>
        using fn = B;
    };

    // select A or B by a boolean constant
    template<bool v, typename A, typename B>
    using ternary_c = typename ternary<v>::template fn<A, B>;

    // select A or B by the value of a boolean trait
    template<class C, typename A, typename B>
    using ternary_t = typename ternary<bool(C::value)>::template fn<A, B>;
}

namespace sqlite_orm {
    namespace mpl = internal::mpl;
}
