#pragma once

#include <tuple>  //  std::tuple

#include "../functional/pack_util.h"

namespace sqlite_orm {
    namespace internal {

        template<class Tpl, template<class...> class Op>
        struct tuple_transformer : mpl::transform_types<std::tuple, Tpl, Op> {};

        /*
         *  Transform specified tuple.
         *  
         *  `Op` is a metafunction operation.
         */
        template<class Tpl, template<class...> class Op>
        using transform_tuple_t = mpl::transform_types_t<std::tuple, Tpl, Op>;
    }
}
