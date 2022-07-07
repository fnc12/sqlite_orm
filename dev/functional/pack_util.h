#pragma once

#include "pack.h"
#include "mpl.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            /*
             *  Flatten specified type lists into specified result type list
             */
            template<template<class...> class R, class... List>
            struct flatten_types {
                using type = R<>;
            };

            template<template<class...> class R, template<class...> class List1, class... X>
            struct flatten_types<R, List1<X...>> {
                using type = R<X...>;
            };

            template<template<class...> class R,
                     template<class...>
                     class List1,
                     template<class...>
                     class List2,
                     class... X,
                     class... Y,
                     class... List>
            struct flatten_types<R, List1<X...>, List2<Y...>, List...> : flatten_types<R, pack<X..., Y...>, List...> {};

            template<template<class...> class R, class... List>
            using flatten_types_t = typename flatten_types<R, List...>::type;

            template<template<class...> class R, class List, template<class...> class Op>
            struct transform_types;

            template<template<class...> class R, template<class...> class List, class... X, template<class...> class Op>
            struct transform_types<R, List<X...>, Op> {
                using type = R<invoke_op_t<Op, X>...>;
            };

            /*
             *  Transform specified type list.
             *
             *  `Op` is a metafunction operation.
             */
            template<template<class...> class R, class List, template<class...> class Op>
            using transform_types_t = typename transform_types<R, List, Op>::type;
        }
    }
}
