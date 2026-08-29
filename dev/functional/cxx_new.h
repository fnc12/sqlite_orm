#pragma once

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#else
#include <new>
#endif

namespace sqlite_orm::internal::polyfill {
#if __cpp_lib_hardware_interference_size >= 201703L
    using std::hardware_constructive_interference_size;
    using std::hardware_destructive_interference_size;
#else
    inline constexpr size_t hardware_constructive_interference_size = 64;
    inline constexpr size_t hardware_destructive_interference_size = 64;
#endif
}

namespace sqlite_orm {
    namespace polyfill = internal::polyfill;
}
