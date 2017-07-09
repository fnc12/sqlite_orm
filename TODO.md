# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

* composite key
* `FOREIGN KEY` - sync_schema fk comparison and not enable `PRAGMA foreign_keys` on open if there are no fks in a storage
* `NATURAL JOIN`
* self `JOIN`(http://www.sqlitetutorial.net/sqlite-self-join/)
* `order_by` with multiple arguments
* rest of core functions(https://sqlite.org/lang_corefunc.html)
* `CASE`
* `HAVING`
* `INSERT` with specified columns (example: `storage.insert(user, columns(&User::name, &User::date));`)
* `EXISTS`
* select rowid
* `INDEX`
* `ATTACH`
* operators +, -, *, /
* blob incremental I/O https://sqlite.org/c3ref/blob_open.html
