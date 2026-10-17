# Source layout

Where a header under `dev/` belongs, folder by folder. Read this before adding, moving or
renaming a header. Traits and algorithms follow [the vocabulary layer](vocabulary-layer.md),
which takes precedence where both apply.

## Execution

- `dev/execution/` - Running SQL against a live database: what holds or drives the state of a database connection. The connection and its options (`connection_holder.h`, `db_open_mode.h`, `storage_options.h`), its limits (`limit_accessor.h`) and backups (`backup.h`); transactions (`transaction_guard.h`, `savepoint.h`, `transaction_state.h`); PRAGMAs and the values they read and write (`pragma.h`, `journal_mode.h`, `locking_mode.h`, `table_info.h`); prepared statements (`prepared_statement.h`, `get_prepared_statement.h`); the function runtime (`udf_proxy.h`) and the check that the functions and collations a statement uses are registered (`udf_existence_checker.h`); and the storage on top of them: `storage.h` - the main storage template with CRUD operations, `storage_base.h` - its base class with connection management, transactions and UDFs, `sync_schema_result.h`. Converting values and iterating result rows also happen at runtime, but are not execution: they stay in `marshaling/` and `result_set/`.

## Schema definition

- `dev/schema/table.h` - Table definitions
- `dev/schema/column.h` - Column definitions and constraints
- `dev/schema/view.h` - View support
- `dev/schema/virtual_table.h` - Virtual table support
- `dev/schema/index.h` - Index support
- `dev/schema/triggers.h` - Trigger support
- `dev/schema/constraints/` - One header per constraint (`primary_key`, `foreign_key`, `check`, ...)
- `dev/schema/algorithms/` - Algorithms operating across the whole schema, e.g. `sync_order.h`, `table_lookup.h`, `column_lookup.h`
- `dev/builtin/` - The stock instances SQLite provides of the kinds of named objects applications can register more of: the built-in SQL functions in `functions/` (umbrella `functions.h`, see below), the built-in collations (`collations.h`: BINARY, NOCASE, RTRIM), the names of the built-in VFSes (`vfs.h`), and in `dbos/` (umbrella `dbos.h`) the built-in database objects - the schema table (`sqlite_master`/`sqlite_schema`), the eponymous virtual tables (dbstat, generate_series) and the virtual table modules (fts5, rtree). Enums of grammar keywords or PRAGMA values do not belong here; they stay with their nodes or settings.

## Vocabulary layer

The placement rules for traits and algorithms are in [the vocabulary layer](vocabulary-layer.md).

- `dev/vocabulary/traits/` - Open classification traits, specialized at each node's own header
- `dev/vocabulary/algorithms/` - Closed algorithms composed from those traits
- `dev/vocabulary/node_traits.h`, `dev/vocabulary/node_algorithms.h` - Declaration-only umbrellas
- `dev/node_definitions.h`, `dev/node_algorithm_definitions.h` - Completeness manifests
- `dev/member_traits/` - Pointer-to-member mechanics; one tier below the DSL

## Query building

- `dev/ast/binary_condition.h`, `dev/ast/negated_condition.h`, `dev/ast/like.h`, `dev/ast/collate.h` - Conditions of a WHERE clause and elsewhere
- `dev/ast/from.h`, `dev/ast/join.h`, `dev/ast/order_by.h` - FROM with its joins, and ORDER BY
- `dev/builtin/functions/` (umbrella `functions.h`): The built-in SQL functions, one header per family as SQLite documents them - `core.h`, `datetime.h`, `aggregate.h`, `math.h`, `json.h`, `window.h` - definitions, not nodes: their call nodes are in `dev/ast/builtin_function.h`
- `dev/ast/app_function.h` - Application-defined function calls (`func<UDF>`, `"name"_scalar`) and the UDF classification traits
- `dev/ast/` - AST nodes for query, DML and operational constructs (`select_t`, `insert_t`, `where`, `window`, ...)
- `dev/ast/crud/` - One header per CRUD statement kind (`get`, `insert`, `replace`, `update`, `remove`), in both their object and their raw DML spellings, plus the clause nodes only they take (`into`, `set`, `default_values`, `upsert_clause`)

