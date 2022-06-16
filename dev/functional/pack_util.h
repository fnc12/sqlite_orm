#pragma once

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            template<template<class...> class R, class... Pack>
            struct flatten_types {
                using type = R<>;
            };

            template<template<class...> class R, template<class...> class Pack1, class... X>
            struct flatten_types<R, Pack1<X...>> {
                using type = R<X...>;
            };

            template<template<class...> class R,
                     template<class...>
                     class Pack1,
                     template<class...>
                     class Pack2,
                     class... X,
                     class... Y,
                     class... Pack>
            struct flatten_types<R, Pack1<X...>, Pack2<Y...>, Pack...> : flatten_types<R, pack<X..., Y...>, Pack...> {};

            template<template<class...> class R, class... Pack>
            using flatten_types_t = typename flatten_types<R, Pack...>::type;
        }
    }
}
