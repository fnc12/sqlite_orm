# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

* `FOREIGN KEY` - sync_schema fk comparison and ability of two tables to have fk to each other (`PRAGMA foreign_key_list(%table_name%);` may be useful)
* rest of core functions(https://sqlite.org/lang_corefunc.html)
* `ATTACH`
* blob incremental I/O https://sqlite.org/c3ref/blob_open.html
* CREATE VIEW and other view operations https://sqlite.org/lang_createview.html
* triggers
* query static check for correct order (e.g. `GROUP BY` after `WHERE`)
* `WINDOW`
* `UPSERT` https://www.sqlite.org/lang_UPSERT.html
* `SAVEPOINT` https://www.sqlite.org/lang_savepoint.html
* add `static_assert` in crud `get*` functions in case user passes `where_t` instead of id to make compilation error more clear (example https://github.com/fnc12/sqlite_orm/issues/485)
* generated columns https://www.sqlite.org/gencol.html
* named constraints: constraint can have name `CREATE TABLE heroes(id INTEGER CONSTRAINT pk PRIMARY KEY)`
* `FILTER` clause https://sqlite.org/lang_aggfunc.html#aggfilter
* scalar math functions https://sqlite.org/lang_mathfunc.html
* improve DROP COLUMN in `sync_schema` https://sqlite.org/lang_altertable.html#altertabdropcol
* `UPDATE FROM` support https://sqlite.org/lang_update.html#upfrom
* `iif()` function https://sqlite.org/lang_corefunc.html#iif
* add strong typed collate syntax (more info [here](https://github.com/fnc12/sqlite_orm/issues/767#issuecomment-887689672))
* raw `INSERT`: `storage.insert_into<User>(&User::id, values(5));` or `storage.insert_into<User>(columns(&User::id, &User::name), values(5, "Puff Diddy"));` or `storage.insert_into<User>(columns(), default_values());`

Please feel free to add any feature that isn't listed here and not implemented yet.
