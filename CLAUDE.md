# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About sqlite_orm

sqlite_orm is a header-only C++17 ORM library for SQLite. It provides type-safe database operations without raw SQL strings, using C++ template metaprogramming to map C++ structs to database tables.

**Key characteristics:**
- Header-only library (main header: `include/sqlite_orm/sqlite_orm.h`)
- Requires C++17 minimum (supports C++20, C++23, C++26)
- Single dependency: libsqlite3
- License: AGPL for open source, MIT after purchase

## Build System

### Building the project
```bash
# Configure with default C++17
cmake -B build

# Configure with specific C++ standard
cmake -B build -DSQLITE_ORM_ENABLE_CXX_20=ON
cmake -B build -DSQLITE_ORM_ENABLE_CXX_23=ON

# Build
cmake --build build

# Install (may need admin rights)
cmake --build build --target install
```

### Running tests
```bash
# Build tests (from project root)
cmake -B build -DBUILD_TESTING=ON
cmake --build build

# Run all tests
cd build && ctest

# Run unit tests directly (with more control)
./build/tests/unit_tests

# Run specific test by name using Catch2 filter
./build/tests/unit_tests "Limits"
./build/tests/unit_tests "[ast_iterator]"
```

The test suite uses Catch2 framework. All test files are in `tests/` directory. Tests are organized using `TEST_CASE` and `SECTION` macros.

