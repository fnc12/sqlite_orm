#pragma once

#include <tuple>  //  std::tuple

namespace sqlite_orm {
    namespace internal {

        template<class T, template<class C> class F>
        struct tuple_transformer;

        template<class... Args, template<class C> class F>
        struct tuple_transformer<std::tuple<Args...>, F> {
            using type = std::tuple<typename F<Args>::type...>;
        };
    }
}
