# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

## SQL language

* `RETURNING` clause for INSERT/UPDATE/DELETE https://sqlite.org/lang_returning.html
* `IS DISTINCT FROM` / `IS NOT DISTINCT FROM` binary operators (3.39), and the bare `IS` / `IS NOT` spellings
* `NULLS FIRST` / `NULLS LAST` in ORDER BY (3.30)
* ORDER BY inside aggregate function calls (3.44), e.g. `string_agg(x, ',' ORDER BY y)`
* HAVING on aggregate queries without GROUP BY (3.39) — currently only reachable via `group_by(...).having(...)`
* UPSERT generalizations of 3.35: multiple ON CONFLICT clauses, final clause without a conflict target
* row values https://sqlite.org/rowvalue.html — `(a, b) = (c, d)`, `(a, b) IN (SELECT ...)` (tuple operands exist, serialization unverified)
* `UPDATE FROM` support https://sqlite.org/lang_update.html#upfrom
* strict tables https://sqlite.org/stricttables.html
* TEMP schema objects: TEMP tables, triggers and views
* named constraints: constraint can have name `CREATE TABLE heroes(id INTEGER CONSTRAINT pk PRIMARY KEY)`
* `INDEXED BY` / `NOT INDEXED` (3.6.4)
* `RAISE` with an arbitrary expression as the error message (3.47) — the `raise_*` factories take only strings
* `VACUUM INTO` (3.27), `ANALYZE` (with an optional index argument), `REINDEX` / `REINDEX EXPRESSIONS` (3.53), `EXPLAIN` / `EXPLAIN QUERY PLAN`
* `ATTACH`

## Functions

* JSON: `->` and `->>` operators (3.38), JSONB function family (3.45+: `jsonb`, `jsonb_extract`, `jsonb_set`, `jsonb_insert`, `jsonb_replace`, `jsonb_remove`, `jsonb_patch`, `jsonb_object`, `jsonb_array`, `jsonb_group_array`, `jsonb_group_object`, `jsonb_array_insert` 3.53)
* `json_each` and `json_tree` (and `jsonb_each`/`jsonb_tree`, 3.51) table-valued functions
* the `carray()` table-valued function — its definition goes to `dev/builtin/dbos/carray.h`, and the `"carray"` pointer type and its binding helpers in `dev/carray.h` move along with it, as the table-valued function is what consumes them
* the PRAGMA table-valued functions `pragma_table_info()` / `pragma_table_xinfo()` — their definitions go to `dev/builtin/dbos/`, and the row types `table_info` / `table_xinfo` in `dev/table_info.h` move along with them
* planner-hint functions: `likely()`, `unlikely()`, `likelihood()`
* introspection functions: `sqlite_version()`, `sqlite_source_id()`, `sqlite_compileoption_used()`, `sqlite_compileoption_get()`, `sqlite_offset()`
* `substring()` alias for `substr()` (3.34)
* the `fts5vocab` virtual table

## Schema synchronisation

* `FOREIGN KEY` - sync_schema fk comparison and ability of two tables to have fk to each other (`PRAGMA foreign_key_list(%table_name%);` may be useful)
* use ALTER TABLE add/drop NOT NULL and CHECK constraints (3.53) to alter constraints in place instead of recreating the table

## C API / runtime

* blob incremental I/O https://sqlite.org/c3ref/blob_open.html
* user-defined window functions (`sqlite3_create_window_function` with step/inverse/value/final)
* function flags for user-defined functions: `SQLITE_DETERMINISTIC`, `SQLITE_DIRECTONLY` (3.30), `SQLITE_INNOCUOUS` (3.31)
* hooks: update hook, commit hook, rollback hook, preupdate hook, `sqlite3_trace_v2`, authorizer
* `sqlite3_serialize` / `sqlite3_deserialize` (enabled by default since 3.36) — a natural fit next to the backup API
* URI filenames (`SQLITE_OPEN_URI`): `file:...?mode=...&cache=shared`, shared in-memory databases, `immutable=1`
* small C API wrappers: `sqlite3_changes64`/`sqlite3_total_changes64` (3.37), `sqlite3_txn_state` (3.34), `sqlite3_error_offset` (3.38), `sqlite3_is_interrupted` (3.41) and `interrupt()`, `sqlite3_db_name` (3.39), `sqlite3_db_readonly` (3.7.11), `sqlite3_stmt_readonly`/`sqlite3_stmt_busy`/`sqlite3_stmt_isexplain`, `sqlite3_setlk_timeout` (3.50)
* PRAGMA wrappers: `foreign_key_check`, `foreign_key_list`, `table_list` (3.37), `index_list`/`index_info`/`index_xinfo`, `database_list`, `optimize` (3.18), `wal_checkpoint` (incl. `NOOP`, 3.51), `data_version`, `freelist_count`, `page_count`/`page_size`, `cache_size`, `mmap_size`, `temp_store`, `secure_delete`, `defer_foreign_keys`, `query_only`, `incremental_vacuum`, `analysis_limit` (3.32), `trusted_schema` (3.31), `hard_heap_limit` (3.31), `threads`, `case_sensitive_like`, `reverse_unordered_selects`
* session extension (changesets/patchsets, `sqlite3_changegroup`) — large
* encryption support via the [SQLite Encryption Extension (SEE)](https://sqlite.org/com/see.html) (`sqlite3_key`/`sqlite3_rekey`), incl. compatible implementations like SQLCipher (`PRAGMA key`) — see #1445

## Internals

* `optional_container` (once #1550 is merged): it is the compile-time optional sub-expression slot of the `case_t`, `limit_t`, `like_t` and trigger nodes, not a generic helper — `node_tuple` pattern-matches it, `ast_iterator` and `statement_serializer` reach into it via `apply()`, and it leaks into constructor signatures. `std::optional` is no replacement, as presence must stay part of the node's type. Candidate: store the sub-expression directly with an empty `absent_t`-like tag for "not present", asked about by a trait (`if constexpr`), `node_tuple<absent_t>` being empty.
* `dynamic_set_t`: an AST node whose type carries the serializer context (`dynamic_set(storage)`), and which serializes each assignment eagerly as it is pushed back - its assignments are of different types, so they cannot be kept as a tuple. Keeping them instead, type-erased, would let a dynamic SET be serialized and bound like any other node, and drop the context from its type.

Please feel free to add any feature that isn't listed here and not implemented yet.
