# Reflection-based `make_table` design

Date: 2026-04-29
Last updated: 2026-08-06
Branch base: `dbo-reflection`

## Revision log

- **2026-08-06** — Updated to match what landed:
  - `dbo_name` renamed to `orm_name`; added the `_orm_name` string-literal
    operator template as the preferred spelling (decision 4, "`orm_name`
    annotation" section).
  - `default_value` with a traditional C string literal works, so the
    string-default trade-off is milder than originally recorded
    ("`default_t<T>` consequence").
  - `foreign_key_t`'s `on_update` / `on_delete` back-reference was reworked in
    commit `9485732b` — the stored reference is gone, replaced by an address
    computation from `this`. Consequences for the constexpr-ification table
    and for the FK note are folded in.
  - `examples/view.cpp` now doubles as the worked example for the new
    `make_table` overload.

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
   from the optional class-scope name annotation, which wraps a compile-time
   C string literal in an `internal::dbo_name_literal<N>`; when the annotation
   is absent the name falls back to `std::meta::identifier_of(^^T)`. The
   classical `make_table(name, columns...)` overload still accepts a runtime
   string for callers that genuinely need dynamic naming.

   The annotation has two spellings, both producing the same
   `dbo_name_literal<N>` value:

   - `[[= "users"_orm_name]]` — the string-literal operator template, the
     preferred spelling.
   - `[[= orm_name("users")]]` — the factory function, kept as a fallback for
     contexts where the literal operator is awkward.

   The public name is `orm_name`, not `dbo_name`: `dbo_name` survives only as
   the internal type/helper prefix (`dbo_name_literal`, `resolve_dbo_name`,
   `filter_out_dbo_name`), where "dbo" usefully denotes *database object* —
   table or view — but the public spelling matches the library's `orm`
   vocabulary.
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
    inline namespace literals {
        template<internal::dbo_name_literal dboName>
        [[nodiscard]] consteval auto operator""_orm_name();
    }

    template<size_t N>
    consteval internal::dbo_name_literal<N> orm_name(const char (&dboName)[N]);

    template<class T, class... Cs>
        requires (!internal::is_column_v<Cs> && ...)
    auto make_table(Cs... tableConstraints);

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_table_reference auto table, class... Cs>
        requires (!internal::is_column_v<Cs> && ...)
    auto make_table(Cs... tableConstraints);
#endif

#ifdef SQLITE_ORM_WITH_VIEW
    template<class O, class Select>
        requires (internal::is_select_expression_v<Select>)
    auto make_view(Select select);
#endif
#endif
}
```

The `orm_table_reference auto` overload mirrors the classical
`make_table<table>(name, …)` alias overload; it forwards to
`make_reflected_table<auto_decay_table_ref_t<table>>`.

Usage:

```cpp
struct [[= "users"_orm_name]] User {
    [[= primary_key().autoincrement()]] int id;
    [[= not_null()]] std::string name;
};

