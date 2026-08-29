# Drawing the C++17 Standard Library base line

Date: 2026-08-28
Branch base: `dev`

## Goal

The compiler base line was drawn deliberately (#1433, #1465, #1469); the library side was left
open just as deliberately, because a C++17 compiler still turns up on a pre-C++17 stdlib. The cost
of that split is visible in `fdc33d69` — *"As long as we don't require a C++17 Standard Library
base line we cannot assume that variable templates are available"* — and in ~72 guarded sites
(`SQLITE_ORM_OPTIONAL_SUPPORTED`, `SQLITE_ORM_STRING_VIEW_SUPPORTED`) plus the polyfills.

This asks one question: draw the library line **now**, or keep waiting for the C++20 line where
`<version>` is guaranteed and the probe collapses to a plain `#include <version>`.

## Enforcement

Landed on this branch in `dev/functional/cxx_check_prerequisites.h` as a separate, droppable commit,
so that CI exercises the base line across the matrix:

```cpp
#if __has_include(<version>)
#include <version>
#elif __has_include(<ciso646>)
#include <ciso646>  // pre-C++20 idiom: pulls in the stdlib's config header
#endif

#if (__cpp_lib_optional < 201606L || __cpp_lib_string_view < 201606L ||
     __cpp_lib_type_trait_variable_templates < 201510L || __cpp_lib_logical_traits < 201510L ||
     __cpp_lib_invoke < 201411L || __cpp_lib_apply < 201603L)
#error A C++17 standard library is required (GCC 9+, libc++ 8+, VS 2017 15.9+).
#endif
```

Both headers are macro-only, so this holds for the `SQLITE_ORM_IMPORT_STD_MODULE` build. No
`__cplusplus >= 202002L` guard: `<version>` ships independently of the language mode and populates
itself for the active `-std`, and `__cplusplus` is 199711L on MSVC without `/Zc:__cplusplus`.
`<ciso646>` is unreachable wherever `<version>` exists.

## Base line this buys

| stdlib | branch taken | first version that passes | ruled out, and how |
|---|---|---|---|
| libstdc++ | `<version>` (since GCC 9) | **GCC 9** (`_GLIBCXX_RELEASE >= 9`) | GCC 7-8 fall to `<ciso646>` → `bits/c++config.h` → **0** `__cpp_lib_*` → error, though those libs do have `std::optional` |
| libc++ | `<version>` (since LLVM 7) | **libc++ 8** (`_LIBCPP_VERSION >= 8000`, ~Xcode 11) | libc++ 7's `<version>` is an **empty stub** → passes `__has_include`, defines nothing → error. libc++ ≤ 6 → `<ciso646>` → `__config` → same |
| MSVC STL | `<ciso646>` ≤ VS 2017 15.9; `<version>` from VS 2019 | **VS 2017 15.9** (`_MSC_VER >= 1916`) | VS 2015, early VS 2017. 15.9's `yvals_core.h` already defines the full C++17 set |

Verified against local MSVC 14.16/14.29/14.44, libstdc++ 13 + 15 (`-E -dM` macro dumps), and the
libc++/libstdc++ release tags. Below the CI matrix (g++-10, clang-12, windows-2022, macos-14).

## Caveats

* **The cut is "C++17 stdlib *with a populated `<version>`*"**, which is stricter than the stated
  goal. GCC 7-8 and libc++ 7 are collateral: they do provide the features and would pass a
  per-header test, but not an up-front one. That is precisely the population the current policy
  protects, and it is the substance of the decision below.
* **`<ciso646>` is what keeps VS 2017 alive.** Only branch that works there, and
  `cxx_compiler_quirks.h:50-56` still carries `_MSC_VER < 1920` workarounds. Dropping it raises the
  MSVC floor to VS 2019.
* **`<ciso646>` yields no `__cpp_lib_*` on libstdc++/libc++** — their config headers carry version
  macros only; only MSVC puts feature-test macros in `yvals_core.h`. On GCC/Clang the check
  degenerates to "no `<version>` → error", hence the `#error` names versions.
* **Language-mode gated** (`_HAS_CXX17`, `__cplusplus >= 201703L`, `_LIBCPP_STD_VER > 14`), so a
  C++14 build trips the same error — intended, and it catches MSVC users who never passed
  `/std:c++17`.
* **Apple deployment target is out of reach of any macro test.** libc++ defines
  `__cpp_lib_optional` regardless of `-mmacosx-version-min`, but `bad_optional_access` lives in the
  OS dylib: below macOS 10.14 / iOS 12 the guards this would delete are load-bearing. Document,
  don't detect.
* An exact alternative — `#include <optional>` etc. in the prerequisites header, then test — has no
  collateral, but pulls the stdlib in ahead of everything and conflicts with the
  `SQLITE_ORM_IMPORT_STD_MODULE` build. Rejected.

## Decision

**Drawn now**, at **GCC 9 / libc++ 8 (Xcode 11) / VS 2017 15.9**; GCC 7-8 and libc++ 7 are spent.
Agreed in the team on 2026-08-29.

## Follow-through

* Enforcement block in `dev/functional/cxx_check_prerequisites.h` (see above); `<version>` dropped from
  `dev/functional/config.h`, which reaches the macros through `cxx_universal.h`.
* `polyfill::void_t`, `bool_constant`, the logical traits, `invoke`, `is_invocable` and `apply` removed;
  call sites name the standard facilities. `functional/cxx_tuple_polyfill.h` deleted.
  `polyfill` keeps only what is genuinely post-C++17: `remove_cvref`, `type_identity`, `identity`,
  `unwrap_reference`, plus the never-standardised `is_detected` / `is_specialization_of`.
* `SQLITE_ORM_OPTIONAL_SUPPORTED` and `SQLITE_ORM_STRING_VIEW_SUPPORTED` removed along with their
  fallbacks (`std::string`-based `serialize_result_type`, the `strlen`/`wcslen` binder overloads,
  `nullif()` without a common return type). `functional/cxx_optional.h` and
  `functional/cxx_string_view.h` deleted; users include `<optional>` / `<string_view>` directly.
* The `fdc33d69` concessions are reverted: `std::tuple_size_v`, `std::is_same_v`, `std::is_void_v`,
  `std::is_base_of_v`, `std::is_empty_v`, `std::is_default_constructible_v` and `std::is_function_v`
  are used again.
* `README.md` names the versions instead of "C++17 compatible compiler", and records the
  macOS 10.14 / iOS 12 deployment target.

Still open: `cxx_compiler_quirks.h` keeps its `_MSC_VER < 1920` workarounds — VS 2017 15.9 stays in
scope, so those are unrelated to this line and are not touched here.
