---
name: add-virtual-table
description: Use when adding a virtual table SQLite or an extension provides to sqlite_orm - an eponymous virtual table or table-valued function (like dbstat, generate_series, pragma_table_info) or a virtual table module applications instantiate (like fts5, rtree). Covers the module tag, the row type with its hidden columns, the definition factory, the table reference, the bundles and tests.
---

# Adding a virtual table

The headers live in `dev/builtin/dbos/`, one per module, included by the umbrella
`dev/builtin/dbos.h`; the generic layer is `dev/schema/virtual_table.h`. Compare with
`dev/builtin/dbos/generate_series.h` (eponymous, from an extension),
`dev/builtin/dbos/pragma_table_info.h` (eponymous, reflection only, bundled) or
`dev/builtin/dbos/rtree.h` (a module with application-defined columns).

Guard each header's contents, never its inclusion, with what the module needs: the SQLite version
(`#if SQLITE_VERSION_NUMBER >= 3008012`), a compile option (`#ifdef SQLITE_ENABLE_DBSTAT_VTAB`) or
reflection (`SQLITE_ORM_REFLECTION_SUPPORTED`).

## 1. The module tag

In `namespace sqlite_orm::internal`:

```cpp
struct generate_series_module_tag : eponymous_module {
    // simplify conceptual/meta programming
    using module_type = generate_series_module_tag;

    static constexpr orm_gsl::czstring name() {
        return "generate_series";
    }
};
```

Derive from `eponymous_module` only for an eponymous virtual table: its definition is then
read-only and implicit, a table without a name is named after the module, and `sync_schema()`
creates only instances named differently from the module.

## 2. The row type (eponymous tables)

In `SQLITE_ORM_EXPORT namespace sqlite_orm`, a struct with one data member per column, named as
SQLite names them. Hidden columns (the table-valued function's arguments, the schema) go into a
nested `struct hidden` with `using enclosing_type = <row type>;`, so that `&row::hidden::arg`
refers to a column of the enclosing table. Without `SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED`,
add a constructor taking every column, as `table_info` does.

Publish a table reference for use as a table-valued function:
`inline constexpr orm_table_reference auto generate_series_table = c<generate_series>();`
(guarded by `SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED`, with a plain `auto` otherwise, unless the
header requires C++26 anyway).

## 3. The definition factory

`using_<module>(...)` returns the virtual table definition:

- with reflection, `make_reflected_virtual_table_definition<module_tag, row_type>(args...)`;
- otherwise `make_virtual_table_definition<module_tag>(make_column(...), make_hidden_column(...), ...)`,
  listing every column.

Module arguments (the table values of `CREATE VIRTUAL TABLE ... USING module(...)`) are passed as
`literal_holder`s and checked with `static_assert`s against the hidden column types, as
`using_dbstat()` does. A module with application-defined columns validates them instead, as
`validate_rtree_definition()` does.

## 4. Making the table available

- An eponymous table gets a public factory adding it to a storage, e.g. `make_generate_series_table()`
  returning `make_virtual_table<row_type>(module_tag::name(), using_<module>())`.
- Tables SQLite provides itself, unlike extension tables, also go into `make_builtin_tables()` in
  `dev/builtin/dbos/builtin_tables.h`, under the same guard; related tables can have a bundle of
  their own like `make_pragma_tables()`.
- Include the new header from `dev/builtin/dbos.h` and update its `@file` comment.
- List the table in the `builtin/` entry of `docs/internals/source-layout.md`.

## 5. Tests and example

- `CREATE VIRTUAL TABLE` and queries: `tests/statement_serializer_tests/schema/using_<module>.cpp`.
  Give a derived instance a name different from the eponymous table, so that column qualification
  is tested.
- Static checks of the definition: `tests/static_tests/virtual_tables.cpp`.
- Execution, including hidden columns and joins: `tests/schema/virtual_table.cpp`; membership in the
  bundles: `tests/builtin_tables.cpp`.
- An example in `examples/` for a table worth showing, with the generated SQL as comments.

## 6. Finish

Run `scripts/pre-commit.sh`, and have the change reviewed against
`docs/internals/review-checklist.md`.
