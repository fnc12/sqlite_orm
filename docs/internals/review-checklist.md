# Review checklist

The repository's conventions a change is checked against, beyond its correctness. Each item
names the document that holds the rule; the rules themselves are not repeated here.

## Placement

- Every new or moved header sits in the folder [the source layout](source-layout.md) assigns
  to its content.
- Every new trait, predicate or algorithm passes the placement test of
  [the vocabulary layer](vocabulary-layer.md): concrete specialization → `vocabulary/traits/`,
  closed single-node composition → `vocabulary/algorithms/`, search across a collection →
  `schema/algorithms/`.
- A new top-level header appears in both `not_single_header_include/sqlite_orm/sqlite_orm.h`
  and `third_party/amalgamate/sqlite_orm.json`, in the same order
  (`scripts/check-amalgamation-sources.sh`).

## Vocabulary

- A new DSL node specializes its vocabulary axes in its own header, over a type predicate or
  a SFINAE enabler on its grammar trait, not by restating its template-parameter pattern.
  Pattern specialization is acceptable only for nodes with non-type template parameters.
- The header of a new node is registered in `dev/node_definitions.h`; a definition-only
  algorithm header in `dev/node_algorithm_definitions.h`.
- Consumers grammar-match and read through projections; they do not pattern-match or
  decompose concrete nodes. Code copied from an outlier that does is a finding, and so is
  the outlier.

## Includes

- No `#include` of a sqlite_orm header is guarded by a condition.
- A header using the SQLite C API includes `<sqlite3.h>`; a header using a sqlite_orm symbol
  includes the header declaring it. A missing include of a `dev/config/` header is not a finding.
- `_definitions.h` headers are included only from `dev/interface_definitions.h`, and a new
  one is registered there.

## Public interface

- Public symbols are declared in `SQLITE_ORM_EXPORT namespace sqlite_orm`, implementation
  details in `sqlite_orm::internal`.
- Code depending on a C++ standard or SQLite version is guarded with the macros of
  `dev/config/config.h` and `sqlite3_config.h`, and has a fallback or is omitted
  consistently in tests and examples.

## Consistency

- `include/sqlite_orm/sqlite_orm.h` is regenerated in the same commit as the `dev/` change.
- Sources are formatted with clang-format 20 and use LF line endings
  (`scripts/pre-commit.sh`).
- Names follow the `_of`/`_for` convention of [AGENTS.md](../../AGENTS.md#code-style);
  comments state what an entity is or checks, without comparisons or justifications.

## Tests

- Behavioral changes and fixes come with tests; serializer output is checked under
  `tests/statement_serializer_tests/`.
- Tests guarded by a version or feature macro use the same condition as the code they test.
