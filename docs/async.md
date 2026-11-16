# Asynchronous storage

> Purpose: tell users of sqlite_orm how to use a storage from C++20 coroutines without blocking a thread on file I/O.
>
> Intention: the user documentation of `dev/async/`: what it requires, its interface, how its `io_context` is run or
> embedded into the event loop an application already has, and its limits. The examples it refers to are
> `examples/async_storage.cpp` and `examples/async_event_loops/`.

Where `SQLITE_ORM_WITH_ASYNC` is defined, `<sqlite_orm/sqlite_orm.h>` also provides a `co_await`-driven front-end over the ordinary `storage`. The synchronous ORM code is not changed: every operation runs on a *fiber* (a stackful coroutine, i.e. a function with a stack of its own that can be suspended in the middle) and the SQLite file I/O underneath it goes through a VFS that hands reads, writes and fsyncs to io_uring. While one operation waits for the disk, the thread runs other coroutines.

Requirements: Linux with io_uring (kernel 5.6 or newer; raw system calls, no liburing dependency), C++20 coroutines, GCC 13 or newer or Clang, x86_64 or aarch64. GCC 11 and 12 are excluded: they miscompile temporaries that live across a suspension in a `co_await` expression, which breaks ordinary code such as `co_await storage.insert(User{...})`. On other platforms the asynchronous part compiles to nothing. If the kernel refuses io_uring at runtime (seccomp, the `io_uring_disabled` sysctl), creating an `io_context` throws; `io_uring_available()` tells in advance.

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

`make_async_storage` takes the same arguments as `make_storage`, with the `io_context` first. It injects the asynchronous VFS through `connection_control`; nothing global is changed. A `connection_control` passed by the caller keeps its other fields.

`async_storage` mirrors the public interface of `storage_t`: CRUD, `get_all`, `select`, `with`, aggregates, `prepare` and `execute`, schema functions, `transaction` and `savepoint` (the callable receives the storage and returns `true` to commit), connection information and backups. Each returns a `task<T>`; arguments are moved into the operation and exceptions surface at the `co_await`. Functions that do no I/O (`filename`, `vfs_name`, `open_mode`, `dump`, `interrupt`, `is_opened`) are plain calls. Anything else, for example pragmas, limits or user-defined functions, goes through `storage.run(function)`, the primitive everything is built on: `function(storage)` runs on a fiber with exclusive access to the storage.

A `task<T>` is an ordinary awaitable: any coroutine type that accepts foreign awaitables can `co_await` it. It is lazy, the operation starts when it is awaited.

## Row by row

`iterate` returns a cursor instead of a range, since a range-based `for` cannot await:

```cpp
auto users = storage.iterate<User>(where(c(&User::age) >= 18), order_by(&User::id));
while (std::optional<User> user = co_await users.next()) {
    // one row in memory at a time
}

auto rows = storage.iterate(select(columns(&User::id, &User::name)));
while (auto row = co_await rows.next()) {
    // std::tuple<int, std::string>
}
```

The first `next()` prepares the statement; it is finalized when the rows are exhausted, when an error ends the iteration, or when the cursor is destroyed. Between two rows the connection is free: other operations on the same storage run in between, further cursors included, as they may inside a loop over the synchronous `storage.iterate()`. A cursor must not outlive its storage.

Every `next()` is an operation of its own, which costs a fraction of a microsecond of bookkeeping per row on top of the step itself. Where neither memory nor latency matters, `get_all` is faster; to process many rows in one go without materializing them, iterate synchronously inside `storage.run()`.

## One connection

An `async_storage` is one connection, opened lazily on first use and kept open. Like a network connection in an asynchronous client, it lets the thread do other work while a request is in progress, but requests on it are executed one after another: operations issued from several coroutines are queued in order. Prepared statements therefore always run on the connection they were prepared on. For I/O parallelism, use several storages. Storages of several threads or processes on one database file contend for its locks like any SQLite connections; with `busy_timeout` set, waiting for a lock does not block the thread either.

## The io_context, and the event loop you already have