auto storage = make_storage("db.sqlite", make_table<User>());  // name "users", PK on id, NOT NULL on name
```

### Overload resolution against the existing classical overloads

Because the reflection overload takes no name parameter, the two families are
separated by arity and by the `requires` clause rather than by partial
ordering of the name argument.

| Call | Picked | Reason |
|---|---|---|
| `make_table<User>()` | new | classical second requires a `name` arg |
| `make_table<User>(primary_key(&User::a, &User::b))` | new | classical second's first parameter is `std::string`; a `primary_key_t` doesn't convert |
| `make_table<User>("u", make_column("id", &User::id))` | classical second | new's `requires` rejects column types |
| `make_table("u", make_column("id", &User::id))` | classical first | first deduces `T` from a column; new and second need explicit `T` |

Ordinary overload resolution plus one `requires` clause does the dispatch — no
SFINAE, no metaprogramming gymnastics.

**Sharp edge — `make_table<User>("u")`.** Passing only a name to the explicit-`T`
form selects the *reflection* overload, not the classical one: `Cs` deduces to
`const char*` (an exact match after array-to-pointer decay), which beats the
classical overload's user-defined conversion to `std::string`. The string
literal is then treated as a table-level constraint and
`validate_base_table_definition` fires its "Incorrect base table elements or
constraints" `static_assert`. This is a compile-time diagnostic, not silent
misbehaviour, but the message doesn't point at the real mistake. Callers who
want a runtime name must pass at least one column, i.e. use the classical
overload proper. (Derived from the overload set as written; not covered by a
test — worth adding a `static_assert`-friendly diagnostic or a deleted overload
if it bites in practice.)

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
template<class O, class... Cs>
auto make_reflected_table(Cs... constraints) {
    auto classAnnotations = extract_type_annotations<O>();
    std::string tableName{resolve_dbo_name<O>(classAnnotations)};
    auto annotationConstraints = filter_out_dbo_name(std::move(classAnnotations));
    static /*gcc*/ constexpr auto members = extract_members<O>();

    auto columns = []<size_t... I>(std::index_sequence<I...>) static {
        return std::tuple{
            []<std::meta::info member>() static {
                return std::apply(
                    [](auto&&... columnConstraints) static {
                        return sqlite_orm::make_column(
                            std::string(std::meta::identifier_of(member)),
                            splice_member_pointer<member>(),
                            std::move(columnConstraints)...
                        );
                    },
                    splice_annotations<member>()
                );
            }.template operator()<members[I]>()...
        };
    }(std::make_index_sequence<members.size()>{});

    return [&tableName]<class... Es>(std::tuple<Es...>&& definition) {
        validate_base_table_definition<Es...>();
        return base_table<O, std::false_type, Es...>{
            std::move(tableName), std::move(definition)
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

### `orm_name` annotation

`dev/schema/dbo_name.h` carries the lightweight infrastructure for the table /
view name annotation:

```cpp
namespace sqlite_orm::internal {
    template<size_t N>
    struct dbo_name_literal : cstring_literal<N> {
        constexpr dbo_name_literal(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}

        constexpr orm_gsl::czstring name() const noexcept { return this->cstr; }
        constexpr operator std::string_view() const noexcept { return this->cstr; }
    };

    template<class T>
    constexpr bool is_dbo_name_literal_v = false;
    template<size_t N>
    constexpr bool is_dbo_name_literal_v<dbo_name_literal<N>> = true;

    template<class T, class Tuple>
    constexpr std::string_view resolve_dbo_name(const Tuple& annotations) {
        using name_index = find_tuple_element<Tuple, is_dbo_name_literal>;

        if constexpr (name_index::value < std::tuple_size_v<Tuple>) {
            return std::get<name_index::value>(annotations).name();
        } else {
            return extract_type_identifier<T>();
        }
    }

    // Returns a copy of `tuple` with all `dbo_name_literal<…>` elements removed,
    // so they don't get merged into the table's element pool.
    template<class Tuple>
    constexpr auto filter_out_dbo_name(Tuple&& tuple);
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline namespace literals {
        template<internal::dbo_name_literal dboName>
        [[nodiscard]] consteval auto operator""_orm_name() {
            return dboName;
        }
    }

    template<size_t N>
    consteval internal::dbo_name_literal<N> orm_name(const char (&dboName)[N]) {
        return {dboName};
    }
}
```

`make_reflected_table` and `make_reflected_view` both call
`resolve_dbo_name<O>(extract_type_annotations<O>())` to settle the name; the
table-builder additionally calls `filter_out_dbo_name` before merging the
remaining class-scope annotations into the elements pool, so the metadata
annotation doesn't trip `validate_base_table_definition`.

Lookup is by trait (`find_tuple_element<Tuple, is_dbo_name_literal>`) rather
than by exact type, because `dbo_name_literal<N>` is parameterised on the
string length — `std::get<dbo_name_literal>` would need the `N` the caller
never spells out.

The string is embedded in the type's bytes via `cstring_literal<N>` rather than
carried by pointer + size. This is a hard requirement for the `_orm_name`
spelling: `operator""_orm_name` takes the literal as a *non-type template
parameter*, and template arguments of pointer type must designate an entity
with linkage — a string literal has none. The byte-array form makes the type
structural and the whole thing works as an NTTP.

Both spellings collapse to the same annotation value, so `resolve_dbo_name`
needs no knowledge of which one was used:

```cpp
struct [[= "users"_orm_name]] User { … };       // preferred
struct [[= orm_name("users")]] User { … };      // equivalent fallback
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
| `foreign_key_intermediate_t<Cs...>`            | `foreign_key(…)` factory `constexpr`; `.references()` **not** — see below |
| `foreign_key_t<...>`                           | aggregate; no ctor to constexpr-ify after the rework |
| `on_fk_update_delete<F, forUpdate>`            | mutators **not** `constexpr` — see below |
| `check_t`                                      | constexpr-ified anyway, for consistency with the broader push |
| `generated_always_t`                           | constexpr-ified anyway, for consistency |

