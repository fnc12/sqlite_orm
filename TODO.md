# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

* `FOREIGN KEY` - sync_schema fk comparison and ability of two tables to have fk to each other
* rest of core functions(https://sqlite.org/lang_corefunc.html)
* `ATTACH`
* blob incremental I/O https://sqlite.org/c3ref/blob_open.html
* reusing of prepared statements - useful for query optimisation
* explicit FROM for subqueries in FROM argument
* backup API https://www.sqlite.org/backup.html
* busy handler https://sqlite.org/c3ref/busy_handler.html
* CREATE VIEW and other view operations https://sqlite.org/lang_createview.html
* triggers
* query static check for correct order (e.g. `GROUP BY` after `WHERE`)
* `WINDOW`
* `UPSERT` https://www.sqlite.org/lang_UPSERT.html
* `CHECK` constraint
* `SAVEPOINT` https://www.sqlite.org/lang_savepoint.html

Please feel free to add any feature that isn't listed here and not implemented yet.
