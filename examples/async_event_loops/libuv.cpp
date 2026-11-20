/**
 *  Purpose: show the asynchronous storage embedded into the event loop of libuv.
 *
 *  Intention: uses libuv [https://libuv.org]; `epoll.cpp` next to this file shows the same with a hand-written loop.
 *
 *  A `uv_poll_t` watches the `async_io_context`'s descriptor, `native_handle()`, and calls `poll()` when it is readable.
 *  The coroutine is resumed from inside `poll()`, on the thread of the loop, next to the loop's own events; here a
 *  timer firing every millisecond.
 */

#include <cstdio>  //  std::remove
#include <iostream>
#include <sqlite_orm/sqlite_orm.h>
#include <string>

#ifdef SQLITE_ORM_WITH_ASYNC
#include <exception>
#include <utility>
#include <uv.h>

using namespace sqlite_orm;
using std::cout;
using std::endl;

struct Measurement {
    int id = 0;
    double value = 0;
};

auto open_storage(async_io_context& io, std::string filename) {
    return make_async_storage(io,
                              std::move(filename),
                              make_table("measurements",
                                         make_column("id", &Measurement::id, primary_key().autoincrement()),
                                         make_column("value", &Measurement::value)));
}

using Storage = decltype(open_storage(std::declval<async_io_context&>(), ""));

/**
 *  Nothing may leave this coroutine: `poll()` would rethrow it through the frames of libuv, which is written in C.
 */
async_task<void> store_measurements(Storage& storage, uv_loop_t* loop) {
    try {
        co_await storage.sync_schema();
        for (int index = 0; index < 200; ++index) {
            //  every insert is a transaction of its own and waits for the disk
            co_await storage.insert(Measurement{0, index * 0.5});
        }
        const int count = co_await storage.count<Measurement>();
        cout << "stored " << count << " measurements" << endl;
    } catch (const std::exception& error) {
        cout << "failed: " << error.what() << endl;
    }
    ::uv_stop(loop);
}
#endif

int main(int, char**) {
#ifdef SQLITE_ORM_WITH_ASYNC
    if (!io_uring_available()) {
        cout << "io_uring is not available in this environment" << endl;
        return 0;
    }
    const char* filename = "async_libuv.sqlite";
    std::remove(filename);

    uv_loop_t* loop = ::uv_default_loop();

    //  an event source of the application itself
    int timerEvents = 0;
    uv_timer_t timer{};
    ::uv_timer_init(loop, &timer);
    timer.data = &timerEvents;
    ::uv_timer_start(
        &timer,
        [](uv_timer_t* timer) {
            ++*static_cast<int*>(timer->data);
        },
        1,
        1);

    //  the whole integration: libuv calls poll() whenever the async_io_context has something to do
    async_io_context io;
    auto storage = open_storage(io, filename);
    uv_poll_t watcher{};
    ::uv_poll_init(loop, &watcher, io.native_handle());
    watcher.data = &io;
    ::uv_poll_start(&watcher, UV_READABLE, [](uv_poll_t* watcher, int /*status*/, int /*events*/) {
        static_cast<async_io_context*>(watcher->data)->poll();
    });

    io.spawn(store_measurements(storage, loop));
    io.poll();

    ::uv_run(loop, UV_RUN_DEFAULT);  //  until the coroutine stops the loop

    //  the watcher is closed before the async_io_context, which owns the descriptor, is destroyed
    ::uv_close(reinterpret_cast<uv_handle_t*>(&watcher), nullptr);
    ::uv_close(reinterpret_cast<uv_handle_t*>(&timer), nullptr);
    ::uv_run(loop, UV_RUN_DEFAULT);
    ::uv_loop_close(loop);

    cout << "the loop served " << timerEvents << " timer events in the meantime" << endl;
#else
    std::cout << "the asynchronous storage is not available with this platform, compiler or C++ standard" << std::endl;
#endif
    return 0;
}