C++ has no standard event loop, so something has to submit the storage's I/O to the kernel, collect the completions and resume whoever waits for them. That is all an `io_context` does. It is not a general-purpose executor with timers and sockets, and it does not ask an application that already runs an event loop to run a second one: like `curl_multi` or an event loop of libhv, it can be driven from outside. There are three ways to drive it.

**1. `run()`**, when the application has no event loop of its own. It returns when everything spawned has finished. See `examples/async_storage.cpp`.

**2. Embedded into another event loop**, on that loop's thread. The `io_context` offers two things: `native_handle()`, a descriptor that is readable whenever the `io_context` has something to do, and `poll()`, which does everything that can be done without waiting and returns. The host loop watches the descriptor like a socket and calls `poll()` when it is readable; whoever hands over new work with `spawn()` calls `poll()` afterwards. Coroutines are resumed from inside `poll()`, hence on the thread of the host loop.

```cpp
// libhv
hio_t* watcher = hio_get(loop, io.native_handle());
hio_set_context(watcher, &io);
hio_add(watcher, [](hio_t* watcher) { static_cast<io_context*>(hio_context(watcher))->poll(); }, HV_READ);

// Asio
asio::posix::stream_descriptor descriptor(host, io.native_handle());
descriptor.async_wait(asio::posix::stream_descriptor::wait_read, [&](auto error) { io.poll(); /* and wait again */ });

// libuv
uv_poll_init(loop, &watcher, io.native_handle());
watcher.data = &io;
uv_poll_start(&watcher, UV_READABLE, [](uv_poll_t* watcher, int, int) { static_cast<io_context*>(watcher->data)->poll(); });

// Qt
QSocketNotifier watcher(io.native_handle(), QSocketNotifier::Read);
QObject::connect(&watcher, &QSocketNotifier::activated, [&io] { io.poll(); });

// GLib
g_unix_fd_add(io.native_handle(), G_IO_IN, [](gint, GIOCondition, gpointer io) -> gboolean {
    static_cast<io_context*>(io)->poll();
    return G_SOURCE_CONTINUE;
}, &io);

// epoll
epoll_event event{.events = EPOLLIN, .data = {.fd = io.native_handle()}};
epoll_ctl(loop, EPOLL_CTL_ADD, io.native_handle(), &event);  // and io.poll() when epoll_wait() reports it
```

The descriptor is level-triggered and for watching only: never read from it, and do not let the host close it (with Asio, `release()` the `stream_descriptor` before it is destroyed). `examples/async_event_loops/` has a complete program for each of them: `epoll.cpp` with a hand-written loop, `libuv.cpp`, `qt.cpp`, `glib.cpp`, `libhv.cpp` with an HTTP server on libhv's event loops, and `asio.cpp`, which also shows how a coroutine of Asio (`asio::awaitable`) awaits a storage operation.

Two things hold for every host loop. Stop watching the descriptor before the `io_context` is destroyed. And an exception that `poll()` rethrows must not travel through the host: libuv, GLib and libhv are written in C, and Qt does not support exceptions in its event loop, so catch inside the coroutine, as the examples do, or around `poll()`.

An exception that escapes a coroutine passed to `spawn()` is rethrown by `run()` or `poll()`.

**3. A thread of its own**, for a host that cannot watch a descriptor. `run_forever()` serves the `io_context` on that thread until `stop()`; from any other thread, `post(function)` hands work to it, and a coroutine started there delivers its result back through whatever the host offers, for example a callback posted to the host loop. `io_pool` packages this for several threads: `pool.run_blocking(function)` for plain code, `co_await pool.async(function)` from a coroutine running on any `io_context`.

## Limits

`xOpen`, `xFileSize` and `xTruncate` are synchronous (they are rare). Writes are not batched: every write is awaited in order, which keeps the file state exactly what SQLite believes it is at all times. Memory-mapped I/O is disabled for these connections because a page fault would be a hidden synchronous read. The time SQLite spends computing is not interruptible: `poll()` and `run()` return to other work when an operation waits for the disk, not while it runs on cached pages.
