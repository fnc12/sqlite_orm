# Reflection-based `make_table` design

Date: 2026-04-29
Branch base: `dbo-reflection`

## Goal

Add a new overload of `make_table<T>()` that reflects the mapped type's columns
(via P2996 reflection) and reads column/table constraints from P3394
annotations on the type and its non-static data members. Mirrors the existing
`make_view<T>()` pipeline in `dev/schema/view.h`.

The new overload is purely additive. The two existing classical overloads are
unchanged.

## Decisions

1. **Annotation vocabulary.** Annotations reuse the existing constraint
   factory functions (`primary_key()`, `collate_nocase()`, `default_value(v)`,
   etc.) directly as P3394 annotation values: `[[=primary_key().autoincrement()]]`,
   `[[=collate_nocase()]]`, etc. No parallel "annotation tag" vocabulary.
   Prerequisite: every constraint factory and constraint type involved must be
   `constexpr` / a literal type.
2. **Variadic fallback.** The new overload accepts a variadic `Cs... extras`
   for table-level constraints that either can't be expressed as annotations
   (e.g. `check()`) or that the user simply prefers to pass at the call site.
   The variadic *rejects* column types via a `requires` clause — column-level
   constraints must come from annotations. If a user needs a column-level
   escape hatch (non-literal default, getter/setter column, hidden column),
   they fall back to the classical overload entirely. No mid-fidelity
   hybrid mode.
3. **Table-level constraints — variadic-extras path only.** Composite PK,
   FK with `.references()`, and multi-column `unique` are passed as variadic
   extras to `make_table`. They cannot be expressed as class-scope
   annotations on the struct because a class-head annotation is parsed
   *before* the class-head-name enters scope, so any reference to `T` (and
   therefore to `T::member`) inside the annotation expression fails name
   lookup; forward-declaring the class doesn't unblock it because the
   forward-declared `T` is incomplete and `&T::a` still fails member lookup.
   Class-scope annotation extraction is still implemented and merged into the
   table elements — it works for self-contained annotation payloads
   (other-type member pointers, primitives, string-views) — but no
   sqlite_orm table-level constraint factory currently fits that shape.
4. **Table/view name from annotation.** The reflection-based `make_table<T>`
   and `make_view<T>` overloads do not take a name argument. The name comes
   from the optional class-scope `[[=dbo_name("…")]]` annotation, which wraps
   a compile-time C string literal in a `dbo_name_t`; when the annotation is
   absent the name falls back to `std::meta::identifier_of(^^T)`. The
   classical `make_table(name, columns...)` overload still accepts a runtime
   string for callers that genuinely need dynamic naming.
5. **Feature gating.** No new feature-test macros. The new overload is gated
   by `SQLITE_ORM_REFLECTION_SUPPORTED`, which already exists in
   `dev/functional/cxx_core_features.h`.
6. **`check()` excluded.** Carrying a select-statement payload, `check_t` is
   not made annotation-friendly. It rides through the variadic extras path
   only, and only if it happens to be a literal type at the call site —
   otherwise the user falls back to the classical overload.
7. **Hidden / getter-setter columns excluded.** Reflection sees only
   non-static data members. Tables that need getter/setter columns or hidden
   columns continue to use the classical overload.

## API surface

```cpp
SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
    template<size_t N>
    constexpr internal::dbo_name_t<N> dbo_name(const char (&dboName)[N]);

    template<class T, class... Cs>
        requires (!internal::is_column_v<Cs> && ...)
    auto make_table(Cs... extras);

#ifdef SQLITE_ORM_WITH_VIEW
    template<class O, class Select>
        requires (internal::is_select_expression_v<Select>)
    auto make_view(Select select);
#endif
#endif
}
```

Usage:

```cpp
struct [[=dbo_name("users")]] User {
    [[=primary_key().autoincrement()]] int id;
    [[=not_null()]] std::string name;
};

auto storage = make_storage("db.sqlite", make_table<User>());  // name "users", PK on id, NOT NULL on name
```

### Overload resolution against the existing classical overloads

| Call | Picked | Reason |
|---|---|---|
| `make_table<User>()` | new | classical second requires a `name` arg |
| `make_table<User>("u")` | new | both viable; new is more-constrained (`requires`) |
| `make_table<User>("u", primary_key(&User::a, &User::b))` | new | both viable; new more-constrained |
| `make_table<User>("u", make_column("id", &User::id))` | classical second | new's `requires` rejects column types |
| `make_table("u", make_column("id", &User::id))` | classical first | first deduces `T` from a column; new and second need explicit `T` |

