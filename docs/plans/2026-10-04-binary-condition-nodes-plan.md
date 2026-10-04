# Binary condition nodes: shared collation, data-driven definitions

Date: 2026-10-04
Branch base: `refactor/split-conditions` (#1550), which gave the binary conditions their own header,
`dev/ast/binary_condition.h`. The model for step 3 is `binary_operator` in `dev/ast/operators.h`, as
`refactor/operators-under-ast` (#1552) leaves it.
Status: plan; nothing implemented yet.

## Goal

Define the binary conditions the way the binary operators are defined since #1552: one generic node, with
everything that tells one condition from another carried by its tags. Get there in steps that each stand on
their own:

1. Collation members written once instead of once per condition.
2. Collation for IS / IS NOT / IS [NOT] DISTINCT FROM, which SQLite supports but sqlite_orm does not offer.
3. The per-condition structs become aliases over the generic node, like `add_t` and friends.

## Where things stand

- `binary_condition<L, R, S, Res>` derives from `condition_t` and the tag `S`, which carries the SQL spelling
  (`serialize()`); `Res` is the C++ result type, `bool` for all of them.
- Twelve structs derive from it and from `negatable_t`: `and_condition_t`, `or_condition_t`, `is_equal_t`,
  `is_not_equal_t`, `is_t`, `is_not_t`, `is_distinct_from_t`, `is_not_distinct_from_t`, `greater_than_t`,
  `greater_or_equal_t`, `less_than_t`, `less_or_equal_t`.
- Six of them - the equality and ordering comparisons - repeat the same five members: `collate_binary()`,
  `collate_nocase()`, `collate_rtrim()`, `collate(std::string)` and `collate<C>()`, each building a `collate_t`
  or `named_collate` around the condition. That is 30 functions differing only in the node type they return.
- `is_t`, `is_not_t` and the DISTINCT FROM conditions have no collation members at all.
- Consumers match the conditions generically: `is_binary_condition` (an `is_base_template_of` test) in
  `ast_iterator`, `node_tuple`, `column_result_t` and the shared serializer of conditions and operators,
  which also decides on parentheses by it.
- The binary operators, since #1552: `binary_operator<L, R, Ds...>` derives from its tags `Ds...`; each operator
  is an alias such as `using add_t = binary_operator<L, R, add_string, arithmetic_t, negatable_t>`, its tag
  carrying the spelling and the `result_type`; a single operator is recognized through its tag, as in
  `is_conc_v = is_binary_operator_v<T> && std::is_base_of<conc_string, T>::value`.
- `is_equal_with_table_t` looked like a binary condition but is none; it moved to
  `builtin/dbos/fts5_deprecations.h` (#1550, 2026-10-04) and is out of scope here.

## Step 1 - `collatable<Self>`

A CRTP base holding the five members once:

```cpp
template<class Self>
struct collatable {
    collate_t<Self> collate_binary() const {
        return {static_cast<const Self&>(*this), collate_argument::binary};
    }
    // collate_nocase(), collate_rtrim(), collate(std::string), collate<C>()
};

template<class L, class R>
struct is_equal_t : binary_condition<L, R, is_equal_string, bool>, negatable_t, collatable<is_equal_t<L, R>> {
    using binary_condition<L, R, is_equal_string, bool>::binary_condition;
};
```

- Applies to the six comparisons that have the members today; their API stays as it is.
- `collate_t<Self>` is only named in the member declarations, so `Self` need not be complete where the base is
  instantiated.
- Consumers: no observable change.

## Step 2 - collation for IS, IS NOT, IS [NOT] DISTINCT FROM

SQLite applies a collating sequence to every binary comparison operator, IS and IS NOT included
(https://www.sqlite.org/datatype3.html#collation), and IS [NOT] DISTINCT FROM are spellings of IS NOT and IS.
With step 1 in place, offering collation for them is one base class each.

- A feature, not a refactoring: consumers can write `is(&User::name, "a").collate_nocase()`, serialized as
  `"name" IS 'a' COLLATE NOCASE`.
- The DISTINCT FROM conditions exist as of SQLite 3.39.0 only; their tests carry the same version guard.
- Tests: serialization of each, and one execution test showing NOCASE changing the result of IS.
- To be decided on its own - a separate commit or PR, so that it can be left out.

## Step 3 - the conditions as aliases over one node

Shape, following `binary_operator`:

```cpp
template<class L, class R, class... Ds>
struct binary_condition : condition_t, Ds..., collatable_if<binary_condition<L, R, Ds...>, Ds...> { ... };

template<class L, class R>
using is_equal_t = binary_condition<L, R, is_equal_string, negatable_t, collatable_t>;
```

- **Collatability as a tag.** A condition is collatable if one of its tags is a `collatable_t` marker.
  Member functions cannot be selected by a tag in C++17 (no `requires` clauses), so the generic node derives
  from `collatable<binary_condition<L, R, Ds...>>` when the marker is present and from an empty base otherwise
  (`collatable_if`, a `std::conditional_t`).
- **Result type in the tag.** `Res` goes; the tags declare `using result_type = bool;`, as #1552 does for the
  operators.
- **Recognizing one condition.** Where a consumer needs a specific condition, it tests the generic node and the
  tag, like `is_conc_v`. An inventory of such consumers comes first; today they match `is_binary_condition`
  only, so few or none are expected.
- **`is_binary_condition`** becomes an `is_specialization_of` test, as nothing derives from the node any more.
- **Consumers**: the API stays; compiler diagnostics show `binary_condition<L, R, is_equal_string, ...>`
  instead of `is_equal_t<L, R>`.
- Builds on #1552: it should follow its merge, to reuse its conventions rather than duplicate them.

## Sequencing

| Step | Kind | Depends on | PR |
|---|---|---|---|
| 1 | refactoring | #1550 | stacked on #1550 |
| 2 | feature | step 1 | own commit or PR, decided separately |
| 3 | refactoring | step 1, #1552 | after #1552 is merged |

## Open questions

- **One node for conditions and operators?** `binary_condition` and `binary_operator` would then differ only by
  their tags (`condition_t` versus `arithmetic_t`, the result type). Tempting, but conditions and operators are
  told apart by consumers for parentheses and result types; to be weighed when step 3 starts, not before.
- **Collation on other nodes.** `between_t`, `in_t` and the ORDER BY term apply collations as well; ORDER BY has
  its own mechanism (`_collate_argument`). Out of scope here; worth a look once `collatable` exists.
