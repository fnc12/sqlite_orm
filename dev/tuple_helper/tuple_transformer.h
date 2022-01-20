#pragma once

#include <tuple>  //  std::tuple

namespace sqlite_orm {
    namespace internal {

        /**
          *  T - table type.
          *  F - Unary metafunction that transforms the types in a tuple.
          *      (Note that multiple arguments are accepted to allow for
          *       metafunctions that need SFINAE specialization)
         */
        template<class T, template<class...> class F>
        struct tuple_transformer;

        template<class... Args, template<class...> class F>
        struct tuple_transformer<std::tuple<Args...>, F> {
            using type = std::tuple<typename F<Args>::type...>;
        };
    }
}
