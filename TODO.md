# To do list

`sqlite_orm` is a wonderful library but there are still features that are not implemented. Here you can find a list of them:

* `FOREIGN KEY` - sync_schema fk comparison
* `NATURAL JOIN`
* self `JOIN`(http://www.sqlitetutorial.net/sqlite-self-join/)
* `order_by` with multiple arguments
* rest of core functions(https://sqlite.org/lang_corefunc.html)
* `CASE`
* `HAVING`
* `INSERT` with specified columns (example: `storage.insert(user, columns(&User::name, &User::date));` or `storage.insert<User>(values(&User::id, 5, &User::createdAt, current_timestamp()));`)
* `EXISTS`
* select rowid
* `ATTACH`
* operators +, -, *, /
* blob incremental I/O https://sqlite.org/c3ref/blob_open.html
* reusing of prepared statements - useful for query optimisation
