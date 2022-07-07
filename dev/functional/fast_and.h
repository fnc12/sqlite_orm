#pragma once

#include "cxx_universal.h"
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
#include <type_traits>
#else
#include "cxx_type_traits_polyfill.h"
#endif

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
namespace sqlite_orm {
    namespace internal {

        template<bool... v>
        struct fast_and : std::is_same<fast_and<v...>, fast_and<(v, true)...>> {};
    }
}

#define SQLITE_ORM_FAST_AND(...) ::sqlite_orm::internal::fast_and<__VA_ARGS__::value...>::value
#else
#define SQLITE_ORM_FAST_AND(...) polyfill::conjunction<__VA_ARGS__...>::value
#endif
