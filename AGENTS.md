# AGENTS.md

Guidance for AI coding agents working in this repository, regardless of which tool reads it.
The rules below apply to every change; the linked documents are read when a task needs them.

## About sqlite_orm

sqlite_orm is a header-only C++ ORM for SQLite. C++17 is the baseline; C++20/23/26 features
are compiled conditionally. Its only dependency is libsqlite3. It models SQL as a compile-time
DSL of individually named node structs and maps C++ structs to tables through member
pointers. License: AGPL for open source, MIT after purchase.

## Repository layout

- `dev/` - the sources, organized by functionality. **Read
  [`docs/internals/source-layout.md`](docs/internals/source-layout.md) before adding, moving
  or renaming a header.**
- `include/sqlite_orm/sqlite_orm.h` - the **generated** single-header amalgamation of `dev/`.
  Never edit it by hand: change `dev/` and regenerate it (see [Before every commit](#before-every-commit)).
- `not_single_header_include/sqlite_orm/sqlite_orm.h` - the non-amalgamated variant. Its
  includes and the `sources` of `third_party/amalgamate/config.json` list the same headers in
  the same order; change both together (`scripts/check-amalgamation-sources.sh` checks it).
- `modules/` - the C++20 named module; public symbols are declared in
  `SQLITE_ORM_EXPORT namespace sqlite_orm`.
- `tests/` - Catch2 unit tests; `examples/` - one program per feature.
- `docs/internals/` - how the library is built ([index](docs/internals/home.md));
  `docs/plans/` - designs of work in progress, deleted once done ([policy](docs/plans/README.md)).

The main entry points: the storage (`dev/execution/storage.h`, `make_storage()`), the table
definitions (`dev/schema/table.h`, `make_table()`) and the statement serializer
(`dev/serialization/statement_serializer.h`).

## Header rules

**Header layers.** A *vocabulary layer* of traits and algorithms lets the library program
against classification ("is this a column") instead of concrete node types. **Read
[`docs/internals/vocabulary-layer.md`](docs/internals/vocabulary-layer.md) before adding a
trait, predicate or algorithm.** The placement test:

- Specialized against something concrete, a node struct or a raw type? → `dev/vocabulary/traits/`
- Composed from other vocabulary, closed and single-node-scoped? → `dev/vocabulary/algorithms/`
- Searches or traverses across a collection? → not vocabulary: `dev/schema/algorithms/`

Complexity never determines the tier. Where nearby code does it differently, nearby code is
the outlier: follow the doc, and convert the outlier rather than copying it.

**Adding a DSL node** follows "Adding a new DSL node" in the vocabulary layer doc. In short:
a node specializes its vocabulary traits in its own header, over a type predicate rather than
by restating its template parameters; consumers grammar-match it and read it through the
projections, never pattern-matching the concrete node. **Register the header in
`dev/node_definitions.h`**: a missing entry compiles, and the trait silently falls back to its
primary template.

**The configuration is universally available.** Every header can rely on `dev/config/`
without including it: the C++ feature and compiler macros, the platform macros, the SQLite
version and feature macros (`SQLITE_VERSION_NUMBER`, `SQLITE_ORM_JSON_SUPPORTED`, ...), the
capability macros (`SQLITE_ORM_WITH_CTE`, ...) and the internal macros
(`SQLITE_ORM_EXPORT`, ...). This covers configuration only.
A header that uses the SQLite C API (functions, types, constants such as
`SQLITE_OPEN_READONLY`) includes `<sqlite3.h>` itself, and a header that uses a sqlite_orm
symbol includes the header declaring it.

**Never guard an `#include` of a sqlite_orm header**, whether with a feature macro, a SQLite
version check or anything else. The amalgamation inlines a header where it is *first*
included, so a guarded first inclusion drops it from every build where the condition is
false. A header needed in one configuration only guards its *contents*. Guarding standard
library includes is fine.

**Out-of-class member definitions live in a `_definitions.h` header next to their class**,
e.g. `schema/column_definitions.h` next to `schema/column.h`. Only the manifest
`dev/interface_definitions.h` includes them: register a new one there, and never include one
from anywhere else.

## Code style

- Match the surrounding code's patterns, naming and comment density.
- Comments state what an entity is or checks, in one accurate sentence. No comparisons with
  neighbours, justifications or restated context. Facts shared by a group of entities go once
  in the group's heading.
- Type metafunctions: `_of` names a type computed from a node (`column_result_of_t`,
  `table_type_of_t`); `_for` names a selection or construction for given parameters
  (`node_tuple_for`). No names like "proxy" for plain type computations.
- Prefer `std::ranges` algorithms, with projections, wherever the code's C++ standard has
  them.
- Weigh internal design alternatives by what library consumers can observe: their types, call
  sites and diagnostics.

## Development workflow

### Before every commit

Run [`scripts/pre-commit.sh`](scripts/pre-commit.sh), on Linux, macOS or WSL. It regenerates
the amalgamation, formats the sources with clang-format 20 (the version CI lints with), warns
about changed files containing carriage returns, and checks the order of the amalgamation
sources and that the paths named in this file and in `docs/internals/` exist. Every commit carries its own regenerated amalgamation, so that each
one is consistent by itself. Files use LF line endings, enforced by `.gitattributes`. When
moving or renaming a file, update the documents naming it.

### Building and testing

```bash
cmake -B build -DBUILD_TESTING=ON              # C++17; add -DSQLITE_ORM_ENABLE_CXX_20=ON (or _23, _26)
cmake -B build -DBUILD_EXAMPLES=ON             # the examples
cmake --build build
ctest --test-dir build                         # all tests
./build/tests/unit_tests "[ast_iterator]"      # selected by Catch2 test name or tag
```

- Behavioral changes and fixes come with tests, written to fail first. Refactorings and moves
  are covered by the existing suite.
- Tests are organized with `TEST_CASE` and `SECTION`; serializer output is tested under
  `tests/statement_serializer_tests/`.
- CI builds on Linux, Windows and macOS with several compilers and C++ standards.
  `-Werror=reorder` is on for GCC and Clang; MSVC builds with `/bigobj` and `/EHsc`.
- The named module tests (`tests/named_module/`, built from `modules/`) need C++23 and either
  MSVC with a Visual Studio generator or the Ninja generator with CMake 4.5;
  `-DSQLITE_ORM_BUILD_MODULE_TESTS=OFF` skips them. CI runs them in
  `.github/workflows/named-module.yaml` only.

### Branches and pull requests

- `master` is the release branch. Development happens on `dev`: base branches and PRs on it.
  Check the current branch with `git branch --show-current` rather than assuming it.
- Bring a branch up to date by merging `dev`, or the base branch of a stacked PR, into it;
  don't rebase.
- Following [`CONTRIBUTING.md`](CONTRIBUTING.md):
  - discuss significant behavioral changes in an issue first;
  - the PR title describes the change, without an issue number, and the description links
    the issue (`Fixes #9999`);
  - write commit messages in English;
  - squash commits that add code a later commit of the same PR removes.

## References

- [`docs/internals/vocabulary-layer.md`](docs/internals/vocabulary-layer.md) also records
  open work and open questions, `TODO.md` missing features, and `docs/plans/` designs in
  progress: check them before starting a refactor.
- [`docs/internals/review-checklist.md`](docs/internals/review-checklist.md) - what a change
  is reviewed against.
- [`docs/thread-safety.md`](docs/thread-safety.md) - the threading guarantees of a storage.
- `.claude/skills/` - step-by-step recipes in the Agent Skills format, for
  [adding a DSL node](.claude/skills/add-dsl-node/SKILL.md),
  [a built-in SQL function](.claude/skills/add-builtin-function/SKILL.md) or
  [a virtual table](.claude/skills/add-virtual-table/SKILL.md), and for
  [moving or renaming a header](.claude/skills/move-header/SKILL.md).
- `.claude/agents/sqlite-orm-reviewer.md` - a reviewer agent working through the review checklist.
