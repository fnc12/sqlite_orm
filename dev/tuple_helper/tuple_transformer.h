#pragma once

#include <tuple>  //  std::tuple

namespace sqlite_orm {
    namespace internal {

        template<class T, template<class...> class Op>
        struct tuple_transformer;

        template<class... Args, template<class...> class Op>
        struct tuple_transformer<std::tuple<Args...>, Op> {
            using type = std::tuple<Op<Args>...>;
        };

        template<class T, template<class...> class Fn>
        using transform_tuple_t = typename tuple_transformer<T, Fn>::type;

    }
}
