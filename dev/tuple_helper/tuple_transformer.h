#pragma once

#include "../functional/mpl.h"

namespace sqlite_orm {
    namespace internal {

        template<class Pack, template<class...> class Op>
        struct tuple_transformer;

        template<template<class...> class Pack, class... Types, template<class...> class Op>
        struct tuple_transformer<Pack<Types...>, Op> {
            using type = Pack<mpl::invoke_meta_t<Op, Types>...>;
        };

        /*
         *  Transform specified tuple.
         *  
         *  `Op` is a metafunction or metafunction operation.
         */
        template<class Pack, template<class...> class Op>
        using transform_tuple_t = typename tuple_transformer<Pack, Op>::type;
    }
}
