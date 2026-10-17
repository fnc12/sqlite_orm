---
name: add-builtin-function
description: Use when adding one of SQLite's built-in SQL functions (core, date/time, aggregate, math, JSON, window) to sqlite_orm. Covers the C++20 definition by name and overload set, the legacy C++17 node, the public declaration, guards and tests.
---

# Adding a built-in SQL function

Built-in functions are definitions, not nodes: one header per family as SQLite documents them, in
`dev/builtin/functions/` (`core.h`, `datetime.h`, `aggregate.h`, `math.h`, `json.h`, `window.h`).
Their call nodes live in `dev/ast/builtin_function.h`. The design is in
`docs/plans/2026-09-12-built-in-function-vocabulary-design.md`. `UNISTR` in `core.h` is a compact
example to compare with.

Each function exists twice, in two blocks of the family's header, until the C++17 baseline is
dropped:

## 1. C++20: the definition (`#else` block of `#ifndef SQLITE_ORM_WITH_CPP20_ALIASES`)

- In `namespace sqlite_orm::internal`, an `inline constexpr` definition of name plus overload set:
  `inline constexpr auto unistr = "UNISTR"_builtin.scalar<std::string(std::string_view)>();`
  Use `.aggregate<...>()` or `.window<...>()` for those kinds, `.function<...>()` for a function of
  several kinds, and list one signature per overload.
- Argument-dependent return types use the placeholders `argument<I>` and
  `common_argument_type<I...>` (`dev/vocabulary/algorithms/argument_placeholders.h`), e.g.
  `std::unique_ptr<argument<0>>(anything)`. A nullable result is a `std::unique_ptr<...>`.
- In `SQLITE_ORM_EXPORT namespace sqlite_orm`, publish it with a comment naming the SQL function and
  linking SQLite's documentation:
  `inline constexpr orm_builtin_function auto unistr = internal::unistr;`
  Wrap it in a function template only where the public function takes the return type as a
  template argument or checks its arguments.

## 2. C++17: the legacy node (`#ifndef SQLITE_ORM_WITH_CPP20_ALIASES` block)

- A name tag in `namespace sqlite_orm::internal`, next to the others:
  `struct unistr_string { std::string_view serialize() const { return "UNISTR"; } };`
- A public factory returning `internal::builtin_function_t<R, internal::unistr_string, X>`
  (`builtin_aggregate_function_t` for aggregates), with the same comment as the C++20 declaration.

## 3. Guards

Guard both definitions the same way when the function depends on the SQLite version
(`#if SQLITE_VERSION_NUMBER >= 3050000`) or on a compile option (`#ifdef SQLITE_ENABLE_PERCENTILE`).
Guard the contents, never an `#include`.

## 4. Tests

Guard each test with the same condition as the function.

- Serialization: `tests/statement_serializer_tests/<family>_functions.cpp`, a `SECTION` per
  function.
- Return types: `tests/static_tests/core_function_return_types.cpp` or
  `aggregate_function_return_types.cpp`, for argument-dependent return types.
- Execution: `tests/builtin_functions_tests/` (`core_functions_tests.cpp`,
  `datetime_function_tests.cpp`, `math_functions.cpp`), a `TEST_CASE` selecting the function from a
  storage.

## 5. Finish

Run `scripts/pre-commit.sh`, and have the change reviewed against
`docs/internals/review-checklist.md`.
