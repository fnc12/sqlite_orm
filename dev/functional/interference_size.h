#pragma once

namespace sqlite_orm::internal {
    /*
     *  Fixed per-architecture interference sizes used for anti-false-sharing alignment.
     *
     *  Deliberately not the `std::hardware_*_interference_size` constants: their values follow
     *  the compiler tuning flags (`-mtune`, `--param destructive-interference-size`), so a type
     *  layout depending on them can differ between translation units of one program - the very
     *  hazard GCC's default-on `-Winterference-size` warns about when they are used in a header.
     *
     *  The destructive size is an upper bound across the CPUs of an architecture: overestimating
     *  only costs a little memory, underestimating brings back false sharing.
     *  - AArch64: 256, GCC's generic value, chosen for the A64FX's 256-byte cache lines.
     *  - x86-64: 128, because the spatial prefetcher pulls in pairs of 64-byte lines (as folly does).
     */
#if defined(__aarch64__) || defined(_M_ARM64)
    inline constexpr size_t constructive_interference_size = 64;
    inline constexpr size_t destructive_interference_size = 256;
#elif defined(__x86_64__) || defined(_M_X64)
    inline constexpr size_t constructive_interference_size = 64;
    inline constexpr size_t destructive_interference_size = 128;
#else
    inline constexpr size_t constructive_interference_size = 64;
    inline constexpr size_t destructive_interference_size = 64;
#endif
}
