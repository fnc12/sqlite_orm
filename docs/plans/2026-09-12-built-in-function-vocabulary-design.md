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
   `internal::variadic<T>` in the last parameter position, meaning "zero or
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
    template<class F>
    concept orm_built_in_function = requires(const F& f) {
        { f.name() } -> std::convertible_to<std::string_view>;
        typename std::remove_cvref_t<F>::signature_tuple;
    };
}

namespace sqlite_orm::internal {
    template<class T> struct variadic {};

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
whose fixed parameter count is `<=` the arity. On no match the
`matched_built_in_signature_t` alias is ill-formed (no sentinel type), and
`operator()` is constrained by a type requirement on it, so a wrong arity is
a "no matching call" error and `std::is_invocable_v` can observe the
rejection in tests; the return type of the match becomes the node's
`return_type`.

## Files

- New `dev/ast/built_in_function.h`; registered in `dev/node_definitions.h`.
- `dev/core_functions.h`: include it; `lower`, `substr` and `substring` per decision 7.
- Tests, all C++20-gated: `tests/static_tests/built_in_function_static_tests.cpp`
  (matching, `variadic`, `return_type`, traits, concept, rejected arity);
  a serializer section with a locally defined `variadic` built-in proving the
  name serializes bare; the pre-existing `lower`/`substr` tests cover the rest.
- `include/sqlite_orm/sqlite_orm.h` is regenerated by the maintainer.

## Phase 2 — aggregates and argument-dependent return types (2026-09-13)

Ports `max`/`min`, which need three things phase 1 could not say: an
aggregate kind, two kinds under one name, and a result type that follows the
first argument.

### Decisions

8. **Kind per signature.** A definition is `built_in_function<N, KindedSigs...>`
   where each element is `scalar_sig<Sig>` or `aggregate_sig<Sig>`, in any
   order. The builder's `.function<KindedSigs...>()` takes them as is;
   `.scalar<Sigs...>()` / `.aggregate<Sigs...>()` remain as sugar for the
   single-kind case, so phase-1 definitions read unchanged.

   ```cpp
   inline constexpr auto lower = "LOWER"_builtin.scalar<std::string(std::string_view)>();
   inline constexpr auto max = "MAX"_builtin.function<
       aggregate_sig<std::unique_ptr<argument<0>>(anything)>,
       scalar_sig<std::unique_ptr<argument<0>>(anything, anything, variadic<anything>)>>();
   ```

   Rejected: chaining `.scalar<>().aggregate<>()` on the definition (the
   callable definition would have to host builder methods — no CTAD trick
   avoids that), and a combiner over two complete definitions (three
   definition types, name stored twice).

   Matching is unchanged — first signature accepting the arity, in declaration
   order — and the kind of the match picks the node.

9. **Aggregate call node.** `built_in_aggregate_function_call` derives from
   `built_in_function_call` and adds the legacy `.filter(where)` → 
   `filtered_aggregate_function<Self, W>` and `.over(...)` → `over_t<Self, ...>`.
   Both wrappers are already generic over the wrapped function node, so the
   serializer, `column_result_t`, `ast_iterator` and `node_tuple` need
   nothing. The exact-match traits (`is_built_in_function_v`,
   `is_operator_argument_v`) get a second specialization — inheritance does
   not classify. No `is_built_in_aggregate_function` trait until something
   consumes one.

10. **Return-position placeholder `argument<I>`.** May appear anywhere inside
    the return type; `column_result_t` substitutes it structurally with the
    column result of the I-th call argument: `argument<I>` → resolved,
    `Tmpl<Ts...>` → `Tmpl<subst<Ts>...>`, else unchanged. So
    `std::unique_ptr<argument<0>>` states MAX's result honestly and
    `std::optional<argument<1>>` works for free. Index-based rather than
    `first_argument`, because `coalesce`/`ifnull`/`nullif` want the same.
    The structural walk is schema-agnostic (it takes the resolver as a quoted
    metafunction) and lives in `vocabulary/algorithms/`; `column_result.h`
    binds `column_result_of_t<DBOs, _>` into it. `nullable_result_proxy` stays
    for the legacy nodes until their last user is ported.

11. **`anything` parameter marker.** Parameter types are nominal and can never
    be checked at the call site (the argument's SQL type needs the schema/CTE
    context that only `storage_t` has), so they document the SQL contract.
    Where that contract is genuinely polymorphic — MAX/MIN compare by the
    first argument's collation and accept text and blobs, likewise
    `coalesce`, `typeof`, `length`, ... — the slot is spelled `anything`.
    Concrete affinities keep concrete C++ types (`std::string_view`, `int`,
    `double`). No SQL-affinity vocabulary for parameters: nothing consumes it.
    A library tag rather than `std::any`, so the three signature markers
    (`variadic<T>`, `argument<I>`, `anything`) live together and nothing
    pulls `<any>` in. All three are `internal`: they are only ever spelled
    next to the internal `_builtin` literal, so nothing public needs them.

### Files

- `dev/ast/built_in_function.h`: `anything`, `scalar_sig`, `aggregate_sig`,
  `built_in_function` (renamed from `built_in_scalar_function`),
  `built_in_aggregate_function_call`, builder `.function<>()`.
  `filtered_aggregate_function` is forward-declared there; its definition
  stays in `core_functions.h`.
- `dev/vocabulary/algorithms/argument_placeholders.h`: the placeholders
  themselves (`argument<I>`, later `common_argument_type<I...>` —
  they are the substitution's vocabulary, not nodes, so no `node_fwd.h`
  entry) and the structural substitution; registered in
  `vocabulary/node_algorithms.h`.
- `dev/column_result.h`: the single built-in branch applies it (see phase 4
  for the argument resolver). `built_in_function_t` and
  `built_in_aggregate_function_t` are C++17-only and retire with the
  baseline.
- `dev/core_functions.h`: `max`/`min` per decision 7; `max_string`/`min_string`
  and the four legacy factories move into the C++17 branch.
- Tests: kinded dispatch, placeholder substitution through `column_result_t`,
  `anything`/`variadic<anything>` arity, `.filter()`/`.over()` node types. The
  local `"MAX"_builtin` fixtures are replaced by the real `max`. Existing
  `max`/`min` runtime, serializer, `ast_iterator` and
  `aggregate_function_return_types` tests are the regression suite.

## Phase 3 — caller-chosen return types (2026-09-13)

49 factories take the return type as a defaulted template argument
(`acos<std::optional<double>>(x)`). That spelling is syntax only functions
have: an explicit template argument on a type is a functional cast with CTAD
off, and "explicit `R`, deduce the call arguments" (partial CTAD) is not in
the language, so no object — variable, CPO, alias template — can be both
`acos<R>(x)` and `acos(x)` while carrying the argument types. Reflection does
not help either: an alias is transparent to the type it names, and P2996
cannot synthesize functions or alias templates. Any object-based spelling
(`acos.as<R>(x)`, `as_result<R>(acos(x))`) is therefore a breaking change
needing agreement and a transition path.

### Decision

12. **Function template facade over a definition object.** For these
    functions the definition object is internal and the public name stays
    the function template it is today, so `acos(x)` and `acos<R>(x)` are
    unchanged in every standard mode:

    ```cpp
    namespace sqlite_orm::internal {
        inline constexpr auto acos = "ACOS"_builtin.scalar<double(double)>();
    }
    SQLITE_ORM_EXPORT namespace sqlite_orm {
        template<class R = double, class X>
        constexpr auto acos(X x) {
            return internal::acos.template operator()<R>(std::move(x));
        }
    }
    ```

    `built_in_function::operator()` takes the return type as a leading,
    defaulted template parameter (`R = void` keeps the declared one), so an
    explicit `operator()<R>(args...)` yields the ordinary call node with
    signature `R(Params...)` (`with_return_type_t`); a placeholder in `R` is
    substituted like a declared one. A single template rather than a second
    overload, so an explicit `<R>` can never be taken for the first call
    argument's type by pack extension.

    Three lines per function instead of a tag struct and a factory; the
    "C++20 way" is the implementation for these, not the surface. A generic
    `as_result<R>(expr)` node — the natural successor, working on any
    expression — is additive and can be introduced whenever wanted; retiring
    the `<R>` parameters afterwards is a separate decision for the maintainers.

`acos` was ported as the proof; the rest followed in phase 4.

## Phase 4 — the remaining port (2026-09-13)

Every built-in in `core_functions.h` is now defined by the mechanism in
C++20 builds; the `*_string` tags and legacy factories live only in the
C++17 branch (`count_string` stays, `count(*)` is `count_asterisk_t` and
untouched). Two additions were needed on the way:

13. **`common_argument_type<I...>` placeholder.** `coalesce`, `ifnull`, `nullif`,
    `iif` and `if_` compute `std::common_type` of (some of) their arguments.
    `common_argument_type<1, 2>` and, for the open-ended
    `coalesce`, `common_argument_type<>` express that in the return
    position; `argument_placeholders.h` substitutes and then applies
    `std::common_type`.

    **Argument resolution (both standards).** The legacy factories resolved
    arguments at the call site with `field_type_or_type_t` — a member
    pointer's field type, anything else as itself — because no schema is
    available there; `max`/`min`'s `nullable_result_proxy` resolved through
    `column_result_t` instead. The placeholders resolve in `column_result_t`
    with `argument_result_of_t`: a bindable value stands for itself (so enum
    columns with custom binders, blob literals and other user types keep
    working exactly as before), except a text value (`is_text_value`: narrow
    or wide C string, string view or string), which yields `std::string` as a
    select of it does (so `max("a", "b")` stays `unique_ptr<std::string>`);
    everything else — member pointer, column
    pointer, nested expression, sub-select — resolves through
    `column_result_t`. That is a superset of both legacy behaviours: the
    argument-typed built-ins now accept any expression, not only columns and
    plain values. The C++17 factories declare the same placeholders, so the
    widening and the single `column_result_t` branch apply to both standards,
    and `field_type_or_type_t`/`nullable_result_proxy` are gone.

