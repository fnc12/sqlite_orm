# The DSL vocabulary layer

How sqlite_orm's internal headers are layered, and **where new traits, predicates and
algorithms belong**. Normative: the placement test in
[Deciding where something goes](#deciding-where-something-goes) is the rule to apply.

## Why this layer exists

sqlite_orm models SQL as a compile-time DSL of individually named node structs. The
vocabulary layer lets the rest of the library program against **classification and
capability** ("is this a select expression", "is this a column") instead of concrete node
types, so consumers do not depend on node identity and classification is written once.

## The tiers

From the bottom up:

| Tier | Location | Subject |
|---|---|---|
| Language mechanics | `dev/functional/` | General-purpose C++ utilities, among them pointer-to-member mechanics. Not SQL, not DSL. |
| DSL vocabulary | `dev/vocabulary/` | Classification of, and computation over, a **single** node or type. |
| Node definitions | `dev/schema/`, `dev/ast/` | The concrete node structs, each specializing the vocabulary axes in place. |
| Schema-level algorithms | `dev/schema/algorithms/` | Search/traversal **across a collection** of nodes. Consumes vocabulary; is not vocabulary. |

### Pointer-to-member mechanics — one tier below the DSL

`functional/member_traits.h` holds closed pointer-to-member mechanics: a member's field type
(getter/setter/object), its enclosing class, casting from base to derived.
`functional/field_of.h` holds `is_field_of_v` (is a pointer-to-member of `Base` usable as one of
`Derived`) and `compare_fields()`. `is_field_of_v` is open: `ast/column_pointer.h`
specializes it for a `column_pointer` of the target type, so this tier depends on nothing in
the DSL. It stays outside `vocabulary/`: it is pointer-to-member computation, not DSL
classification.

### `vocabulary/` — the layer proper

```
vocabulary/
    node_fwd.h          bare forward declarations several vocabulary files need without the
                        full definitions (column_pointer, primary_key_t, ...); the definitions
                        arrive via node_definitions.h
    traits/             "What is this?"
    projections/        "What does it carry?"
    algorithms/         "What follows from that?"
    node_traits.h       umbrella, declaration-only: traits/*_fwd.h, projections/*.h (one tier of
                        question, so no umbrella of their own), and traits/recordset_concepts.h
                        (concepts cannot be forward-declared)
    node_algorithms.h   umbrella, declaration-only: the algorithms, except field_predicates.h
```

Consumers include the umbrellas, never a file behind them; that arrangement is free to
change. Included by name from outside are only the files existing for that purpose: a
`traits/*_fwd.h` (or `algorithms/field_predicates_fwd.h`) from a header specializing it, and
the manifest `node_algorithm_definitions.h`.

## The core distinction: classification vs. computation

**Traits (`vocabulary/traits/`)** answer *"what is this"* about a **single** node or type: a
primary template, `false` or empty by default, specialized **at the definition site of what
it classifies**.

**Projections (`vocabulary/projections/`)** answer *"what does this carry"*: closed
accessors extracting a type from a single node.

**Algorithms (`vocabulary/algorithms/`)** answer *"what follows from that"*: closed logic
composing several traits and projections into a compound answer — a bool, an index sequence,
a sub-object, a type tuple. One complete definition, never specialized.

Algorithms that search or relate nodes *across a collection* are **not vocabulary**; see
[Schema-level algorithms](#schema-level-algorithms).

## Deciding where something goes

> **Is it specialized against something concrete** — a node struct or a raw type?
> → `vocabulary/traits/`
>
> **Is it composed from other vocabulary, but closed and single-node-scoped** — "I already
> have a node, tell me something about it"?
> → `vocabulary/algorithms/`
>
> **Does it search or traverse across a collection** to locate or relate nodes?
> → **not vocabulary.** `dev/schema/algorithms/`, possibly feeding a vocabulary algorithm.

- **Complexity never determines the tier.** `is_raw_dml_expression_v` composes grammar
  traits and a projection and is still an open semantic trait; `alias_holder_type_or_none_t`
  uses the detection idiom and is still a projection. The test is whether classification
  traits are composed into a judgment.
- **Openness is about who writes specializations**, not how complex they are; see
  [Open vs. closed](#open-vs-closed).

Not every `is_*_v` belongs in the layer:

- **Language- or binding-level mechanics** (`is_stateless_deleter_v`, `is_bindable_v`,
  `is_printable_v`, `is_integral_fp_c_v`) stay where they are, or one tier below.
- **Definition-time mechanics over something that never becomes a node** stay with their
  consumer, e.g. `is_dbo_name_literal_v` in `schema/dbo_name.h`.
- **Closed predicates naming concrete nodes for a single consumer**
  (`is_builtin_numeric_column_alias_v`) stay next to those nodes.
- **Admissibility composites** over existing vocabulary are algorithms, not traits.

### Which algorithms file — the level of the construct

A membership check is placed by the grammar level of the construct it is a member of, not by
the grammar family it is about:

| Level of the construct | File | Question |
|---|---|---|
| Schema | `ddl_predicates.h` | Is this an admissible element of a column, table or index definition? |
| Statement | `clause_predicates.h` | Is this an admissible clause of a statement, and in what order? |
| Expression | `expression_element_predicates.h` | Is this an admissible element of a compound expression construct? |

So a window-definition element check sits next to the frame boundary checks, as a new table
constraint check sits next to the column constraint checks. Whether a clause or element holds
an expression rather than another clause is enforced by the factories, not by these files.

The other algorithms files are chosen by what they compute:

| File | Holds |
|---|---|
| `operand_predicates.h` | Whether a type may be an operand of a named expression factory (`are_valid_operands`). |
| `index_filters.h` | Index sequences of the matching positions in a tuple of nodes — table elements or a flattened expression — not filtered tuples. |
| `accessors.h` | Uniform access to a node's relevant sub-part across dissimilar grammar families (`access_main_select`); the payoff of the semantic traits. Also `mapped_type_of_t`, the mapped object a record set stands for. |
| `field_accessors.h` | The same, keyed on a raw C++ type: the object a holder type holds (`held_object_t`). |
| `column_field_types.h` | Type tuples over the columns of a single table definition. |
| `argument_placeholders.h` | The return type placeholders of the built-in functions and their structural substitution. |
| `field_predicates_fwd.h` | Declarations of the field-level predicates; their definitions are spread as described there. |
| `field_predicates.h` | Definitions of the derived field predicates, reached only through the manifest. |
| `field_predicates_concepts.h` | The hidden-column concepts with their C++17 fallback. |
| `recordset_predicates.h` | The concepts whether a node may occupy a record set slot, composing the single-node concepts of `traits/recordset_concepts.h` across axes. |

## The trait axes

| Axis | File | Keyed on | Question |
|---|---|---|---|
| **Grammar** | `traits/grammar_traits_fwd.h` | a DSL node | What SQL grammar production is this a member of? |
| **Semantic** | `traits/semantic_traits_fwd.h` | a DSL node | What role does this play across *dissimilar* grammar families? |
| **Structural** | `traits/structural_traits_fwd.h` | a DSL node | Is this a DSL-tree-only node with no SQL grammar counterpart? |
| **Operand** | `traits/operand_traits_fwd.h` | a DSL node | Can this node appear as an operand in an expression context? |
| **Field** | `algorithms/field_predicates_fwd.h` | a raw C++ type | Does this C++ type qualify for an SQL-level role (bindable, printable, mapped to INTEGER, ...)? |

The four node axes are **open**: the primary template is declared in the `_fwd.h` file, and
each node specializes it **in its own header**. `traits/recordset_concepts.h` holds the
public concepts that are the faces of single grammar or structural traits (`orm_alias` …
`orm_cte_moniker`, `orm_table_reference`, `orm_table_valued_expression`); see
[Concepts](#concepts-a-hard-constraint-on-splitting).

- **Grammar** — membership in a real SQL production: `is_column`, `is_base_table`,
  `is_compound_operator`, `is_builtin_function_call`, `is_binary_condition`, ...
- **Semantic** — a role shared by nodes of *different* productions: `is_select_expression_v`
  covers `WITH` and `SELECT`, `is_raw_dml_expression_v` raw and with-wrapped DML. Grammar if
  the grouped nodes share a production, semantic if they only share a role.
- **Structural** — tree-only nodes the AST nonetheless needs: `is_columns`,
  `is_object_node`, `is_struct`, `is_indexed_column`, and the "quoting" nodes lifting a C++
  value into the DSL (literals, pointer-to-member wrappers). A `quoting_traits_fwd.h` split,
  if ever warranted, subdivides this axis.
- **Operand** — orthogonal to what a node is: `is_arithmetic_operand`,
  `is_conditional_operand`, `is_negatable_operand`, `is_chainable_operand`,
  `is_operator_argument`. A node may have grammar and operand traits (aliases, function
  calls), operand only (quoting wrappers), or grammar only (CTE bindings).

### Projections — `projections/`

Closed, so no `_fwd.h` files.

- `nested_types.h` reads a type the node declares: `expression_type_t`, `field_type_t`,
  `left_type_t`/`right_type_t`, ..., and detected accessors with a fallback
  (`alias_holder_type_or_none_t`).
- `mapped_types.h` destructures a node to a type it captured, naming the nodes it matches
  (hence it needs `node_fwd.h`): `table_type_of` (the class of a pointer-to-member, the `T` of
  a `column_pointer`), `decay_table_ref`. Whether that type is mapped by a schema is a
  schema-wide lookup, in `schema/algorithms/table_lookup.h`.

## Open vs. closed

A trait is **open** if independent call sites add specializations over time: the four node
axes and `is_field_of_v`. It is **closed** if its conditions are written once — predicates,
index filters, accessors, projections — however complex. `is_rowid_alias_capable_v` is
closed although it looks like an extension point: types qualify through `type_printer`
derivation, nobody specializes the predicate.

### The two independent reasons for a `_fwd`/impl split

1. **Openness.** The primary template must be visible at every specialization site; hence
   each node axis is a `_fwd.h`.
2. **Dependency weight.** A closed trait whose definition needs a heavy header
   (`is_rowid_alias_capable_v` needs `type_printer.h`) is split so that naming it stays
   cheap. This saves something only if the umbrella includes the declaration file and not
   the definition file: `node_algorithms.h` includes `field_predicates_fwd.h`, and
   `field_predicates.h` is reached only through `node_algorithm_definitions.h`.

**Do not split by reflex.** Closedness alone never requires a split; add one only for a real
header-weight cost, with the umbrella excluding the definition file.

## Concepts: a hard constraint on splitting

Concepts cannot be forward-declared.

- A concept forwarding to a closed bool (`concept C = is_x_v<T>;`) costs nothing to declare
  early: the condition still lives where the variable template is defined.
- A concept with its own nested `requires` checks, written so that a failed constraint names
  the failing check, *is* its condition and must live where everything it names is visible.

The latter goes into a dedicated `_concepts.h` file, included eagerly by its umbrella, with
its C++17 fallback under the same `SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED` guard in the same
file: `algorithms/field_predicates_concepts.h`, and `traits/recordset_concepts.h`, whose alias
concepts need only the forward-declared `alias_tag`. Concepts composing those across axes are
algorithms, in `algorithms/recordset_predicates.h`.

## Schema-level algorithms

`dev/schema/algorithms/` holds algorithms that search or relate nodes **across a
collection**. They consume vocabulary, which is why they are easily misfiled. The reference
example is `sync_order.h`: a topological sort of the `db_objects_tuple`, relating its members
rather than classifying a node in hand. `table_lookup.h` finds the database object mapping a
lookup type ("table" in the wide sense: base tables, views and virtual tables) and feeds it
into the vocabulary algorithms of `column_field_types.h`,
`column_lookup.h` a column across the schema, and `table_filters.h` selects tables or views.

They operate on the **schema**, not on `storage_t`, so they are named `schema_*`, never
`storage_*`. The facilities for the tuple itself (`db_objects_tuple`, `db_objects_cat`,
`db_objects_for_expression`) are not algorithms and sit in `schema/db_objects.h`.

## The manifests

The `_fwd` split removed the transitive inclusion that used to compile every specialization
in. Two manifests, **siblings to `vocabulary/`**, restore it:

- **`dev/node_definitions.h`** — an `#include` of every node header in `schema/` and `ast/`.
- **`dev/node_algorithm_definitions.h`** — the definition-only files excluded from
  `node_algorithms.h`: `field_predicates.h` today.

The top-level headers (`not_single_header_include/sqlite_orm/sqlite_orm.h`, and
`third_party/amalgamate/sqlite_orm.json` in the same order) include them, not the trait's users.
That suffices for traits evaluated inside templates, i.e. in user code. A trait evaluated for
a concrete type while the headers are processed — a customization point instantiated for
`std::string` in non-template code — needs its definition earlier: hence the manifests, and
`marshaling/pointer_passing.h`, come ahead of `storage.h`. Otherwise the evaluation binds to the
bare `extern const` declaration, which MSVC rejects as not a constant expression and GCC and
Clang silently accept.

> **When you add a node header under `schema/` or `ast/`, add it to `node_definitions.h`.**
> A missing entry compiles: the trait silently falls back to its primary template. The unit
> tests are the backstop.

## Adding a new DSL node

1. Define the node struct in `dev/schema/` or `dev/ast/`.
2. In **that same header**, specialize the axes that apply, including the relevant
   `traits/*_fwd.h`, as described in [How a node specializes a trait](#how-a-node-specializes-a-trait).
3. Register the header in `dev/node_definitions.h`.
4. Add tests under `tests/`.

If vocabulary files need the node's name without its definition, add a forward declaration to
`vocabulary/node_fwd.h`.

### How a node specializes a trait

A trait the node's header *owns* — its primary is the `extern const bool` declaration in the
`_fwd.h` file — is **one closed definition** over a type predicate, never a partial
specialization spelling out the node's template parameters:

```cpp
template<class T>
constexpr bool is_app_function_call_v = polyfill::is_specialization_of_v<T, app_function_call>;

template<class T>
constexpr bool is_builtin_function_call_v = is_base_template_of<builtin_function_call, T>::value;

template<class T>
constexpr bool is_count_asterisk_v =
    polyfill::is_specialization_of_v<T, count_asterisk_t> || std::is_same_v<T, count_asterisk_without_type>;
```

A trait the node *joins* — whose primary is already defined `false`, such as
`is_operator_argument_v` — is specialized with a **SFINAE enabler on the node's own grammar
trait**:

```cpp
template<class T>
constexpr bool is_operator_argument_v<T, std::enable_if_t<is_app_function_call_v<T>>> = true;
```

The node's template-parameter list is its implementation detail: restating it at each
specialization site means a new parameter or a derived node (`builtin_aggregate_function_call`
from `builtin_function_call`) silently breaks or forks the classification. A pattern
specialization is acceptable only where the predicates cannot express the match — nodes with
non-type template parameters, such as `limit_t`.

Both are placed **directly after the node they classify**; a definition covering derived
nodes follows the base it names.

Every DSL node gets a trait of its own, or is covered by one of its family —
`union_t`/`except_t`/`intersect_t` by `is_compound_operator`, the built-in functions of every
kind by `is_builtin_function_call`. Builder intermediates (`simple_case_builder`,
`conflict_target`) and element types no consumer visits (`then_t`, `dynamic_set_entry`) are
not nodes in this sense.

### How consumers match a node

Customization points — `statement_serializer`, `ast_iterator`, `node_tuple`,
`column_result_of`, callables handed to `iterate_ast` — **grammar-match** a node and **read it
through the projections**, never pattern-matching the concrete node:

```cpp
// yes
template<class T>
struct node_tuple<T, match_if<is_app_function_call, T>> : node_tuple<args_tuple_t<T>> {};

template<class T, satisfies<is_app_function_call, T> = true>
void operator()(std::true_type, const T& udfCall) const;

// no
template<class F, class... Args>
struct node_tuple<app_function_call<F, Args...>, void> : node_tuple_for<Args...> {};
```

If a consumer needs something the projections do not expose, add the projection to
`vocabulary/projections/` rather than reaching into the node.

## Open work

Decided, not yet done.

- **Retire the legacy built-in function nodes with the C++17 baseline:** delete the
  `#ifndef SQLITE_ORM_WITH_CPP20_ALIASES` branches — `builtin_function_t` and its kin in
  `ast/builtin_function.h`, and the `*_string` tags and factories in `builtin/functions/*.h`.

## Open questions

Deliberately unresolved; revisit when there is a concrete reason to.

- **Struct→table / expression→table mapping.** A possible tier above `functional/` and
  `vocabulary/`: schema-wide resolution, consuming vocabulary. Naming and folder undecided.
  `table_type_of` is not this tier: it extracts a type a node captured, without a schema.
- **Names of the built-in function definitions in `internal`** (`max`, `min`, `count`, ...).
  Class members shadow them, but a namespace-scope internal helper of the same name would
  clash. A nested `internal::builtins` namespace is the escape hatch, once a clash occurs.
