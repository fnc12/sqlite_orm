# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

## SQL language

* `RETURNING` clause for INSERT/UPDATE/DELETE https://sqlite.org/lang_returning.html
* `RIGHT JOIN` / `FULL OUTER JOIN` (3.39)
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
* `FOREIGN KEY ... DEFERRABLE INITIALLY DEFERRED`
* `INDEXED BY` / `NOT INDEXED` (3.6.4)
* `RAISE` with an arbitrary expression as the error message (3.47) — the `raise_*` factories take only strings
* `VACUUM INTO` (3.27), `ANALYZE` (with an optional index argument), `REINDEX` / `REINDEX EXPRESSIONS` (3.53), `EXPLAIN` / `EXPLAIN QUERY PLAN`
* `ATTACH`

## Functions

* JSON: `->` and `->>` operators (3.38), JSONB function family (3.45+: `jsonb`, `jsonb_extract`, `jsonb_set`, `jsonb_insert`, `jsonb_replace`, `jsonb_remove`, `jsonb_patch`, `jsonb_object`, `jsonb_array`, `jsonb_group_array`, `jsonb_group_object`), `json_pretty` (3.46), `json_error_position` (3.42), `json_valid` flags argument (3.45), `json_array_insert`/`jsonb_array_insert` (3.53)
* `json_each` and `json_tree` (and `jsonb_each`/`jsonb_tree`, 3.51) table-valued functions
* planner-hint functions: `likely()`, `unlikely()`, `likelihood()`
* introspection functions: `sqlite_version()`, `sqlite_source_id()`, `sqlite_compileoption_used()`, `sqlite_compileoption_get()`, `sqlite_offset()`
* `substring()` alias for `substr()` (3.34)
* FTS5 auxiliary functions `bm25()` and `snippet()` (only `highlight()` is wrapped); the `fts5vocab` virtual table

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

Please feel free to add any feature that isn't listed here and not implemented yet.