Concept partial-ordering does the dispatch — no SFINAE, no metaprogramming
gymnastics.

## Reflection mechanics

### Generic helpers in `dev/functional/meta_util.h`

Gated by `SQLITE_ORM_REFLECTION_SUPPORTED`. No sqlite_orm-specific symbols, no
new headers. Their purpose is to keep all of the bleeding-edge reflection
syntax (`^^T`, `[: refl :]`) localized — both for separation of concerns and
because clang-format currently chokes on it. A single member-source helper
returns the array of `std::meta::info` reflections; per-member name / pointer /
annotation queries are then composed inline against that array, consolidating
what was originally three split helpers into one source of truth.

```cpp
template<class T>
consteval auto extract_members() {
    constexpr auto ctx = std::meta::access_context::current();
    constexpr size_t N = nonstatic_data_members_of(^^T, ctx).size();
    return [&]<size_t... I>(std::index_sequence<I...>) consteval {
        return std::array<std::meta::info, N>{
            nonstatic_data_members_of(^^T, ctx)[I]...
        };
    }(std::make_index_sequence<N>{});
}

template<class T>
consteval auto extract_type_identifier() {
    return std::meta::identifier_of(^^T);  // string_view
}

template<std::meta::info Member>
consteval auto splice_member_pointer() {
    return &[: Member :];
}

template<std::meta::info Refl>
consteval auto splice_annotations() {
    return []<size_t... I>(std::index_sequence<I...>) consteval {
        return std::tuple{
            [: std::meta::constant_of(std::meta::annotations_of(Refl)[I]) :]...
        };
    }(std::make_index_sequence<std::meta::annotations_of(Refl).size()>{});
}

template<class T>
consteval auto extract_type_annotations() {
    return splice_annotations<^^T>();
}
```

Two implementation details inside `splice_annotations` are load-bearing and
non-obvious:

- **`std::meta::constant_of` wrap.** Per P3394 §[meta.reflection.annotation],
  reflections returned by `annotations_of` are not directly spliceable;
  `constant_of(annotation_info)` returns a splice-able constant reflection of
  the underlying value.
- **No `constexpr auto annos = annotations_of(refl)` binding.**
  `annotations_of` returns a `std::vector<std::meta::info>`, and the heap
  allocation is *transient* under C++20 constexpr rules — it cannot be bound
  to a `constexpr` variable that outlives the immediate call. The size and
  per-index lookups therefore re-call `annotations_of` inline so each
  transient vector dies within its own constant expression.

`splice_member_pointer` and `splice_annotations` take `std::meta::info` as a
non-type template parameter rather than a runtime parameter because the
splices `&[: Member :]` and `[: constant_of(...) :]` produce return types that
depend on the parameter *value* — fixed-signature `auto` deduction can't
handle that, so each splice instantiates its own specialization.

### `internal::make_reflected_table` in `dev/schema/table.h`

Mirrors `internal::make_view` in `dev/schema/view.h`. Both consume the same
`extract_members` source and use the same nested generic-lambda iteration
pattern.

```cpp
template<class T, class... Cs>
auto make_reflected_table(std::string name, Cs... constraints) {
    if (name.empty()) {
        name = std::string(internal::extract_type_identifier<T>());
    }
    static constexpr auto members = extract_members<T>();

    auto columns = []<size_t... I>(std::index_sequence<I...>) static {
        return std::tuple{
            []<std::meta::info Member>() static {
                return std::apply(
                    [](auto&&... annotations) static {
                        return make_column(
                            std::string(std::meta::identifier_of(Member)),
                            splice_member_pointer<Member>(),
                            std::move(annotations)...
                        );
                    },
                    splice_annotations<Member>()
                );
            }.template operator()<members[I]>()...
        };
    }(std::make_index_sequence<members.size()>{});

    auto annotationConstraints = extract_type_annotations<T>();

    return [&name]<class... Es>(std::tuple<Es...>&& definition) {
        validate_base_table_definition<Es...>();
        return base_table<T, std::false_type, Es...>{
            std::move(name), std::move(definition)
        };
    }(std::tuple_cat(std::move(columns),
                     std::move(annotationConstraints),
                     std::tuple<Cs...>{std::move(constraints)...}));
}
```

