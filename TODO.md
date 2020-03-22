# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

* `FOREIGN KEY` - sync_schema fk comparison and ability of two tables to have fk to each other
* rest of core functions(https://sqlite.org/lang_corefunc.html)
* `ATTACH`
* blob incremental I/O https://sqlite.org/c3ref/blob_open.html
* explicit FROM for subqueries in FROM argument
* busy handler https://sqlite.org/c3ref/busy_handler.html
* CREATE VIEW and other view operations https://sqlite.org/lang_createview.html
* triggers
* query static check for correct order (e.g. `GROUP BY` after `WHERE`)
* `WINDOW`
* `UPSERT` https://www.sqlite.org/lang_UPSERT.html
* `SAVEPOINT` https://www.sqlite.org/lang_savepoint.html
* add `static_assert` in crud `get*` functions in case user passes `where_t` instead of id to make compilation error more clear (example https://github.com/fnc12/sqlite_orm/issues/485)

Please feel free to add any feature that isn't listed here and not implemented yet.
