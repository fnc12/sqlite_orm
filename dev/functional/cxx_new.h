#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <cstddef>  //  size_t
#endif

namespace sqlite_orm::internal::polyfill {
    /*
     *  Fixed per-architecture cache-line sizes used for anti-false-sharing alignment.
     *
     *  Deliberately not the `std::hardware_*_interference_size` constants: their values follow
     *  the compiler tuning flags (`-mtune`, `--param destructive-interference-size`), so a type
     *  layout depending on them can differ between translation units of one program - the very
     *  hazard GCC's default-on `-Winterference-size` warns about when they are used in a header.
     */
#if defined(__aarch64__) || defined(_M_ARM64)
    inline constexpr size_t hardware_constructive_interference_size = 64;
    inline constexpr size_t hardware_destructive_interference_size = 128;
#else
    inline constexpr size_t hardware_constructive_interference_size = 64;
    inline constexpr size_t hardware_destructive_interference_size = 64;
#endif
}

namespace sqlite_orm {
    namespace polyfill = internal::polyfill;
}
