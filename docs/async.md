# Asynchronous storage

Where `SQLITE_ORM_ASYNC_SUPPORTED` is defined, `<sqlite_orm/sqlite_orm.h>` also provides a `co_await`-driven front-end over the ordinary `storage`. The synchronous ORM code is not changed: every operation runs on a *fiber* (a stackful coroutine with its own stack) and the SQLite file I/O underneath it goes through a custom VFS that hands reads, writes and fsyncs to io_uring. While one operation waits for the disk, the thread runs other coroutines.

Requirements: Linux with io_uring (kernel 5.6 or newer; raw syscalls, no liburing dependency), C++20 coroutines, GCC 13 or newer or Clang, x86_64 or aarch64. GCC 11 and 12 are excluded: they miscompile temporaries that live across a suspension in a `co_await` expression, which breaks ordinary code such as `co_await storage.insert(User{...})`. On other platforms the asynchronous part compiles to nothing. If the kernel refuses io_uring at runtime (seccomp, the `io_uring_disabled` sysctl), creating an `io_context` throws; `io_uring_available()` tells in advance.

## Usage

```cpp
#include <sqlite_orm/sqlite_orm.h>
using namespace sqlite_orm;

struct User { int id; std::string name; int age; };

io_context io;
auto storage = make_async_storage(io, "app.db",
    make_table("users",
        make_column("id", &User::id, primary_key().autoincrement()),
        make_column("name", &User::name),
        make_column("age", &User::age)));

io.spawn([&storage]() -> task<void> {
    co_await storage.sync_schema();
    co_await storage.insert(User{0, "ann", 30});
    auto adults = co_await storage.get_all<User>(where(c(&User::age) >= 18));
    co_await storage.transaction([](auto& storage) {
        storage.insert(User{0, "bob", 41});
        return true;   // false rolls back
    });
    auto statement = co_await storage.prepare(select(columns(&User::name), where(c(&User::id) == 1)));
    auto names = co_await storage.execute(statement);
}());
io.run();
```

`make_async_storage` takes the same arguments as `make_storage`, with the `io_context` first. It injects the asynchronous VFS through `connection_control`; nothing global is registered. A `connection_control` passed by the caller keeps its other fields.

`async_storage` mirrors the public interface of `storage_t`: CRUD, `get_all`, `select`, `with`, aggregates, `prepare` and `execute`, schema functions, `transaction` and `savepoint` (the callable receives the storage and returns `true` to commit), connection information and backups. Each returns a `task<T>`; arguments are moved into the operation and exceptions surface at the `co_await`. Functions that do no I/O (`filename`, `vfs_name`, `open_mode`, `dump`, `interrupt`, `is_opened`) are plain calls. Anything else, for example pragmas, limits, user-defined functions or `iterate`, goes through `storage.run(function)`, the primitive everything is built on: `function(storage)` runs on a fiber with exclusive access to the storage.

## One connection

An `async_storage` is one connection, opened lazily on first use and kept open. Like a network connection in an asynchronous client, it lets the thread do other work while a request is in progress, but requests on it are executed one after another: operations issued from several coroutines are queued in order. Prepared statements therefore always run on the connection they were prepared on. For I/O parallelism, use several storages.

## Threads and other event loops

Everything created from one `io_context` runs on the thread that calls `io.run()`, the same way an asio `io_context`, a tokio runtime or an asyncio loop needs a thread. To combine it with another runtime that owns its threads (a ROS 2 executor, a GUI loop, a server framework):

- Give the `io_context` a thread of its own with `run_forever()` and `stop()`. From any other thread, `post(function)` hands work to it; a coroutine started there delivers its result back through whatever the host offers, for example a thread-safe publisher or a callback posted to the host loop. `io_pool` packages this: `pool.run_blocking(function)` for plain code, `co_await pool.async(function)` from a coroutine running on any `io_context`.
- Or embed it: `native_handle()` is a descriptor that becomes readable when completions are waiting; watch it in the host loop (epoll, a guard condition, a socket notifier) and call `poll()` when it fires. `poll()` never blocks.

## Limits

Row-by-row `iterate<T>()` is not exposed as an asynchronous generator; iterate inside `run`. `xOpen`, `xFileSize` and `xTruncate` are synchronous (they are rare). Writes are not batched: every write is awaited in order, which keeps the file state exactly what SQLite believes it is at all times. Memory-mapped I/O is disabled for these connections because a page fault would be a hidden synchronous read.
