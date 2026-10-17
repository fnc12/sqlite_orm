# Source layout

Where a header under `dev/` belongs, folder by folder: what belongs there, where its boundary
to the neighbouring folders runs, and a few examples. Read this before adding, moving or
renaming a header. Traits and algorithms follow [the vocabulary layer](vocabulary-layer.md),
which takes precedence where both apply.

## Schema and DSL

- `dev/schema/` - The definitions of database objects: tables, columns, views, virtual tables,
  indexes, triggers (`table.h`, `column.h`, `virtual_table.h`, ...); one header per constraint
  in `constraints/`; algorithms across the whole schema in `algorithms/` (`sync_order.h`,
  `table_lookup.h`).
- `dev/ast/` - The nodes of queries, DML and other statements and their clauses and
  expressions (`select_t`, `where`, `binary_condition.h`, `join.h`, ...), including the call
  nodes of functions (`builtin_function.h`, `app_function.h` for application-defined ones);
  one header per CRUD statement kind in `crud/`, in both its object and raw DML spellings, with
  the clause nodes only it takes (`into`, `set`, `upsert_clause`).
- `dev/builtin/` - The stock instances SQLite provides of the kinds of named objects
  applications can register more of: the built-in SQL functions in `functions/`, one header
  per family as SQLite documents them (definitions, not nodes), the collations
  (`collations.h`), the VFS names (`vfs.h`), and in `dbos/` the database objects - the schema
  table, the eponymous virtual tables, their bundles (`make_builtin_tables()`,
  `make_pragma_tables()`) and the virtual table modules (fts5, rtree). Enums of grammar
  keywords or PRAGMA values do not belong here; they stay with their nodes or settings.
- `dev/cte_db_objects.h` - The database objects of a WITH clause: its CTEs mapped as tables,
  prepended to the schema's.

## Vocabulary and traversal

- `dev/vocabulary/` - Traits, projections and algorithms classifying a single node; its
  manifests `dev/node_definitions.h` and `dev/node_algorithm_definitions.h` sit next to it.
- `dev/member_traits/` - Pointer-to-member mechanics, one tier below the DSL.
- `dev/traversal/` - Walking an expression tree: `ast_iterator.h` visits its nodes,
  `node_tuple.h` flattens it into a tuple. Traversal, not vocabulary: neither classifies a node.

## From expressions to SQL and values

- `dev/serialization/` - The statement serializer, its context, and the helpers only it uses:
  quoting (`quoting.h`), streaming utilities, and the collectors walking an expression for what
  serialization needs (`table_name_collector.h`). The customization points it calls are not
  its own; they are type mapping and marshaling.
- `dev/type_mapping/` - The type-level mapping of C++ types onto SQLite's, as public
  customization points: the declared SQLite type (`type_printer.h`), nullability
  (`type_is_nullable.h`, `is_std_ptr.h`). Converting values is not type mapping but marshaling.
- `dev/marshaling/` - Moving values across the boundary to the C library, in both directions:
  the public customization points binding, extracting and printing values (`statement_binder.h`,
  `row_extractor.h`, `field_printer.h`), what assembles or disassembles several values at once
  (`parameter_binders.h`, `struct_extractor.h`, `values_to_tuple.h`), and the pointer-passing
  interface's values (`pointer_value.h`), which exist only to be bound and extracted.
- `dev/result_set/` - What a query yields and how it is iterated: the C++ result type of an
  expression (`column_result.h`), the column expressions of a select (`column_expression.h`)
  and the references to them (`column_reference.h`), and the lazy views over a select or the
  objects of a table (`result_set_view.h`, `mapped_view.h`).

## Execution

- `dev/execution/` - Running SQL against a live database, i.e. what holds or drives the state
  of a connection: the connection and its options (`connection_holder.h`), transactions
  (`transaction_guard.h`), PRAGMAs and their values (`pragma.h`), prepared statements
  (`prepared_statement.h`), the function runtime (`udf_proxy.h`), and the storage on top
  (`storage.h`, `storage_base.h`). Converting values and iterating result rows also happen at
  runtime, but are marshaling and result sets.

## Foundations

- `dev/sqlite3/` - sqlite_orm's interface to the SQLite C library, under the umbrella
  `dev/sqlite3_interface.h`: the configuration derived from `<sqlite3.h>`, the types
  republished from it (`sqlite3_types.h`), its result codes as `std::error_code`s, deleters,
  the destructor callback convention (`sqlite3_xdestroy.h`) and statement execution. A header
  belongs here only if both hold: it depends on nothing in sqlite_orm but `functional/` and
  other `sqlite3/` headers, and its content is the C library's vocabulary - handles, calls,
  result codes, callback conventions - rather than sqlite_orm's own concepts. Merely using the
  C API does not qualify: `prepared_statement.h` owns a `sqlite3_stmt`, but is a storage concept
  carrying a DSL expression.
- `dev/functional/` - The configuration (`config.h`, the C++ feature and compiler macros) and
  general-purpose C++ utilities with no SQL or DSL meaning: polyfills, metaprogramming
  (`mpl.h`), reflection helpers (`meta_util.h`).
- `dev/tuple_helper/` - Generic algorithms over tuples and type lists.
