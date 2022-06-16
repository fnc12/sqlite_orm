#pragma once
#include <type_traits>  //  std::is_empty, std::is_final

namespace _sqlite_orm {
    template<class T>
    constexpr bool is_ebo_able_v = std::is_empty<T>::value && !std::is_final<T>::value;
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            using _sqlite_orm::is_ebo_able_v;

            template<bool SameNumberOfElements, class Tuple, class... Y>
            struct enable_tuple_variadic_ctor;

            template<bool DefaultOrDirect, class Tuple, class... Void>
            struct enable_tuple_ctor;

            template<class Tuple, class Other, class... Void>
            struct enable_tuple_nonconst_copy_ctor;

            struct from_variadic_t {};

            template<class T>
            struct remove_rvalue_reference {
                using type = T;
            };
            template<class T>
            struct remove_rvalue_reference<T&&> {
                using type = T;
            };
            template<class T>
            using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

#ifdef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
            template<class Tpl>
            struct make_idxseq_helper {
                using type = std::make_index_sequence<std::tuple_size<Tpl>::value>;
            };
#endif
        }
    }

    namespace mpl = internal::mpl;
}
