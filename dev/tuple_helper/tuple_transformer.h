#pragma once

#include <tuple>  //  std::tuple

namespace sqlite_orm {
    namespace internal {

        template<class T, template<class...> class F>
        struct tuple_transformer;

        template<class... Args, template<class...> class F>
        struct tuple_transformer<std::tuple<Args...>, F> {
            using type = std::tuple<typename F<Args>::type...>;
        };

        template<class T, template<class...> class F>
        using transform_tuple_t = typename tuple_transformer<T, F>::type;

    }
}