## Serialization

- `dev/serialization/` - The statement serializer and its context, and the helpers it alone uses: streaming utilities (`serializing_util.h`), the quoting of identifiers and literals (`quoting.h`), the ORDER BY serializer, the DEFAULT value extractor, and the collectors walking an expression for what serialization needs (`table_name_collector.h`, `column_expressions_collector.h`, `cte_column_names_collector.h`). The customization points it uses are not its own: they live in `dev/type_mapping/` and `dev/marshaling/`.

## Type mapping

- `dev/type_mapping/` - The type-level mapping of C++ types onto SQLite's, public customization points and their helpers: the declared SQLite type of a C++ type (`type_printer.h`), whether it admits NULL (`type_is_nullable.h`), the smart pointers recognized as nullable holders (`is_std_ptr.h`), and the storage-class tag arithmetic values are dispatched on (`arithmetic_tag.h`). Conversions of values do not belong here, they are marshaling.

## Marshaling

- `dev/marshaling/` - Moving values across the boundary to the SQLite C library, in both directions, building on the type mapping. The public customization points: `statement_binder.h` binds C++ values to statement parameters and sets them as function results, `row_extractor.h` extracts C++ values from result columns, column text and function arguments, `field_printer.h` prints C++ values as text. On top of them, what assembles or disassembles several values at once: the parameters of a statement in sequence (`parameter_binders.h`), tuples, structures and objects from result rows (`struct_extractor.h`, mapped objects column by column with `object_from_column_builder.h`, the C++ value type of a column result in `column_result_proxy.h`), and the arguments of application-defined functions (`arg_values.h` as runtime-typed values, `values_to_tuple.h` as the typed parameter tuple). The pointer-passing interface's values (`pointer_value.h`: `pointer_arg`, `pointer_binding`) belong here too: they exist only to be bound and extracted. The function runtime (`udf_proxy.h`) does not marshal and lives in `execution/`.

## Result sets

- `dev/result_set/` - What a query yields and how it is iterated, consuming the marshaling: the C++ result type of an expression (`column_result.h`: `column_result_of_t`) the column expressions a select yields, from which a CTE derives its columns (`column_expression.h`: `column_expression_of_t`), and the references by which an outer statement refers to them (`column_reference.h`: `get_column_references_of()`), the lazy views over a select (`result_set_view.h`, `result_set_iterator.h`) and over the mapped objects of a table (`mapped_view.h`, `mapped_iterator.h`).

## Utilities

- `dev/sqlite3/` - sqlite_orm's interface to the SQLite C library: the configuration derived from `<sqlite3.h>` (`sqlite3_config.h`), the types republished from it as sqlite_orm's own (`sqlite3_types.h`: `int64`, `uint64`), its result codes as `std::error_code`s (`sqlite3_errors.h`), the deleters of what it allocates (`sqlite3_deleters.h`: `statement_finalizer`), its destructor callback convention and the adaptation of C++ deleters to it (`sqlite3_xdestroy.h`: `xdestroy_fn_t`, `obtain_xdestroy_for()`), and the execution of statements through it (`sqlite3_statements.h`); umbrella `sqlite3_interface.h`. A header belongs here only if both hold: it depends on nothing in sqlite_orm but `functional/` and other `sqlite3/` headers (no DSL, schema, storage or connection), and its content is the C library's vocabulary - handles, calls, result codes, callback conventions - rather than sqlite_orm's own concepts. Merely using the C API does not qualify: `prepared_statement.h` owns a `sqlite3_stmt`, but it is a storage concept carrying a DSL expression.
- `dev/traversal/` - Walking an expression tree: `ast_iterator.h` visits its nodes, `node_tuple.h` flattens it into a tuple of nodes. Traversal, not vocabulary: neither classifies a node
- `dev/cte_db_objects.h` - The database objects of a WITH clause: its CTEs mapped as tables (`schema/cte_mapping.h`), prepended to the schema's database objects
