# Nullable holder facility design

Date: 2026-09-29
Branch base: `refactor/constexpr-variable-templates-not-inline` (#1554), planned as if the
PRs below it are merged in order - in particular #1548's `vocabulary/algorithms/field_accessors.h`.

## Goal

sqlite_orm maps a field that holds a value or nothing - `std::optional<T>`,
`std::unique_ptr<T>`, `std::shared_ptr<T>` - to a nullable column: NULL when empty, the held
value otherwise. That one idea is spelled out separately in every place that handles field
values:

| Where | Smart pointers | `std::optional` |
|---|---|---|
| `type_printer.h` | specialization on `is_std_ptr` | specialization on `is_specialization_of<T, std::optional>` |
| `statement_binder.h` | specialization on `is_std_ptr` | specialization on `is_specialization_of<V, std::optional>` |
| `row_extractor.h` | specialization on `is_std_ptr` | specialization on `is_specialization_of<V, std::optional>` |
| `field_printer.h` | specialization on `is_std_ptr` | specialization on `is_specialization_of<T, std::optional>` |
| `type_is_nullable.h` | named by hand | named by hand |
| `vocabulary/algorithms/field_accessors.h` | `std::unique_ptr` | `std::optional` |

Within each pair the two specializations are the same code, differing only in spelling:
`element_type` vs. `value_type`, `is_std_ptr<V>::make(v)` vs. `std::make_optional(v)`, and
NULL as `nullptr` vs. `std::nullopt`. The NULL paths are already equivalent: the `std::nullptr_t`
and `std::nullopt_t` binders both call `sqlite3_bind_null`, and both printers print `NULL`.

Replace the pairs by one facility: a customization point describing a *nullable holder*, and
a single specialization per consumer keyed on it.

The payoff beyond deduplication: today a user who wants `boost::optional` fields has to
specialize `type_is_nullable`, `type_printer`, `statement_binder`, `row_extractor` and
`field_printer`. With the facility, one specialization of the holder point does it.

## Decisions

1. **A public, open customization point.** The extension case above only works if users can
   specialize it, so it lives in the exported `sqlite_orm` namespace next to its siblings
   `type_printer`, `statement_binder`, `row_extractor`, `field_printer` and `type_is_nullable`,
   in their pre-existing top-level location - not in `vocabulary/`, which is `internal`.
   Working name: `nullable_holder_traits<H, SFINAE = void>` (to be settled, see
   [Open points](#open-points)).

2. **`is_std_ptr` stays, untouched, and becomes the facility's bridge.** `is_std_ptr` is
   exported public API. It can be neither deprecated nor aliased:
   - an alias template cannot be specialized, so any user specialization of `is_std_ptr` for
     a custom pointer would stop compiling;
   - aliasing it to the holder trait would change its meaning - `std::optional` would become
     an `is_std_ptr`;
   - deprecating the class template would fire in the bridge below.

   So it keeps its current shape (`value`, `element_type`, `make()`), and the holder point's
   pointer specialization is keyed on `is_std_ptr<H>::value` and implemented through its
   members. A user specialization of `is_std_ptr` for a custom pointer thereby keeps working,
   and now reaches all consumers through the facility. The header `is_std_ptr.h` folds into
   the facility's header - same name, same namespace; only the file goes.

3. **Detection follows the `field_printer`/`is_printable` precedent.** An internal, closed
   field predicate `is_nullable_holder_v` tests whether the point is specialized for a type.
   It is declared in `vocabulary/algorithms/field_predicates_fwd.h` and defined next to the
   point it tests, exactly like `is_bindable_v` and `is_printable_v`.

4. **The field accessors build on the point.** `held_object_t` and `emplace_held_object` in
   `vocabulary/algorithms/field_accessors.h` (#1548) keep their contract - a non-holder is
   its own held object - but are implemented through the point instead of naming
   `std::unique_ptr` and `std::optional`. Get results thereby accept any holder, not just the
   two standard ones; the `storage_t` API is unaffected, as it names its result types.

5. **`type_is_nullable` keys on the facility, but is not merged into it.** Nullable is wider
   than holder: `examples/nullable_enum_binding.cpp` makes an enum nullable through a sentinel
   value, with no held object. So `type_is_nullable`'s holder specialization keys on
   `is_nullable_holder` and answers through the point's `has_value`; the primary template and
   user specializations are untouched. Moving `type_is_nullable` into the field traits is
   separate work.

6. **One consumer specialization each.** `type_printer`, `statement_binder`, `row_extractor`
   and `field_printer` each replace their pointer/optional pair by one specialization keyed on
   `is_nullable_holder`, keeping the existing gates (`is_bindable`, `is_printable`, the
   extraction concepts) on the held type. NULL is bound and printed through `std::nullptr_t`.

## Shape

```cpp
SQLITE_ORM_EXPORT namespace sqlite_orm {
    /*
     *  Describes a type that holds an object or nothing, which sqlite_orm maps to a nullable column.
     *  Specialize it for your own holder type (e.g. `boost::optional`).
     */
    template<class H, class SFINAE = void>
    struct nullable_holder_traits;  // undefined: not a holder

    template<class T>
    struct nullable_holder_traits<std::optional<T>, void> {
        using held_type = T;

        static std::optional<T> make(std::remove_cv_t<T>&& v);
        static T& emplace(std::optional<T>& h);
        static bool has_value(const std::optional<T>& h);
        static const T& value(const std::optional<T>& h);
    };

    //  the bridge: every `is_std_ptr`, including user specializations
    template<class H>
    struct nullable_holder_traits<H, std::enable_if_t<is_std_ptr<H>::value>> {
        using held_type = typename is_std_ptr<H>::element_type;

        static H make(std::remove_cv_t<held_type>&& v) {
            return is_std_ptr<H>::make(std::move(v));
        }
        // emplace, has_value, value ...
    };
}
```

An empty holder is a value-initialized `H{}`, for which no member is needed.

Consumers read, e.g. for `row_extractor`:

```cpp
template<class H>
struct row_extractor<H, std::enable_if_t<internal::is_nullable_holder_v<H>>> {
    using traits = nullable_holder_traits<H>;
    using unqualified_type = std::remove_cv_t<typename traits::held_type>;

    H extract(sqlite3_stmt* stmt, int columnIndex) const {
        if (sqlite3_column_type(stmt, columnIndex) != SQLITE_NULL) {
            return traits::make(row_extractor<unqualified_type>{}.extract(stmt, columnIndex));
        } else {
            return H{};
        }
    }
    // ...
};
```

## Files

- `dev/nullable_holder_traits.h` (new, public, top-level): the point, its `std::optional`
  and `is_std_ptr` specializations, the definition of `is_nullable_holder_v`; absorbs
  `is_std_ptr.h`.
- `dev/is_std_ptr.h`: removed, its content moved unchanged.
- `dev/vocabulary/algorithms/field_predicates_fwd.h`: declares `is_nullable_holder_v` /
  `is_nullable_holder`.
- `dev/vocabulary/algorithms/field_accessors.h`: `held_object_t` / `emplace_held_object`
  through the point. As it then needs the point's definitions, it moves from the
  `node_algorithms.h` umbrella to the `node_algorithm_definitions.h` manifest, like
  `field_predicates.h` for the same reason.
- `dev/type_printer.h`, `dev/statement_binder.h`, `dev/row_extractor.h`,
  `dev/field_printer.h`: one holder specialization each, replacing the pairs.
- `dev/type_is_nullable.h`: its holder specialization keyed on the facility.
- `docs/internals/vocabulary-layer.md`: `is_nullable_holder_v` in the field-predicate rows,
  `field_accessors.h` built on the point.
- Tests:
  - static tests for `is_nullable_holder_v` and `held_object_t` over `std::optional`,
    `std::unique_ptr`, `std::shared_ptr`, a non-holder;
  - a minimal user-defined optional-like holder: one `nullable_holder_traits`
    specialization gives it a nullable column type, binding, extraction and printing;
  - a custom pointer that only specializes `is_std_ptr`: it keeps working through the
    bridge;
  - existing round-trip tests for `std::optional` and the smart pointers stay as they are
    and pin the unchanged behavior.

## Open points

- **Name** of the point: `nullable_holder_traits`, `holder_traits` or `nullable_traits`.
- **`std::shared_ptr` as a get result.** Get results today use `std::unique_ptr` and
  `std::optional` only. Through the facility `std::shared_ptr` would qualify as well; harmless,
  but it is a new capability of the get API rather than a refactoring - decide whether to
  admit or exclude it.
- **Extraction of a const held type.** `is_std_ptr::make` takes `std::remove_cv_t<T>&&`; the
  optional specialization should do the same, so `std::optional<const T>` and
  `std::shared_ptr<const T>` behave alike.