Three implementation details inside the body are load-bearing:

- **`static constexpr auto members`.** Function-local `constexpr`
  alone is insufficient: GCC's reflection branch over-eagerly captures
  enclosing-scope `constexpr` locals by reference inside nested lambdas (even
  with no-capture `[]`), which then loses constexpr-ness inside the lambda
  body and immediate-escalates the lambda to consteval — at which point the
  runtime call to `make_column` is rejected. Promoting `members` to static
  storage sidesteps the question entirely; static-storage variables don't
  need capture in any scenario.
- **`Member` as a `std::meta::info` NTTP** on the inner lambda. The outer
  pack expansion supplies `members[I]` directly, so the inner lambda body
  never has to declare a `constexpr auto member = ...` local — same
  motivation as above, applied to the inner scope.
- **`std::apply` instead of a structured-binding pack.** Replaces the P1061
  `auto&& [...as] = annos;` from the original sketch and removes the
  dependency on `SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED`.

The tuple-spread generic lambda at the bottom turns
`std::tuple<E1, E2, …>` back into a parameter pack `Es…` for the `base_table`
template. `validate_base_table_definition<Es…>()` runs uniformly on the merged
elements regardless of source (reflection, class-scope annotations, or
extras).

### Public overload

```cpp
template<class T, class... Cs>
    requires (!internal::is_column_v<Cs> && ...)
auto make_table(Cs... tableConstraints) {
    return internal::make_reflected_table<T>(std::forward<Cs>(tableConstraints)...);
}
```

### `dbo_name` annotation

`dev/schema/dbo_name.h` carries the lightweight infrastructure for the table /
view name annotation:

```cpp
namespace sqlite_orm::internal {
    template<size_t N>
    struct dbo_name_t : cstring_literal<N> {
        constexpr dbo_name_t(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}
    };

    template<class T, class Tuple>
    constexpr std::string_view resolve_dbo_name(const Tuple& annotations) {
        if constexpr (tuple_contains_dbo_name_v<Tuple>) {
            return std::get<dbo_name_t>(annotations).cstr;
        } else {
            return extract_type_identifier<T>();
        }
    }

    // Returns a copy of `tuple` with all `dbo_name_t` elements removed,
    // so they don't get merged into the table's element pool.
    template<class Tuple>
    constexpr auto filter_out_dbo_name(Tuple&& tuple);
}

namespace sqlite_orm {
    template<size_t N>
    constexpr internal::dbo_name_t<N> dbo_name(const char (&dboName)[N]) {
        return {dboName};
    }
}
```

`make_reflected_table` and `make_reflected_view` both call
`resolve_dbo_name<O>(extract_type_annotations<O>())` to settle the name; the
table-builder additionally calls `filter_out_dbo_name` before merging the
remaining class-scope annotations into the elements pool, so the metadata
annotation doesn't trip `validate_base_table_definition`.

The string is embedded in the type's bytes via `cstring_literal<N>` rather than
carried by pointer + size: pointers to string literals are not accepted as
annotation values by current reflection implementations (the underlying object
has no linkage), so a self-contained fixed-size byte array is required.

## Constexpr-ification prerequisite

A separate, mechanical precursor commit. Required because annotation values
must be constant expressions of literal types.

| Type | Work |
|---|---|
| `primary_key_t<Cs...>` / `_with_autoincrement` | likely none — ctor already `constexpr` |
| `unique_t<Cs...>`                              | factory `constexpr` |
| `null_t`, `not_null_t`                         | factories `constexpr` |
| `collate_constraint_t`                         | factory + ctor `constexpr` |
| `default_t<T>`                                 | factory `constexpr`; *T must itself be a literal type* |
| `foreign_key_intermediate_t<Cs...>`            | factory + `.references()` member `constexpr` |
| `foreign_key_t<...>`                           | both ctors `constexpr` |
| `on_update_delete_t<F>`                        | ctor + the four mutator methods (`cascade()`, `set_null()`, `restrict_()`, `set_default()`, `no_action()`) `constexpr` |
| `check_t`                                      | constexpr-ified anyway, for consistency with the broader push |
| `generated_always_t`                           | constexpr-ified anyway, for consistency |

