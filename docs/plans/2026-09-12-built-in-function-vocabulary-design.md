# Built-in function vocabulary design

Date: 2026-09-12
Branch base: `feature/defaulted-return-type-factories`

## Goal

Replace the per-function factory + `*_string` tag pair in `dev/core_functions.h`
(`lower()` + `lower_string` → `built_in_function_t<R, S, Args...>`) with one
generic definition mechanism, so a built-in SQLite function is a single
`inline constexpr` line stating its name and its signature set:

```cpp
inline constexpr orm_built_in_function auto lower = "LOWER"_builtin.scalar<std::string(std::string_view)>();
```

The mechanism mirrors the quoted user-defined function pipeline in
`dev/function.h` — `_scalar` → `quoted_function_builder` →
`quoted_scalar_function` — minus the callable, which a built-in does not
have.

Phase 1 (this document) covers scalar built-ins, including overload sets and
open-ended arity, and ports `lower` (single signature), `substr` (overload
set) and its alias `substring` to prove the pattern. Everything else stays on
`built_in_function_t` until ported.

## Decisions

1. **Scope: scalars with overload sets.** A definition takes one or more
   C++ function signatures; the overload is picked by call arity. Aggregates
   (`.filter()`/`.over()`) and `nullable_result_proxy` return types are
   follow-ups, but nothing in the naming closes the door on them (see
   decision 4).

2. **Signature spelling.** `.scalar<Sig...>()` takes a variadic pack of
   function types. Open-ended arity is expressed by the marker
   `sqlite_orm::variadic<T>` in the last parameter position, meaning "zero or
   more further `T`": the parameters before it are required, so minimum
   arity falls out naturally.

   ```cpp
   "SUBSTR"_builtin.scalar<std::string(std::string_view, int), std::string(std::string_view, int, int)>()
   "MAX"_builtin.scalar<double(double, double, variadic<double>)>()
   ```

   C varargs (`R(A, ...)`) were rejected: no element type, and would need a
   new `function_traits` specialization. A template parameter pack cannot
   appear inside a concrete function type at all.

   Parameter types are nominal today — no callable receives them, and call
   arguments are not checked against them (same as UDFs). Prefer
   `std::string_view` for text parameters; it says "text-ish, non-owning".
   The **return type is not nominal**: it is what `column_result_t` reports,
   so it must be an owning type (`std::string`, never `string_view`).

3. **Value, not type-level.** `built_in_scalar_function<N, Sigs...>` stores
   the name as `char _nme[N]`, exactly like `quoted_scalar_function`. Making
   the name an NTTP was considered (static `name()`, empty definition type)
   and dropped for symmetry with the UDF side.

   Consequence: the call node copies the definition object (and with it the
   name) rather than pointing into it, because a built-in defined as a local
   `constexpr` variable would otherwise leave nodes dangling. The member is
   `SQLITE_ORM_NOUNIQUEADDRESS`.

4. **Names keep the aggregate path open.** The literal `_builtin` is kind
   neutral; the kind is chosen by the builder method: `.scalar<Sigs...>()`
   now, `.aggregate<Sigs...>()` later. (`quote` was rejected as the method
   name — there is no callable to quote.) Types: `built_in_scalar_function`
   (definition), `built_in_function_call` (node); a later
   `built_in_aggregate_function` sits next to the legacy
   `built_in_aggregate_function_t` until the port is complete.

5. **`_builtin` is internal.** sqlite_orm defines all built-ins itself, so
   the literal operator lives in `namespace sqlite_orm::internal`, not in
   `sqlite_orm::literals`. The definitions are written in
   `sqlite_orm::internal`, where unqualified lookup finds the literal, and
   published in `sqlite_orm` as copies:

   ```cpp
   namespace sqlite_orm::internal {
       inline constexpr auto lower = "LOWER"_builtin.scalar<std::string(std::string_view)>();
   }
   SQLITE_ORM_EXPORT namespace sqlite_orm {
       inline constexpr orm_built_in_function auto lower = internal::lower;
   }
   ```

   A using-declaration would read better but is ill-formed in the module
   build: an exported using-declaration must name an entity with external
   linkage ([module.interface]/3), and unexported names in the module purview
   only have module linkage. `using namespace sqlite_orm;` in user code does
   not pick up `_builtin`, because `sqlite_orm` never nominates `internal`.

