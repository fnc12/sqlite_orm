---
name: add-dsl-node
description: Use when adding a new DSL node to sqlite_orm - a new SQL expression, operator, clause or statement kind in dev/ast/ or dev/schema/. Lists every file a node touches, from the vocabulary traits to serialization, traversal, result type and tests.
---

# Adding a DSL node

The rules are in `AGENTS.md` ("Adding a DSL node") and `docs/internals/vocabulary-layer.md`
("Adding a new DSL node", "How a node specializes a trait", "How consumers match a node"). Read
those sections first; this is the list of places a node touches. `cast_t` in `dev/ast/cast.h` is
a compact example to compare with.

## 1. The node and its factory

- The node struct goes into the header of its feature in `dev/ast/` (or `dev/schema/`), in
  `namespace sqlite_orm::internal`, following `docs/internals/source-layout.md`. Members are
  exposed through type aliases (`expression_type`, `left_type`, ...) that the vocabulary
  projections read.
- The public factory goes into `SQLITE_ORM_EXPORT namespace sqlite_orm` in the same header, with a
  comment naming the SQL it produces and an example. Gate it on valid operands the way neighbouring
  factories do (`dev/vocabulary/algorithms/operand_predicates.h`).

## 2. Vocabulary

- A new grammar classification is declared in `dev/vocabulary/traits/grammar_traits_fwd.h`
  (`extern const bool is_x_v;` plus `using is_x = std::bool_constant<is_x_v<T>>;`, with a one-line
  comment) and defined in the node's header over a type predicate:
  `constexpr bool is_x_v = polyfill::is_specialization_of_v<T, x_t>;`.
- Specialize the other axes that apply (semantic, structural, operand, e.g.
  `is_operator_argument_v`) in the node's header too, as SFINAE enablers on `is_x_v`.
- Register the header in `dev/node_definitions.h`. Nothing fails at compile time if this is
  missing; the trait silently keeps its primary template.

## 3. Consumers

Each consumer matches the node by its grammar trait (`match_if<is_x, T>`) and reads it through
projections, never by spelling out `x_t<...>`:

- `dev/serialization/statement_serializer.h` - `statement_serializer<T, match_if<is_x, T>>`.
- `dev/traversal/ast_iterator.h` - `ast_iterator<T, match_if<is_x, T>>`, visiting each
  subexpression, so that table names, bindables and columns inside the node are found.
- `dev/traversal/node_tuple.h` - `node_tuple<T, match_if<is_x, T>>` over the subexpressions.
- `dev/result_set/column_result.h` - `column_result_of<DBOs, T, match_if<is_x, T>>`, if the node
  yields a value.
- Any further consumer the node takes part in (e.g. the collectors in `dev/serialization/`).

## 4. Tests

- Serialization: `tests/statement_serializer_tests/`, in the file of the feature.
- Classification, operand validity, node tuple and result type: `tests/static_tests/`
  (`expression_classification.cpp`, `operand_validity.cpp`, `node_tuple.cpp`,
  `column_result_of.cpp`).
- Traversal: `tests/ast_iterator_tests.cpp`.
- Execution against a storage: the feature's runtime test file, e.g. `tests/operators/cast.cpp`.
- New test files are picked up by the glob in `tests/CMakeLists.txt`.

## 5. Finish

Run `scripts/pre-commit.sh`, and have the change reviewed against
`docs/internals/review-checklist.md`.