The user's broader codebase direction is to widen what's available at constant
evaluation, so all of the above are constexpr-ified in this commit even when
the new overload doesn't strictly require it.

### Note on `foreign_key_t`'s back-reference

`on_update_delete_t<F>` holds `const F& fk`, a back-reference to its parent
`foreign_key_t`. The existing copy-ctor (constraints.h:352-354) reseats
`on_update`/`on_delete` to `*this` so runtime copies always have a valid
back-reference. The intended verification was to check that this same logic
holds across the constexpr→runtime materialization that P2996 performs when
an annotation value is read out into a runtime tuple element — but that
materialization path only happens for FK *annotations*, and FK as a class- or
member-scope annotation isn't expressible (forward-reference issue, decision
3). FK as a variadic extra never goes through annotation materialization, so
the back-reference reseat for the annotation path remains untested in
practice. No design-level rework is required; this just records why the
originally-planned test isn't present.

### `default_t<T>` consequence — explicit

`[[=default_value(std::string("foo"))]]` will not compile (string isn't a
literal type for cross-translation-unit annotation persistence even in C++26).
For string defaults, either:
- use `default_value("foo"sv)` for a `std::string_view`-typed column, or
- fall back to the classical overload entirely.

This is the intended trade-off behind Option Y in the decisions section.

## File-by-file plan

### Commit 1 — Constexpr-ification (mechanical, no behavior change)

- `dev/constraints.h` — `constexpr` on every factory function and constraint
  type ctor / fluent-API mutator listed in the table above.

`make_hidden_column` in `dev/schema/column.h` was considered but is *not*
constexpr-ified: its `std::string` parameter has no constexpr call site
(hidden columns are constructed at runtime as part of normal table builds),
so marking it `constexpr` would not unlock any actual compile-time use case.

After this commit lands, the user manually compiles and verifies nothing
broke before commit 2 starts.

### Commit 2 — Reflection-based `make_table` overload

- `dev/functional/meta_util.h` — generic helpers (`extract_members`,
  `extract_type_identifier`, `splice_member_pointer`, `splice_annotations`,
  `extract_type_annotations`). The same `extract_members` source is also
  consumed by the refactored `internal::make_view` so both schema-builders
  share one reflection pipeline.
- `dev/schema/table.h` — new `internal::make_reflected_table<T>` and new
  public `make_table<T>(name = "", extras...)` overload. Gated by
  `SQLITE_ORM_REFLECTION_SUPPORTED`. No edits to the existing classical
  overloads.
- `dev/schema/view.h` — `internal::make_view` refactored to consume
  `extract_members` and `splice_member_pointer` instead of the old
  `extract_member_names` / `extract_member_pointers` / `count_members`
  helpers.

### Commit 3 — Tests

New file `tests/schema/reflection_table_tests.cpp`, gated by
`SQLITE_ORM_REFLECTION_SUPPORTED`. Test cases that landed:

- Column reflection: `make_table<T>("name")` reflects the type's non-static
  data members as columns; column-name lookup via member pointer round-trips.
- Empty-name reflection: `make_table<T>()` produces a table named after the
  type's identifier; non-empty `name` overrides.
- Member-scope annotations: `[[=primary_key().autoincrement()]]`,
  `[[=not_null()]]`, `[[=default_value(literal)]]`, `[[=collate_nocase()]]`
  apply to the annotated column; verified after `sync_schema` via
  `pragma.table_xinfo`.
- Variadic-extras path: composite `primary_key(&T::a, &T::b)` populates
  `table_key_columns_names()`; `foreign_key(...).references(...)` survives
  schema sync.
- Overload dispatch: `make_table<T>("u", make_column(...))` falls through to
  the classical overload (verified via `tuple_size_v` of `elements_type`).

Tests originally planned but not landed:

- Class-scope annotation cases (composite PK, FK, multi-column unique on the
  struct itself) — not expressible (decision 3).
- `foreign_key_t` back-reference reseat across constexpr→runtime
  materialization — only exercised on the annotation path, which FK can't
  ride (see "Note on `foreign_key_t`'s back-reference" above).

## Out of scope (deferred until somebody asks)

- Per-field opt-out via `[[=transient]]`.
- Custom column renaming via annotation.
- Getter/setter columns via reflection.
- Annotation-driven `check()`.