### Building examples
```bash
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

## Architecture Overview

### Core Design Pattern
The library uses a **storage-centric architecture** with compile-time type safety through template metaprogramming:

1. **Storage object** (`dev/storage.h`, `dev/storage_base.h`): The main interface that provides CRUD operations, query building, and schema management. Created via `make_storage()`.

2. **Table definitions** (`dev/schema/table.h`): Tables are defined using `make_table()` which maps C++ structs to database schema. Columns are mapped using member pointers.

3. **Type system** (`dev/type_traits.h`, `dev/type_printer.h`): Extensive compile-time type introspection to deduce types from member pointers and validate queries at compile time.

4. **Statement serialization** (`dev/statement_serializer.h`, `dev/serializer_context.h`): Converts C++ expression objects into SQL strings.

5. **Expression objects** (`dev/conditions.h`, `dev/core_functions.h`, `dev/ast/`): Type-safe representations of SQL operations (WHERE, JOIN, ORDER BY, etc.).

### Key Implementation Files

**Storage layer:**
- `dev/storage.h` - Main storage template with CRUD operations
- `dev/storage_base.h` - Base class with connection management, transactions, UDFs
- `dev/storage_impl.h` - Implementation details
- `dev/connection_holder.h` - RAII wrapper for sqlite3 connections

**Schema definition:**
- `dev/schema/table.h` - Table definitions
- `dev/schema/column.h` - Column definitions and constraints
- `dev/schema/view.h` - View support
- `dev/schema/virtual_table.h` - Virtual table support
- `dev/schema/index.h` - Index support
- `dev/schema/triggers.h` - Trigger support
- `dev/schema/constraints/` - One header per constraint (`primary_key`, `foreign_key`, `check`, ...)
- `dev/schema/algorithms/` - Algorithms operating across the whole schema, e.g. `sync_order.h`
- `dev/vtabs/` - Built-in virtual tables (fts5, rtree, dbstat, generate_series)

**Vocabulary layer** (see [Header layers](#header-layers) below):
- `dev/vocabulary/traits/` - Open classification traits, specialized at each node's own header
- `dev/vocabulary/algorithms/` - Closed algorithms composed from those traits
- `dev/vocabulary/node_traits.h`, `dev/vocabulary/node_algorithms.h` - Declaration-only umbrellas
- `dev/node_definitions.h`, `dev/node_algorithm_definitions.h` - Completeness manifests
- `dev/member_traits/` - Pointer-to-member mechanics; one tier below the DSL

**Query building:**
- `dev/conditions.h` - WHERE clause conditions
- `dev/core_functions.h` - SQL functions
- `dev/select_constraints.h` - SELECT modifiers (ORDER BY, LIMIT, etc.)
- `dev/ast/` - AST nodes for DML and operational constructs (`select_t`, `insert_t`, `where`, `window`, ...)

**Type binding:**
- `dev/statement_binder.h` - Binds C++ values to prepared statements
- `dev/row_extractor.h` - Extracts C++ objects from result rows
- `dev/field_printer.h` - Serializes field values

**Utilities:**
- `dev/prepared_statement.h` - Prepared statement support
- `dev/ast_iterator.h` - Traverses expression ASTs
- `dev/transaction_guard.h` - RAII transaction guards
- `dev/cte_storage.h` - Common Table Expression support

### Header Organization

`dev/` is the source of truth: ~30,000 lines across ~184 headers, organized by
functionality. `include/sqlite_orm/sqlite_orm.h` is the **generated** single-header
amalgamation of `dev/`. Never edit it by hand — change `dev/` and regenerate.
`not_single_header_include/` holds the non-amalgamated variant.

### Header layers

**Read [`docs/internals/vocabulary-layer.md`](docs/internals/vocabulary-layer.md) before
adding, moving or renaming anything under `dev/`.** It is the authority on where internal
code belongs and why; what follows is only enough to know when to consult it.

sqlite_orm models SQL as a compile-time DSL of individually named node structs. A
*vocabulary layer* of traits and algorithms lets the rest of the library program against
classification and capability ("is this a column", "is this a select expression") instead
of against concrete node types. The placement test for anything new:

- Specialized against something concrete — a node struct or a raw type? → `dev/vocabulary/traits/`
- Composed from other vocabulary, but closed and single-node-scoped ("I already have a node, tell me something about it")? → `dev/vocabulary/algorithms/`
- Searches or traverses *across a collection* to locate or relate nodes? → **not vocabulary.** `dev/schema/algorithms/`

Complexity never determines the tier — only those three questions do. A trait that
composes five others is still a trait.

#### Adding a new DSL node

1. Define the node struct in `dev/schema/` or `dev/ast/`.
2. In **that same header**, specialize whichever vocabulary axes apply (grammar, semantic,
   structural, operand), including the relevant `vocabulary/traits/*_fwd.h` for each.
3. **Register the header in `dev/node_definitions.h`.**
4. Add tests under `tests/`.

Step 3 is not optional and does not fail loudly. The `_fwd` split removed the transitive
inclusion that used to guarantee specializations were compiled in; a missing entry means
the trait silently falls back to its primary template and the node quietly loses its
classification. The test suite is the backstop.

Definition-only headers deliberately excluded from the declaration-only umbrellas —
`vocabulary/algorithms/field_predicates.h` today — are registered in
`dev/node_algorithm_definitions.h` instead.

## Development Workflow

### Code Style
- Follow existing code patterns in the codebase
- C++17 is the baseline; conditional compilation for C++20/23/26 features
- Use template metaprogramming for compile-time type safety
- Member pointers are used extensively for column mapping

### Testing Requirements
- All changes must include tests in the `tests/` directory
- Use Test-Driven Development (TDD): write failing test first
- Tests use Catch2 framework with `TEST_CASE` and `SECTION`
- Ensure tests pass on multiple platforms (CI runs on Linux, Windows, macOS)

### Compilation Considerations
- MSVC requires `/bigobj` flag for 64-bit builds (already configured)
- MSVC requires `/EHsc` for exception handling
- Clang/GCC: watch for `-Wreorder` warnings (treated as errors)
- Large template instantiations may cause long compile times

### Pull Request Guidelines
Per `CONTRIBUTING.md`:
- Create GitHub issue for significant changes (not needed for typos/warnings)
- PR title must begin with issue number: `#9999 : description`
- Base PRs against `dev` branch (not `master`)
- Commit messages in English only
- Squash commits if adding/removing code within same PR
- All tests must pass on CI (Travis, AppVeyor, GitHub Actions)

## Common Patterns

### Creating a storage
```cpp
auto storage = make_storage("database.db",
    make_table("users",
        make_column("id", &User::id, primary_key().autoincrement()),
        make_column("name", &User::name)
    )
);
storage.sync_schema(); // Creates/migrates schema
```

### Query expressions
Queries are built using expression objects that overload operators:
- `where(c(&User::id) == 5)` - type-safe WHERE clause
- `order_by(&User::name)` - ORDER BY
- `limit(10)` - LIMIT
- Composable: `where(...), order_by(...), limit(...)`

### Member pointers for type safety
`&User::id` is used instead of string `"id"`. The library deduces the type and column name from the member pointer at compile time, preventing runtime SQL injection and type mismatches.

## Important Notes

- **Internal structure**: See `docs/internals/vocabulary-layer.md` for header layering and placement rules. It also records open work (traits not yet lifted into the vocabulary layer, `storage_*` → `schema_*` renames) — check it before starting a refactor that may already be planned there.
- **Thread safety**: See `docs/thread-safety.md`. Storage objects are thread-safe by default since v1.10.
- **Schema sync**: `sync_schema()` attempts to preserve data but may drop tables if column constraints change significantly. Back up data before schema changes.
- **In-memory databases**: Use `:memory:` or `""` as filename.
- **Primary key requirement**: CRUD operations like `get()`, `update()`, `remove()` require a primary key column.
- **No raw SQL**: The library's design philosophy avoids raw SQL strings for type safety, but you can use `execute()` for raw queries if needed.

## Branches
`master` is the release branch; active development happens on `dev`, and feature branches
are based on it. Check out the actual current branch with `git branch --show-current`
rather than assuming.