6. **Trait integration, not type piggy-backing.** The new node specializes
   `is_built_in_function_v` (and `is_operator_argument_v`) and provides the
   three members the trait's consumers use — `serialize()`, `args`,
   `return_type`. `statement_serializer`, `column_result_t`, `ast_iterator`
   and `node_tuple` program against the trait and need no edits.
   `built_in_function_t` the type is not touched or reused.

7. **Ported functions switch over under `#ifdef`.** In
   `SQLITE_ORM_WITH_CPP20_ALIASES` builds `lower`/`substr`/`substring` are the
   new definitions; the `#else` branch keeps the `*_string` tag and the old
   factories verbatim. Existing call sites and tests are unchanged and become
   the regression suite.

## Shape

`dev/ast/built_in_function.h`, whole body under `SQLITE_ORM_WITH_CPP20_ALIASES`:

```cpp
namespace sqlite_orm {
    template<class T> struct variadic {};

    template<class F>
    concept orm_built_in_function = requires(const F& f) {
        { f.name() } -> std::convertible_to<std::string_view>;
        typename std::remove_cvref_t<F>::signature_tuple;
    };
}

namespace sqlite_orm::internal {
    template<class F, class Sig, class... CallArgs>
    struct built_in_function_call : arithmetic_t {
        using function_type = F;
        using signature_type = Sig;              // the matched overload
        using return_type = function_return_type_t<Sig>;
        using args_type = std::tuple<CallArgs...>;

        SQLITE_ORM_NOUNIQUEADDRESS F function;
        args_type args;

        std::string_view serialize() const;      // function.name()
    };

    template<size_t N, orm_function_sig... Sigs>
    struct built_in_scalar_function {
        using signature_tuple = std::tuple<Sigs...>;
        template<class... CallArgs> constexpr auto operator()(CallArgs...) const;
        constexpr std::string_view name() const;
        consteval built_in_scalar_function(const char (&name)[N]);
        char _nme[N];
    };

    template<size_t N>
    struct built_in_function_builder : cstring_literal<N> {
        template<orm_function_sig... Sigs> requires (sizeof...(Sigs) > 0)
        [[nodiscard]] consteval auto scalar() const;
    };

    template<built_in_function_builder builder>
    [[nodiscard]] consteval auto operator""_builtin() { return builder; }
}
```

Overload matching runs once in `operator()`: first `Sig` whose parameter
count equals the call arity, or — when the last parameter is `variadic<T>` —
whose fixed parameter count is `<=` the arity. `operator()` is constrained
by a `requires` clause on a match existing, so a wrong arity is a
"no matching call" error and `std::is_invocable_v` can observe the rejection
in tests; the return type of the match becomes the node's `return_type`.

## Files

- New `dev/ast/built_in_function.h`; registered in `dev/node_definitions.h`.
- `dev/core_functions.h`: include it; `lower`, `substr` and `substring` per decision 7.
- Tests, all C++20-gated: `tests/static_tests/built_in_function_static_tests.cpp`
  (matching, `variadic`, `return_type`, traits, concept, rejected arity);
  a serializer section with a locally defined `variadic` built-in proving the
  name serializes bare; the pre-existing `lower`/`substr` tests cover the rest.
- `include/sqlite_orm/sqlite_orm.h` is regenerated by the maintainer.

## Follow-ups

- Port the remaining scalar built-ins; delete their `*_string` tags and
  factories from the C++20 branch as they go.
- `.aggregate<Sigs...>()` + an aggregate call node with `.filter()`/`.over()`.
  Blocks `max`/`min`: their one-argument overload *is* the aggregate and shares
  the name with the scalar, so the scalar can't be ported alone.
- `nullable_result_proxy`-style "result type follows the first argument"
  return types (`abs`, `max`, `min`, ...), which need a return-type hook the
  plain signature cannot express.
- Caller-chosen return types. The math and some json factories take the
  return type as a defaulted template argument
  (`acos<std::optional<double>>(x)`); a variable cannot take explicit template
  arguments, so porting them needs a spelling such as `acos.as<R>(x)` or a
  separate hook — decide before touching them.
- Name clashes inside `sqlite_orm::internal` as more built-ins are defined
  there (`max`, `min`, `count`, ...). Class members shadow them, but a
  namespace-scope internal helper of the same name would not; a nested
  `internal::builtins` namespace is the escape hatch if it ever bites.
- Optional: argument-type checking against the nominal parameter types.
