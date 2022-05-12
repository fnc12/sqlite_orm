#pragma once

#include <tuple>  //  std::tuple

namespace sqlite_orm {
    namespace internal {

        template<class T, template<class...> class Fn>
        struct tuple_transformer;

        template<class... Args, template<class...> class Fn>
        struct tuple_transformer<std::tuple<Args...>, Fn> {
            using type = std::tuple<typename Fn<Args>::type...>;
        };

        template<class T, template<class...> class Fn>
        using transform_tuple_t = typename tuple_transformer<T, Fn>::type;

    }
}
