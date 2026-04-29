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
3. **Table-level constraints — both placements.** Composite PK, FK with
   `.references()`, and multi-column `unique` may be specified either as a
   class-scope annotation (`[[=primary_key(&T::a, &T::b)]]` on the struct) or
   as a variadic extra. Both forms are merged.
4. **Optional table name.** Signature: `make_table<T>(std::string name = "",
   Cs... extras)`. When `name` is empty, the table name is derived at runtime
   from `std::meta::identifier_of(^^T)` (i.e. the struct's identifier).
5. **Feature gating.** No new feature-test macros. The new overload is gated
   by both `SQLITE_ORM_REFLECTION_SUPPORTED` and
   `SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED`, which already exist in
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
#ifdef SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
    template<class T, class... Cs>
        requires (!internal::is_column_v<Cs> && ...)
    auto make_table(std::string name = "", Cs... extras);
#endif
#endif
}
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

### Three new generic helpers in `dev/functional/meta_util.h`

Gated by `SQLITE_ORM_REFLECTION_SUPPORTED`. No sqlite_orm-specific symbols, no
new headers. Their purpose is to keep all of the bleeding-edge reflection
syntax (`^^T`, `[: refl :]`) localized — both for separation of concerns and
because clang-format currently chokes on it.

```cpp
template<class T>
consteval auto extract_type_identifier() {
    return std::meta::identifier_of(^^T);  // string_view
}

template<class T>
consteval auto extract_type_annotations() {
    constexpr auto annos = std::meta::annotations_of(^^T);
    return [&annos]<size_t... J>(std::index_sequence<J...>) consteval {
        return std::tuple{[: annos[J] :]...};
    }(std::make_index_sequence<annos.size()>{});
}

template<class T, size_t I>
consteval auto extract_member_annotations() {
    constexpr auto ctx = std::meta::access_context::current();
    constexpr auto members = nonstatic_data_members_of(^^T, ctx);
    constexpr auto annos = std::meta::annotations_of(members[I]);
    return [&annos]<size_t... J>(std::index_sequence<J...>) consteval {
        return std::tuple{[: annos[J] :]...};
    }(std::make_index_sequence<annos.size()>{});
}
```

### `internal::make_table` in `dev/schema/table.h`

Mirrors `internal::make_view` in `dev/schema/view.h`. Reuses existing
`extract_member_names<T>` and `extract_member_pointers<T>`. Uses local
generic-lambda iteration in the same style as the helpers in
`meta_util.h` (rather than `internal::make_view`'s outer
`std::index_sequence<I...>` template parameter).

```cpp
template<class T, class... Cs>
auto make_table(std::string name, Cs... extras) {
    constexpr auto memberNames    = extract_member_names<T>();
    constexpr auto memberPointers = extract_member_pointers<T>();

    auto columns = [&]<size_t... I>(std::index_sequence<I...>) {
        return std::tuple{
            [&]<size_t Idx>() {
                constexpr auto annos = extract_member_annotations<T, Idx>();
                auto&& [...as] = annos;          // P1061 structured binding pack
                return make_column(
                    std::string(std::get<Idx>(memberNames)),
                    std::get<Idx>(memberPointers),
                    as...
                );
            }.template operator()<I>()
            ...
        };
    }(std::make_index_sequence<count_members<T>()>{});

    auto classScopedConstraints = extract_type_annotations<T>();

    return [&]<class... Es>(std::tuple<Es...>&& elements) {
        validate_base_table_definition<Es...>();
        return base_table<T, std::false_type, Es...>{
            std::move(name), std::move(elements)
        };
    }(std::tuple_cat(std::move(columns),
                     std::move(classScopedConstraints),
                     std::tuple<Cs...>{std::move(extras)...}));
}
```

The nested generic lambda gives each member its own structured-binding-pack
scope: the outer pack expands across members, the structured binding pack
expands across one member's annotations. Members with zero annotations
collapse the inner pack to an empty `make_column(name, ptr)` call — identical
to the current `make_view` behavior.

The tuple-spread generic lambda at the bottom turns
`std::tuple<E1, E2, …>` back into a parameter pack `Es…` for the `base_table`
template. `validate_base_table_definition<Es…>()` runs uniformly on the merged
elements regardless of source (reflection, class-scope annotations, or
extras).

### Public overload

```cpp
template<class T, class... Cs>
    requires (!internal::is_column_v<Cs> && ...)
auto make_table(std::string name = "", Cs... extras) {
    if (name.empty()) {
        name = std::string(internal::extract_type_identifier<T>());
    }
    return internal::make_table<T>(std::move(name), std::move(extras)...);
}
```

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
| Hidden-column factory in `dev/schema/column.h` | `constexpr` |

The user's broader codebase direction is to widen what's available at constant
evaluation, so all of the above are constexpr-ified in this commit even when
the new overload doesn't strictly require it.

### Genuine snag — `foreign_key_t`'s back-reference

`on_update_delete_t<F>` holds `const F& fk`, a back-reference to its parent
`foreign_key_t`. The existing copy-ctor (constraints.h:352-354) reseats
`on_update`/`on_delete` to `*this` so runtime copies always have a valid
back-reference. This same logic must hold across the constexpr→runtime
materialization that P2996 performs when an annotation value is read out
into a runtime tuple element. Test coverage is needed but no design-level
rework.

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
- `dev/schema/column.h` — `constexpr` on hidden-column factory.

After this commit lands, the user manually compiles and verifies nothing
broke before commit 2 starts.

### Commit 2 — Reflection-based `make_table` overload

- `dev/functional/meta_util.h` — three new generic helpers
  (`extract_type_identifier`, `extract_type_annotations`,
  `extract_member_annotations`).
- `dev/schema/table.h` — new `internal::make_table<T>` and new public
  `make_table<T>(name = "", extras...)` overload. Gated by both
  `SQLITE_ORM_REFLECTION_SUPPORTED` and
  `SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED`. No edits to the existing
  classical overloads.

### Commit 3 — Tests

New file `tests/reflection_make_table.cpp` (or piggyback onto wherever
`make_view` is tested), gated by both macros. Test cases:

- Bare `make_table<User>()` reflects name + columns; `sync_schema` produces
  the expected SQL.
- Member-scope annotations: PK + autoincrement, `not_null`, each `collate_*`,
  `default_value` with literal-type values, composite combinations on a
  single member.
- Class-scope annotations: composite `primary_key(&T::a, &T::b)`,
  `foreign_key(...).references(...)`, multi-column `unique`.
- Variadic-extras path: the same constraints passed as extras instead of as
  annotations produce the same resulting schema.
- Negative test: `make_table<T>("u", make_column(...))` still picks the
  classical overload — verified via a trait probe on the return type.
- Empty-name reflection: `make_table<User>("")` produces a table named
  `User`.
- The `foreign_key_t` back-reference reseat across the constexpr→runtime
  materialization (the snag from the constexpr-ification section).

## Out of scope (deferred until somebody asks)

- Per-field opt-out via `[[=transient]]`.
- Custom column renaming via annotation.
- Getter/setter columns via reflection.
- Annotation-driven `check()`.
