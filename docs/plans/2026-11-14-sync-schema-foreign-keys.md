# `sync_schema` and foreign keys

**Purpose:** let `sync_schema` bring a table's foreign keys in line with the storage, as it does
its columns — the open part of [#790](https://github.com/fnc12/sqlite_orm/issues/790).

**Intention:** a plan for the issue's work, read before its sub-issues are drafted. It changes
`schema_status` and the table rebuild in `dev/execution/`, reads the database through `PRAGMA
foreign_key_list` and `PRAGMA foreign_key_check`, and closes the `FOREIGN KEY` entry of
[TODO.md](../../TODO.md). The decisions below are open until the maintainers choose; the
hand-over at the end carries what the next session needs to start.

Written against `dev` at `8d304ddf` (merge of #1611); what it says of the code holds for that
commit.

## State of #790

- Part 2 of the issue (`INSERT INTO 'C' DEFAULT VALUES`) is settled: `replace` writes the primary
  key, and the raw insert asked for exists as `insert(into<T>(), columns(...), values(...))`.
- Part 1 is open: `schema_status` for a base table decides from `PRAGMA table_xinfo` alone, so a
  table that differs only in its foreign keys reports `already_in_sync`.

## What a probe showed

SQLite 3.50.4, an in-memory database:

- `PRAGMA foreign_key_list(C)` yields `id, seq, table, from, to, on_update, on_delete, match`. An
  action left unspecified reads `NO ACTION`; a composite key is one `id` with several `seq`; the
  ids run in reverse declaration order. `DEFERRABLE INITIALLY DEFERRED` appears nowhere in it.
- The storage switches `PRAGMA foreign_keys` on whenever it declares a foreign key. With it on:
  - copying rows that violate a new key fails with `FOREIGN KEY constraint failed`; the rebuild
    runs outside a transaction, so the half-built `<table>_backup` stays behind;
  - dropping a table that another table references fails the same way;
  - dropping a parent whose child references it `ON DELETE CASCADE` empties the child, silently.

The last one is a bug of today, independent of this feature: every rebuild of a referenced
table, with `preserve` or without, wipes its cascading children.

## The change

1. **Read the database side** — `pragma.foreign_key_list(table)`, built like
   `pragma.table_xinfo`, grouped by `id` into one record per key: target table, source
   columns, target columns, ON UPDATE, ON DELETE.
2. **Build the same records from the storage** — walk the table's `foreign_key_t` constraints;
   the target table name comes from `lookup_table_name`, as the foreign key's serializer takes
   it; `foreign_key_action::none` maps to `NO ACTION`. A table without foreign keys skips the
   query at compile time.
3. **Compare** them as sets, order ignored. A difference sets `gottaCreateTable` in
   `schema_status`; from there today's path runs (`backup_table` under `preserve`,
   `drop_create_with_loss` otherwise). `sync_schema_simulate` reports it as well, since it calls
   the same `schema_status`.
4. **Make the rebuild safe**, after SQLite's documented procedure for altering a table:
   `foreign_keys` off, which takes effect only outside a transaction → SAVEPOINT → create, copy,
   drop, rename → `PRAGMA foreign_key_check(table)` → on a violation roll back and throw,
   otherwise release → restore `foreign_keys` to what it was.
5. **Documentation** — the warning in `sync_schema`'s doc-comment, which says foreign keys
   cannot be read, gives way to what is compared; the `FOREIGN KEY` entry of TODO.md keeps only
   its second half, two tables referencing each other.

## Decisions (open)

1. **Existing rows violate the new key:** (a) roll back and throw, table as it was — *lean*;
   (b) rebuild and keep the violating rows; (c) delete them, report data loss.
2. **DEFERRABLE**, absent from the pragma: (a) left out of the comparison, said in the
   doc-comment — *lean*; (b) parsed from the `CREATE TABLE` text in `sqlite_master`.
3. **Name comparison:** (a) case-insensitive, as SQLite's identifiers are, and a missing
   target-column list (a database written elsewhere: `REFERENCES A`) resolved to the target's
   primary key — *lean*; (b) exact, which rebuilds such tables needlessly.
4. **Where the reader lives:** (a) a private `pragma.foreign_key_list`, as `table_xinfo` is —
   *lean*; (b) a public eponymous `pragma_foreign_key_list` virtual table beside
   `pragma_table_info`.
5. **Result value:** (a) the existing `dropped_and_recreated` / `_with_data_loss` — *lean*;
   (b) a new enumerator, which breaks every exhaustive user `switch`.
6. **Split:** (a) two PRs — the safe rebuild (step 4) first, since it fixes the cascade bug on
   its own, then the comparison (steps 1–3, 5) — *lean*; (b) one PR.

## Hand-over

Where to start, by name; the code is in `dev/execution/` unless said otherwise.

- **Status decision** — `storage_t::schema_status` for `is_base_table` in `storage.h`. The new
  check goes after `calculate_remove_add_columns` and before the branch on `gottaCreateTable`, so
  the existing `canPreserveData` scan over the added columns stays in force. A table whose
  columns are already in sync but whose keys differ reaches that branch with `columnsToAdd`
  empty, which yields `dropped_and_recreated`.
- **Acting on it** — `storage_t::sync_regular_base_table` in `storage_definitions.h` already
  turns `dropped_and_recreated` into `backup_table` or `drop_create_with_loss`; step 3 needs no
  change there.
- **Rebuild** — `backup_table` (create `<name>_backup`, `copy_table`, drop, rename) and
  `drop_create_with_loss` in `storage.h`; both drop through `drop_dbo_internal`. Step 4 wraps
  both: the drop of a referenced table is the hazard, the copy is a second one. Rebuilds also
  run where `ALTER TABLE ... DROP COLUMN` fails and falls back to `backup_table`.
- **Foreign-key switch** — `storage_base::foreign_keys(db)` / `foreign_keys(db, bool)` and
  `on_open_internal` in `storage_base.h`, which turns the switch on when
  `cachedForeignKeysCount` is non-zero.
- **Pragma readers** — `pragma.h`: `table_xinfo` shows the shape for `foreign_key_list` and
  `foreign_key_check`. Option 4(b) follows the skill `add-virtual-table` and
  `dev/builtin/dbos/pragma_table_info.h`.
- **Storage side** — `foreign_key_t` in `dev/schema/constraints/foreign_key.h` holds `_columns`,
  `_references`, `on_update`, `on_delete`, `_deferrable`; its serializer in
  `dev/serialization/statement_serializer.h` shows how the target table name and the column
  names are resolved, and that sqlite_orm always writes the target columns. sqlite_orm writes
  foreign keys as table constraints only.
- **Tests** — `tests/sync_schema_tests.cpp` for the comparison; the rebuild needs a case each
  for a cascading child surviving the rebuild of its parent, a violating row rolling back, and
  the `_backup` table absent afterwards. `tests/constraints/foreign_key.cpp` holds the
  foreign-key fixtures.
- **Probe** — a short Python `sqlite3` script against `:memory:` reproduces every finding above
  in seconds; it is the quickest check of a claim about the pragmas before writing C++.
