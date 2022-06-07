#pragma once

#include <tuple>  //  std::tuple

#include "../functional/mpl.h"

namespace sqlite_orm {
    namespace internal {

        template<class Tpl, template<class...> class Op>
        struct tuple_transformer;

        template<class... Types, template<class...> class Op>
        struct tuple_transformer<std::tuple<Types...>, Op> {
            using type = std::tuple<mpl::invoke_op_t<Op, Types>...>;
        };

        /*
         *  Transform specified tuple.
         *  
         *  `Op` is a metafunction operation.
         */
        template<class Tpl, template<class...> class Op>
        using transform_tuple_t = typename tuple_transformer<Tpl, Op>::type;
    }
}
