# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

* `RETURNING` clause for INSERT/UPDATE/DELETE https://sqlite.org/lang_returning.html
* `FOREIGN KEY` - sync_schema fk comparison and ability of two tables to have fk to each other (`PRAGMA foreign_key_list(%table_name%);` may be useful)
* rest of core functions(https://sqlite.org/lang_corefunc.html)
* `ATTACH`
* blob incremental I/O https://sqlite.org/c3ref/blob_open.html
* query static check for correct order (e.g. `GROUP BY` after `WHERE`)
* `SAVEPOINT` https://www.sqlite.org/lang_savepoint.html
* add `static_assert` in crud `get*` functions in case user passes `where_t` instead of id to make compilation error more clear (example https://github.com/fnc12/sqlite_orm/issues/485)
* named constraints: constraint can have name `CREATE TABLE heroes(id INTEGER CONSTRAINT pk PRIMARY KEY)`
* `UPDATE FROM` support https://sqlite.org/lang_update.html#upfrom
* strict tables https://sqlite.org/stricttables.html
* static assert when UPDATE is called with no PKs
* JSON: `->` and `->>` operators (3.38), JSONB function family (3.45+: `jsonb`, `jsonb_extract`, `jsonb_set`, `jsonb_insert`, `jsonb_replace`, `jsonb_remove`, `jsonb_patch`, `jsonb_object`, `jsonb_array`, `jsonb_group_array`, `jsonb_group_object`), `json_pretty` (3.46), `json_error_position` (3.42), `json_valid` flags argument (3.45), `json_array_insert`/`jsonb_array_insert` (3.53)
* `json_each` and `json_tree` (and `jsonb_each`/`jsonb_tree`, 3.51) table-valued functions for JSON1 extension
* user-defined window functions (`sqlite3_create_window_function` with step/inverse/value/final)
* URI filenames (`SQLITE_OPEN_URI`): `file:...?mode=...&cache=shared`, shared in-memory databases, `immutable=1`
* update hook

Please feel free to add any feature that isn't listed here and not implemented yet.