    Two consequences: the legacy call-site SFINAE of `nullif`/`iif`/`if_`
    (enabled only when a common type exists) is gone — the error now surfaces
    where the result type is computed, i.e. at `select`; and the node's
    nested `return_type` holds the placeholder rather than the resolved type,
    which needs the schema — code that inspected
    `decltype(coalesce(...))::return_type` must ask `column_result_t`.

14. **Facades beyond the `R` case.** A public function template also stays
    where the name is shared with other function templates (`count(X)` next
    to `count()`/`count<T>()`, `replace(X, Y, Z)` next to the DML `replace`),
    or where the call checks its arguments (`sqlite_offset`'s column check,
    `likelihood`'s constant-evaluated probability range, the odd-argument
    checks of `json_insert`/`json_replace`/`json_set`). `json_extract` and
    `json_quote` keep requiring `R` explicitly.

Arities are now enforced by the signatures rather than accepted blindly;
they follow the SQLite documentation (`coalesce` needs two arguments,
`concat_ws` a separator and one value, `strftime` a format, ...).
`json_group_array`/`json_group_object` are declared as the aggregates they
are and gain `.filter()`/`.over()`.

### Known issue: name ambiguity with the C library in C++20 builds

Some public built-ins are now *objects*, and a few of them share their name
with C library functions that `<math.h>`/`<time.h>`/`<stdlib.h>` put into the
global namespace — `abs`, `round`, `time`, `random`, `strftime`, `printf`.
Under `using namespace sqlite_orm;` an unqualified use of such a name finds
both the object and the C function, which is an ambiguous *lookup*, not an
overload resolution — so it fails even for arguments only one of them could
take. With the legacy function templates the same names merely competed in
overload resolution and usually resolved; `sqlite_orm::abs` already had to be
qualified in the tests for that reason, and the others are affected the same
way now. Qualifying the call (`sqlite_orm::time("now")`) is the workaround.

**Resolved (2026-09-14): those six are function template facades**, like the
`R`-taking functions, `count` and `replace` — a function and a function are an
overload set, so the C++17 behaviour is back exactly: `round(&User::id)` picks
ours because the C overloads are not viable, `abs(-1)` picks `::abs(int)` as it
always did. What the facade gives up is only that these six cannot be passed
around as `orm_built_in_function` objects; the internal definition object is
still there. Rule, recorded next to the other facade reasons in
`core_functions.h`: a built-in whose name is also a global C library function
is published as a function template.

Modules were considered and rejected: with `import std;` the C functions live
in `std` only, but the moment the user's TU includes `<cmath>`/`<ctime>` (which
in practice declare the global names too) or imports `std.compat` the clash is
back — what is in the global namespace is decided by the user's TU, not by the
library.

## Merged: the widening and the node split (2026-09-14)

The parts of phase 4 that hold in both standards went out on their own
(`feature/generic-traits`, `feature/widen-built-in-factories`) and were merged
back. What that changed for this branch:

- `ast/built_in_function.h` is one header for both standards: the nodes shared
  by both (`filtered_aggregate_function`, `count_asterisk_t`,
  `count_asterisk_without_type`, with `count_string` as their name tag) sit
  unconditionally at the top, the legacy `built_in_function_t` /
  `built_in_aggregate_function_t` under `#ifndef SQLITE_ORM_WITH_CPP20_ALIASES`,
  the definition mechanism under `#else`. `core_functions.h` holds only the
  legacy name tags and the factories/definitions; `storage.h` is its one
  consumer.
- The consumers (`ast_iterator`, `column_result`, `node_tuple`,
  `statement_serializer`, `table_name_collector`) match the nodes through the
  vocabulary — grammar traits `is_filtered_aggregate_function`,
  `is_count_asterisk`, projection `where_expression_t` — rather than by name.
- `column_result_t` for a built-in function derives from `substitute_arguments`
  instead of aliasing `substitute_arguments_t`, so an unresolvable placeholder
  (no common type among the arguments) leaves the result type absent rather
  than ill-formed.
- The legacy node's argument tuple is spelled `args_tuple`, like the call
  node's.

## Follow-ups

- Optional: `as_result<R>(expr)` as the general, callee-independent result
  type override (generalizing `as_optional`).
- When C++17 support is dropped: delete the `#ifndef` branches —
  `built_in_function_t`, `built_in_aggregate_function_t`, the tag structs
  and the legacy factories.
- Name clashes inside `sqlite_orm::internal` as more built-ins are defined
  there (`max`, `min`, `count`, ...). Class members shadow them, but a
  namespace-scope internal helper of the same name would not; a nested
  `internal::builtins` namespace is the escape hatch if it ever bites.
- Optional: argument-type checking against the nominal parameter types.