The user's broader codebase direction is to widen what's available at constant
evaluation, so all of the above are constexpr-ified in this commit even when
the new overload doesn't strictly require it.

### Note on `foreign_key_t`'s back-reference

**Superseded by commit `9485732b`** (merge of `fk-improvements`). The mechanism
changed; the conclusion for this design did not.

*Originally:* `on_update_delete_t<F>` stored `const F& fk`, a back-reference to
its enclosing `foreign_key_t`, with copy construction/assignment deleted and a
hand-written `foreign_key_t` copy-ctor reseating `on_update` / `on_delete` to
`*this`. The plan was to verify that the reseat also holds across the
constexpr→runtime materialization P2996 performs when an annotation value is
read out into a runtime tuple element.

*Now* (`dev/schema/constraints/foreign_key.h`): the stored reference is gone.
`on_fk_update_delete<F, forUpdate>` derives from `on_fk_action` (which carries
just the `foreign_key_action` enum) and recovers its enclosing FK from its own
address:

```cpp
foreign_key_type copy_fk(foreign_key_action newAction) const {
    const foreign_key_type* thisFk = addressof_enclosing(this, forUpdate ? &F::on_update : &F::on_delete);
    foreign_key_type fk2 = *thisFk;   // copy, then mutate the copy's action
    …
}
```

`addressof_enclosing` (`dev/functional/addressof.h`) subtracts a computed
member offset from `this` via `reinterpret_cast`, and `offsetof_member`
`static_assert`s `std::is_standard_layout_v<O>`. The `forUpdate` bool is a
template parameter rather than a data member, so the pick between
`&F::on_update` and `&F::on_delete` is `if constexpr`.

What this buys, and what it costs:

- **Buys:** `foreign_key_t` is a plain aggregate again — smaller (no reference
  member), freely copyable and assignable, with no reseating copy-ctor to keep
  in sync. The back-reference can never dangle, because it is never stored.
- **Costs:** the address arithmetic uses `reinterpret_cast`, which is not
  permitted during constant evaluation. So `cascade()`, `set_null()`,
  `restrict_()`, `set_default()` and `no_action()` cannot be `constexpr`, and
  neither can `foreign_key_intermediate_t::references()` (which builds the
  `foreign_key_t` those mutators are called on). The `foreign_key(…)` factory
  itself is still `constexpr`, but that only gets you as far as the
  intermediate.

**Consequence for this design: none, but for a second reason now.** A fully
formed FK still cannot be an annotation value — previously because it isn't
*expressible* as a class-scope annotation (the forward-reference issue,
decision 3), and now additionally because it isn't *constructible* at compile
time. Both roads lead to the same place: FK reaches a reflected table only
through the variadic-extras path, which is ordinary runtime construction and
never goes through annotation materialization. The originally-planned
materialization test therefore still isn't present, and the runtime path is
covered by the "foreign_key().references() supplied as an extra survives schema
sync" test case and by `examples/view.cpp`.

If FK-as-annotation is ever wanted, the constexpr blocker is the one to attack
first: it would need a constant-evaluation-friendly way to reach the enclosing
FK (e.g. reinstating an explicit back-reference only on a constexpr path, or
having the mutators return a fresh `foreign_key_t` built from the members they
can see rather than from a copy of the enclosing object).

### `default_t<T>` consequence — explicit

`[[= default_value(std::string("foo"))]]` will not compile: `std::string`
allocates, so it isn't a literal type usable as an annotation value even in
C++26.
For string defaults, either:

- `[[= default_value("foo")]]` — a traditional C string literal works.
  `T` deduces to `const char*`, `default_t<const char*>` is a literal type, and
  the pointer is a valid annotation value (a string literal has static storage
  duration; the linkage restriction that bites NTTPs does not apply to
  annotation values). This is the simplest spelling and the one to reach for.
- `[[= default_value("foo"sv)]]` — a `std::string_view`, for when the column's
  own type is `std::string_view` or when the extra type information is wanted.
- Fall back to the classical overload entirely — only genuinely needed when the
  default value is not a constant expression at all (computed at runtime,
  dependent on configuration, etc.).

## Worked example — `examples/view.cpp`

The existing SQL-view example was extended to exercise the new `make_table`
overload alongside `make_view`, so a single example shows the whole
reflection-based schema pipeline rather than just the view half. Base tables
that were previously spelled out column by column:

```cpp
struct Employee {
    int64 id;
    std::string name;
    int64 department_id;
    double salary;
};

make_table("employees",
           make_column("id", &Employee::id, primary_key()),
           make_column("name", &Employee::name),
           make_column("department_id", &Employee::department_id),
           make_column("salary", &Employee::salary)),
```

now read:

```cpp
struct [[= "employee"_orm_name]] Employee {
    [[= primary_key()]] int64 id;
    [[= collate_nocase()]] std::string name;
    [[= not_null()]] int64 department_id;
    [[= default_value(0.)]] double salary;
};

make_table<Employee>(foreign_key(&Employee::department_id).references(&Department::id)),
```

The example covers, in one place, most of what this design adds:

- the `_orm_name` class-scope annotation on both tables and view objects
  (previously only view objects carried it);
- member-scope column constraints — `primary_key()`, `not_null()`,
  `collate_nocase()`, `default_value(0.)`;
- the variadic-extras path, via the FK on `Employee::department_id` — which is
  also the one constraint here that *cannot* be an annotation (decision 3 plus
  the constexpr blocker above), making the split between the two paths visible
  at a glance;
- declaration order mattering for FKs: `make_table<Department>()` must precede
  `make_table<Employee>(…)` in the `make_storage` argument list.

Note that the example is gated on `SQLITE_ORM_WITH_VIEW`, not on
`SQLITE_ORM_REFLECTION_SUPPORTED`; the view half already required reflection,
so the table half adds no new toolchain requirement.

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
- `dev/schema/dbo_name.h` — `dbo_name_literal<N>`, `resolve_dbo_name`,
  `filter_out_dbo_name`, and the public `orm_name(…)` / `operator""_orm_name`
  factories.
- `dev/schema/table.h` — new `internal::make_reflected_table<O>` and new
  public `make_table<T>(extras...)` overload, plus the
  `orm_table_reference auto` variant. Gated by
  `SQLITE_ORM_REFLECTION_SUPPORTED`. No edits to the existing classical
  overloads.
- `dev/schema/view.h` — `internal::make_view` refactored to consume
  `extract_members` and `splice_member_pointer` instead of the old
  `extract_member_names` / `extract_member_pointers` / `count_members`
  helpers.

### Commit 3 — Tests

New file `tests/schema/reflection_table_tests.cpp`, gated by
`SQLITE_ORM_REFLECTION_SUPPORTED`. Test cases that landed:

- Column reflection: `make_table<T>()` reflects the type's non-static data
  members as columns; column-name lookup via member pointer round-trips.
- Name resolution: the `_orm_name` annotation supplies the table name; a type
  without the annotation falls back to its reflected identifier.
- Member-scope annotations: `[[= primary_key().autoincrement()]]`,
  `[[= not_null()]]`, `[[= default_value(literal)]]`, `[[= collate_nocase()]]`
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

### Commit 4 — Example

- `examples/view.cpp` — base tables converted to the reflection-based
  `make_table` overload; see "Worked example" above.

## Follow-ups

- Consider a better diagnostic for `make_table<T>("name")` — see "Sharp edge"
  under overload resolution.

## Out of scope (deferred until somebody asks)

- Per-field opt-out via `[[=transient]]`.
- Custom column renaming via annotation.
- Getter/setter columns via reflection.
- Annotation-driven `check()`.
