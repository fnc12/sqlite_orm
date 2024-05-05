#pragma once

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            /*
             *  Binary quoted metafunction equivalent to `std::conditional`,
             *  using an improved implementation in respect to memoization.
             *  
             *  Because `conditional` is only typed on a single bool non-type template parameter,
             *  the compiler only ever needs to memoize 2 instances of this class template.
             *  The type selection is a nested cheap alias template.
             */
            template<bool>
            struct conditional {
                template<typename A, typename>
                using fn = A;
            };

            template<>
            struct conditional<false> {
                template<typename, typename B>
                using fn = B;
            };

            // directly invoke `conditional`
            template<bool v, typename A, typename B>
            using conditional_t = typename conditional<v>::template fn<A, B>;
        }
    }

    namespace mpl = internal::mpl;
}
