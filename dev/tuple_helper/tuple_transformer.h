#pragma once

#include <type_traits>  //  std::index_sequence, std::make_index_sequence
#include <tuple>  //  std::tuple

namespace sqlite_orm {
    namespace internal {

        /*
         *  Implementation note for tuple_transformer:
         *  tuple_transformer is implemented in terms of index sequences instead of argument packs,
         *  otherwise type replacement may fail for legacy compilers
         *  if alias template `Op` has a dependent expression in it [SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION].
         */

        template<class Tpl,
                 template<class...>
                 class Op,
                 class Seq = std::make_index_sequence<std::tuple_size<Tpl>::value>>
        struct tuple_transformer;

        template<class Tpl, template<class...> class Op, size_t... Idx>
        struct tuple_transformer<Tpl, Op, std::index_sequence<Idx...>> {
            using type = std::tuple<Op<std::tuple_element_t<Idx, Tpl>>...>;
        };

        /*
         *  Transform specified tuple.
         *  
         *  `Op` is a metafunction operation.
         */
        template<class Tpl,
                 template<class...>
                 class Op,
                 class Seq = std::make_index_sequence<std::tuple_size<Tpl>::value>>
        using transform_tuple_t = typename tuple_transformer<Tpl, Op, Seq>::type;
    }
}
