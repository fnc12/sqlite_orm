# Thread safety and concurrent database access

There are two layers to consider regarding concurrent database access and thread safety: the SQLite layer and the `sqlite_orm` layer.

## 1. The SQLite layer

Everything depends on how SQLite was compiled.

`sqlite_orm` does not add any layer to SQLite's multi-threading policy or capability, nor does it impose additional restrictions.

At the time of writing, there are no runtime database connection options in `sqlite_orm` that change SQLite's concurrent access modes.

Other SQLite features affect concurrent database access, data visibility, and on-disk integrity; for example: journal mode (WAL), handling the `busy` state, transactions, and syncing to disk.

Schema synchronization or data operations that require coherent, atomic behavior must be performed inside a transaction and often require a busy handler. This is especially important when accessing a database from multiple processes.

Familiarize yourself with SQLite's threading and concurrency documentation for full details:
- [Can multiple applications or multiple instances of the same application access a single database file at the same time?](https://www.sqlite.org/faq.html#q5) — yes; SQLite uses file locks to coordinate concurrent access to the database file itself. Restrictions apply.
- [Is SQLite threadsafe?](https://www.sqlite.org/faq.html#q6) — yes; SQLite uses locks for a database connection.
- [Using SQLite In Multi-Threaded Applications](https://www.sqlite.org/threadsafe.html)

## 2. The `sqlite_orm` layer

A program commonly uses a single `storage` instance created by `sqlite_orm::make_storage()` to represent the database. As of `sqlite_orm` v1.10, a `storage` instance can be safely shared; see the considerations below.

Unless you are using an in-memory database, no connection is established by default when you define the `storage` object. On each database access, a connection is opened and closed automatically. It is, of course, possible to open the database permanently, as described in the "A word about performance" section below.

Background: each `storage` instance maintains an atomic connection counter and a handle to the database (pointer). When the counter increases from 0 to 1 the database is opened; when it drops from 1 to 0 the database is closed. As of `sqlite_orm` v1.10, this process is always synchronized (i.e., atomic). For the slow path that involves opening the database, the process of establishing a connection is performed under a mutually exclusive lock. When a connection already exists, the fast path can be taken, which only involves atomic reference counting.

### Special considerations

As mentioned above, a `storage` instance may be shared across threads to concurrently perform queries or CRUD operations.

However, there are things you should only do from a single-threaded context, preferably at the beginning of your program:

1. Synchronization of the database schema.
2. Actions for which `sqlite_orm` must keep state independent of whether a database connection exists:
   a. Creation or deletion of application-defined scalar, aggregate, and collating functions with `storage.create_scalar_function()`, `storage.delete_scalar_function()`, `storage.create_aggregate_function()`, `storage.delete_aggregate_function()`, `storage.create_quoted_scalar_function()`, `storage.delete_quoted_scalar_function()`, `storage.create_collate_function()`, and `storage.delete_collate_function()`.
   b. Configuration of database limits with `storage.limit.set()`.
   c. Installation of the busy handler with `storage.busy_handler()`.
   d. Installation of an 'on open' handler with `storage.on_open`.
   e. Opening a permanent connection to a database on disk by calling `storage.open_forever()`.

## A word about performance

Opening and setting up a database connection has nontrivial overhead.

If your program accesses the database frequently, especially from multiple threads, open the database "forever" — preferably by passing connection control options when creating the `storage` instance:
```
auto storage = make_storage("", connection_control{true});
```
... or call:
```
storage.open_forever();
```

Advantages:
1. No locking is required in the fast path, which helps the CPU branch predictor.
2. Connection setup (for example, configuring limits or registering application-defined functions) happens only once.
