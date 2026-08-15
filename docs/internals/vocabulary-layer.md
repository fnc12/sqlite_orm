# The DSL vocabulary layer

This document describes how sqlite_orm's internal headers are layered, and — more
importantly — **where new traits, predicates and algorithms belong**. It is written for
contributors and for AI coding tools working in this repository.

It is normative: when you add a trait or an algorithm, the placement test in
[Deciding where something goes](#deciding-where-something-goes) is the rule to apply.

## Why this layer exists

sqlite_orm models SQL as a compile-time DSL. SQL keywords and constructs are represented
as individually named C++ structs ("nodes"), assembled into an AST.

A large amount of internal logic needs to reason *about* those nodes — what SQL role a
node plays, what it carries, whether it can be used a certain way — without depending on
the concrete struct identity of every node individually.

The vocabulary layer is the answer to that. It is a set of traits and composed algorithms
that let the rest of the library program against **classification and capability** ("is
this a select expression", "is this a column") rather than against concrete types. This:

- decouples consumers from node identity,
- centralizes classification logic that was previously ad hoc (`is_same_v` chains,
  scattered SFINAE),
- makes the DSL's own conceptual structure visible in the codebase, rather than implicit
  in behavior.

## The tiers

From the bottom up:

| Tier | Location | Subject |
|---|---|---|
| Language mechanics | `dev/member_traits/` | Pointer-to-member mechanics. Not SQL, not DSL. |
| DSL vocabulary | `dev/vocabulary/` | Classification of, and computation over, a **single** node or type. |
| Node definitions | `dev/schema/`, `dev/ast/` | The concrete node structs, each specializing the vocabulary axes in place. |
| Schema-level algorithms | `dev/schema/algorithms/` | Search/traversal **across a collection** of nodes. Consumes vocabulary; is not vocabulary. |

### `member_traits/` — one tier below the DSL

`member_traits/member_traits.h` holds pointer-to-member mechanics: extracting a member's
field type (getter/setter/object), its enclosing class type, casting a pointer-to-member
from base to derived. Fully closed, no open specialization points.

`member_traits/field_of.h` holds `is_field_of_v` / `is_field_of` (an *open* trait: is a
pointer-to-member of `Base` usable as a pointer-to-member of `Derived`; does a
`column_pointer` match a target type), plus `compare_fields()`.

`is_field_of_v` is open, yet deliberately lives outside `vocabulary/`. It is
pointer-to-member computation — the same subject matter as `member_traits.h` — not DSL
classification. It is also foundational and cross-cutting (used by concepts,
`static_assert`s and overload resolution rather than by any one domain), and must be
eager because concepts reference it directly.

`compare_fields()` is an ordinary function built on top of `is_field_of`; it is neither a
trait nor a predicate, and stays alongside it.

### `vocabulary/` — the layer proper

```
vocabulary/
    node_fwd.h              bare forward declarations needed by several vocabulary files
                            without their full definitions (e.g. column_pointer,
                            primary_key_t, indexed_column_t). Real definitions stay in
                            schema/ and arrive via node_definitions.h.

    traits/                 "What is this?" — see below
    algorithms/             "What follows from that?" — see below

    node_projections.h      Closed single-node accessors (not _fwd — nothing to
                            specialize).

    node_traits.h           Umbrella, declaration-only: traits/*_fwd.h +
                            node_projections.h. Never includes a definition file.
                            Safe for broad inclusion.

    node_algorithms.h       Umbrella, declaration-only: predicates.h, index_filters.h,
                            accessors.h, field_predicates_fwd.h,
                            field_predicates_concepts.h. Deliberately does NOT include
                            field_predicates.h.
```

## The core distinction: classification vs. computation

Every trait and algorithm here falls into exactly one of two categories, and almost every
placement question reduces to identifying which.

**Vocabulary traits (`vocabulary/traits/`)** answer *"what is this"*, or *"what does this
carry"*, about a **single** node or type. Either a primary template that is `false`/empty
by default and specialized **at the definition site of the thing being classified** (an
open, per-node customization point), or a closed accessor that extracts something
directly with no specialization needed.

**Algorithms (`vocabulary/algorithms/`)** answer *"what follows from that"*. Closed,
composed logic that **consumes** vocabulary — several traits and projections at once — to
produce a compound answer: a bool, an index sequence, a reference to a sub-object, a type
tuple. Never itself specialized; one complete definition.

A third category is easily confused with these and sits **outside `vocabulary/`
entirely**: algorithms that search or relate nodes *across a collection*. These do not
classify a node you already have in hand; they locate or relate nodes across the whole
schema. See [Schema-level algorithms](#schema-level-algorithms).

## Deciding where something goes

> **Is it specialized against something concrete** — a node struct or a raw type?
> → `vocabulary/traits/`
>
> **Is it composed from other vocabulary, but closed and single-node-scoped** — you start
> from "I already have a node, tell me something about it"?
> → `vocabulary/algorithms/`
>
> **Does it search or traverse across a collection** to locate or relate nodes — you start
> from "search a collection to find/relate nodes"?
> → **not vocabulary.** `dev/schema/algorithms/`, likely feeding its result into a
> vocabulary algorithm.

Two rules that repeatedly prevent mistakes:

- **Complexity never determines tier.** How many other traits a condition composes, or how
  involved its SFINAE is, has no bearing on where it belongs. `is_raw_dml_expression_v`
  internally composes other grammar traits and the `expression_type_t` projection, and is
  still an open semantic trait. `field_type_or_type_t` uses the detected idiom with a
  fallback, and is still a plain projection. The test is *whether it composes
  classification traits into a judgment*, not whether the logic is hard.
- **Openness is about who writes specializations**, not about how complex they are. See
  [Open vs. closed](#open-vs-closed).

## The trait axes

Four axes are keyed on a DSL node; one is keyed on a raw C++ type.

| Axis | File | Keyed on | Question |
|---|---|---|---|
| **Grammar** | `traits/grammar_traits_fwd.h` | a DSL node | What SQL grammar production is this a member of? |
| **Semantic** | `traits/semantic_traits_fwd.h` | a DSL node | What cross-cutting role does this play, across *dissimilar* grammar families? |
| **Structural** | `traits/structural_traits_fwd.h` | a DSL node | Is this a DSL-tree-only node with no SQL grammar counterpart? |
| **Operand** | `traits/operand_traits_fwd.h` | a DSL node | Can this node appear as an operand in an expression context? |
| **Field** | `algorithms/field_predicates_fwd.h` | a raw C++ type | Does this C++ type qualify for a SQL-level role? |

All four node axes are **open**: the primary template is declared in the `_fwd.h` file, and
each node specializes it **in the node's own header**.

### Grammar

Classifier/leaf traits plus structural grouping by real SQL grammar production:
compound-statement, DML-statement, CTE-binding, function-call, alias, and so on —
`is_column`, `is_base_table`, `is_foreign_key`, `is_compound_operator`,
`is_built_in_function`, `is_binary_condition`, and many more.

### Semantic

Cross-cutting role or capability that groups nodes **not sharing a grammar family**.
`is_select_expression_v` covers both `WITH` and `SELECT`, so a main select can be
extracted from either. `is_raw_dml_expression_v` groups raw DML structs and with-wrapped
DML together.

Distinguishing grammar from semantic in practice: ask whether the nodes being grouped
share an actual SQL grammar production (→ grammar), or only share a capability or role
despite being different productions (→ semantic).

A semantic specialization stays semantic even when the specialization itself is involved
and composes other grammar traits and projections internally.

### Structural

DSL-tree-only nodes with no SQL grammar counterpart, which are nonetheless structurally
required members of the logical AST: ad-hoc column-selection structs (`is_columns`),
mapped result-set objects (`is_object_node`, `is_struct`), `is_indexed_column`.

"Quoting" nodes — literals, pointer-to-member wrappers, anything that lifts a foreign C++
value into the DSL — are structural traits and belong in this file. Should their volume or
coupling ever justify it, they may be split out into a dedicated `quoting_traits_fwd.h`;
that would be a subdivision of the structural axis, not a new axis.

### Operand

An orthogonal axis: can this node appear as an operand in an arithmetic or expression
context, independent of what the node *is* syntactically —
`is_arithmetic_operand`, `is_conditional_operand`, `is_negatable_operand`,
`is_chainable_operand`, `is_operator_argument`.

Orthogonality is the point. A node can need grammar **and** operand simultaneously
(aliases and function calls have a real grammar production *and* are usable as operands).
A node can be operand-only with no grammar trait (quoting wrappers). A node can have a
grammar trait and no operand capability at all (CTE bindings).

### Projections — `node_projections.h`

Closed, and deliberately **not** a `_fwd.h`, because there is nothing to specialize.
Direct single-node accessors, in two shapes:

- plain nested-typedef access — `constraints_type_t`, `field_type_t`, `elements_type_t`,
  `object_type_t`, `expression_type_t`, `left_type_t`/`right_type_t`, …
- detected-idiom accessors with a fallback — `field_type_or_type_t`,
  `alias_holder_type_or_none_t`.

The extra work in the second shape does not promote it to the algorithm tier. Nothing is
being composed into a judgment; it is still extraction.

## The algorithms

| File | Contents |
|---|---|
| `algorithms/predicates.h` | Closed, composed alias-template validity checks with no significant header-weight dependency, e.g. `is_pkcol_implicitly_insertable`. Single file; no split needed. |
| `algorithms/index_filters.h` | Closed alias templates that scan a node's `Elements` tuple and yield an `index_sequence` of matching positions — **not** a filtered tuple. E.g. `col_index_sequence_of`, `col_index_sequence_with_field_type`. Built on `filter_tuple_sequence_t` + grammar traits + projections. |
| `algorithms/accessors.h` | Closed runtime and compile-time accessors that retrieve a node's relevant sub-part, or the node itself, uniformly across dissimilar grammar families: `access_main_select`/`main_select_t`, `access_main_dml`/`main_dml_t`, `access_column_expression`. This is the concrete payoff of the semantic traits. |
| `algorithms/field_predicates_fwd.h` | Declarations of the closed field-level predicates, split off for dependency weight: `is_rowid_alias_capable_v`, and (C++17 only) `is_hidden_column_of_vtab_v`. |
| `algorithms/field_predicates.h` | Their definitions, which need `type_printer.h` and `member_traits/`. Reached only through the `node_algorithm_definitions.h` manifest. |
| `algorithms/field_predicates_concepts.h` | The `hidden_column_of_vtab` / `hidden_field_of_vtab` concepts, plus the C++17 fallback, under one guard. See [Concepts](#concepts-a-hard-constraint-on-splitting). |

## Open vs. closed

A trait or predicate is **open** if independent call sites add new specializations over
time. All four node axes are open, as is `is_field_of_v`.

It is **closed** if it has a fixed, complete set of conditions written once and never
independently extended — predicates, index filters, accessors, projections — *even if
those conditions are individually complex or SFINAE-heavy*.

`is_rowid_alias_capable_v` is a good illustration: it is **closed**, despite looking like
an extension point. New types qualify automatically through `type_printer` derivation;
nobody adds another specialization of the predicate itself.

### The two independent reasons for a `_fwd`/impl split

They do not always coincide.

1. **Openness.** The primary template must be visible at multiple independent
   specialization sites before those sites can specialize it. This is why every
   node-classification axis is a `_fwd.h`.
2. **Dependency-weight isolation.** Even a fully closed trait can be worth splitting when
   evaluating it requires a heavy header that most consumers of the trait's *name* — in a
   signature, say — should not have to pay for. `is_rowid_alias_capable_v` needs
   `type_printer.h`, so its declaration and definition are separated.

Reason 2 only actually saves anything **if the umbrella includes the declaration file but
not the definition file**. `node_algorithms.h` includes `field_predicates_fwd.h` and does
not include `field_predicates.h`; the definition is reachable only through the explicit
`node_algorithm_definitions.h` manifest. Were both pulled in by the same umbrella, the
split would be cosmetic and buy nothing.

**Do not split by reflex.** Closedness alone never requires a split. Add one only when
there is a real, identifiable header-weight cost being deferred — and confirm the umbrella
actually excludes the definition file. Lightweight closed predicates stay single files.

## Concepts: a hard constraint on splitting

**Concepts cannot be forward-declared.** A concept's full body must be visible wherever it
is checked. There is no "declare now, define later" mechanism, unlike a variable
template's primary-false/specialize-later shape.

How that interacts with the framework above depends on *why* the concept exists:

- A concept that is a thin forwarding shim over an already-closed bool
  (`concept C = is_x_v<T>;`) costs nothing to declare early. You get the diagnostic *name*
  in a failed `requires`-clause at no header-weight cost, because the real condition still
  only needs to be visible where the underlying variable template is specialized.
- A concept written with its **own nested `requires` sub-clauses** — chosen deliberately
  for diagnostic transparency, so the compiler reports exactly which nested check failed
  instead of one opaque pass/fail — *is* its own condition. There is no boundary left to
  split across: the body directly names the projections and traits it depends on, so it
  must live wherever those are visible.

The second case forces the concept's entire definition into a file that is **eagerly
included** by the umbrella, accepting the header weight that would otherwise have been
deferred. This is an inherent trade — transparency and deferred weight are in direct
tension for a concept written this way — not a design flaw to route around.

`field_predicates_concepts.h` is the resolution: a dedicated `_concepts.h` file, eagerly
included by `node_algorithms.h`, holding both the concept and its C++17 non-concepts
fallback together under one `SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED` guard *inside the file*.
Both branches check the identical condition over the identical dependencies, so there is
no benefit to separating them.

This pattern generalizes to any future concept, in traits or algorithms, needing the same
transparency-over-weight-isolation trade.

## Schema-level algorithms

`dev/schema/algorithms/` holds algorithms that search or relate nodes **across a
collection**, mirroring `vocabulary/algorithms/` one tier up. They routinely *consume*
vocabulary, which is exactly why they are easy to misfile.

`schema/algorithms/sync_order.h` is the reference example. It computes the order in which
database objects must be synced — a stable topological sort over the `db_objects_tuple`,
deferring indexes and triggers until after their target table or view. It consumes
`vocabulary/node_traits.h` for the `object_type_t` and `table_mapped_type_t` projections,
but it is not vocabulary: it starts from a *collection* and relates its members, rather
than classifying a node already in hand.

Table lookup (`storage_find_table` / `storage_pick_table` / `enable_found_table`, searching
`db_objects_tuple` for a matching DBO) is the same tier. It currently still lives in
top-level `dev/storage_lookup.h` — see [Open work](#open-work).

## The manifests

The `_fwd` split removed the incidental transitive inclusion that used to guarantee every
specialization got compiled in. Two manifest headers, **siblings to `vocabulary/` and
never nested inside it**, restore that guarantee. Their job — aggregating concrete
definitions — is categorically different from vocabulary's job of declaring and computing
classification.

- **`dev/node_definitions.h`** — an explicit `#include` of every concrete node header in
  `schema/` and `ast/`. Its only job is completeness.
- **`dev/node_algorithm_definitions.h`** — the definition-only files deliberately *not*
  aggregated by `node_algorithms.h`: `field_predicates.h` today, and any future
  algorithm-tier item split for the same dependency-weight reason.

> **When you add a node header under `schema/` or `ast/`, add it to `node_definitions.h`.**
> A missing entry does not fail to compile. The trait silently falls back to its primary
> template, and the node quietly loses its classification.

The practical backstop against that silent fallback is the project's near-100% unit test
coverage: a real behavioral test failure is a stronger signal than an isolated trait
self-check would be.

## Adding a new DSL node

1. Define the node struct in `dev/schema/` or `dev/ast/`.
2. In **that same header**, specialize whichever vocabulary axes apply — grammar,
   semantic, structural, operand. Include the relevant `traits/*_fwd.h` for each.
3. Register the header in `dev/node_definitions.h`.
4. Add tests under `tests/`.

If the node's name is needed by vocabulary files that cannot include its full definition,
add a bare forward declaration to `vocabulary/node_fwd.h` — the real definition still
arrives via the manifest.

## Open work

Decided, not yet done. The destination is settled in each case; only the work remains.

- **Lift the remaining traits into the vocabulary layer.** A substantial number of node
  classification traits are still declared *and* specialized inside their concrete node
  headers, having never been lifted into the vocabulary layer by forward-declaring a
  primary template in the appropriate `traits/*_fwd.h`. Examples:
  `is_alias_v` / `is_column_alias_v` / `is_recordset_alias_v` / `is_table_alias_v`,
  `is_cte_moniker_v`, `is_insert_v` / `is_insert_range_v` / `is_insert_raw_v` and the
  `is_replace_*` family, `is_update_all_v`, `is_remove_all_v`, `is_upsert_clause_v`,
  `is_over_v`, `is_partition_by_v`, `is_window_defn_v`, `is_values_v`,
  `is_table_valued_expression_v`, `is_literal_v`.

  Each needs triage before being moved — not every `is_*_v` in a node header is DSL node
  classification. Several are language- or binding-level mechanics
  (`is_stateless_deleter_v`, `is_unusable_for_xdestroy_v`, `is_bindable_v`,
  `is_printable_v`, `is_integral_fp_c_v`) and belong where they are, or one tier below.
  For those that do qualify, apply the axis table above to pick the file.

- **`storage_traits.h`.** Its `storage_mapped_columns_impl` and
  `storage_mapped_column_expressions_impl` are closed, single-table, classification-driven
  computations over an already-located DBO — `vocabulary/algorithms/` items, despite the
  "traits" in the filename. They should move, and the file should be renamed to something
  that says what it computes (`column_field_types.h`, or similar).

  **Naming:** drop `storage` in favour of `schema` when these move. These algorithms
  operate on the schema — the mapped database objects — not on the `storage_t` object.
  `storage_mapped_columns` → `schema_mapped_columns`, and so on.

- **`storage_lookup.h`.** Schema-level traversal; belongs under `dev/schema/algorithms/`
  alongside `sync_order.h`.

  **Naming:** same substitution — `storage_find_table` → `schema_find_table`,
  `storage_pick_table` → `schema_pick_table`, and correspondingly for the rest of the file.

- **`type_printer` / `integer_printer`.** A genuine open per-raw-type customization point,
  the same shape as the field traits, but still in its current public, pre-existing
  location. Real work, not urgent.

## Open questions

Deliberately unresolved — the destination itself is undecided. Revisit when there is a
concrete reason to, not before.

- **Struct→table / expression→table mapping.** A further tier above both `member_traits/`
  and `vocabulary/`: schema-wide resolution and lookup, likely *consuming* vocabulary
  rather than being vocabulary. Not yet reviewed against real code; naming and folder
  undecided.
