/**
 *  Purpose: show the asynchronous storage embedded into the event loop of GLib, the one GTK applications run on.
 *
 *  Intention: uses GLib [https://docs.gtk.org/glib/main-loop.html]; `epoll.cpp` next to this file shows the same with a hand-written loop.
 *
 *  `g_unix_fd_add()` watches the `async_io_context`'s descriptor, `native_handle()`, and calls `poll()` when it is readable.
 *  The coroutine is resumed from inside `poll()`, on the thread of the loop, next to the loop's own events; here a
 *  timer firing every millisecond.
 */

#include <cstdio>  //  std::remove
#include <iostream>
#include <sqlite_orm/sqlite_orm.h>
#include <string>

#ifdef SQLITE_ORM_WITH_ASYNC
#include <exception>
#include <glib-unix.h>
#include <glib.h>
#include <utility>

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
 *  Nothing may leave this coroutine: `poll()` would rethrow it through the frames of GLib, which is written in C.
 */
async_task<void> store_measurements(Storage& storage, GMainLoop* loop) {
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
    ::g_main_loop_quit(loop);
}
#endif

int main(int, char**) {
#ifdef SQLITE_ORM_WITH_ASYNC
    if (!io_uring_available()) {
        cout << "io_uring is not available in this environment" << endl;
        return 0;
    }
    const char* filename = "async_glib.sqlite";
    std::remove(filename);

    GMainLoop* loop = ::g_main_loop_new(nullptr, FALSE);

    //  an event source of the application itself
    int timerEvents = 0;
    const guint timer = ::g_timeout_add(
        1,
        [](gpointer timerEvents) -> gboolean {
            ++*static_cast<int*>(timerEvents);
            return G_SOURCE_CONTINUE;
        },
        &timerEvents);

    //  the whole integration: GLib calls poll() whenever the async_io_context has something to do
    async_io_context io;
    auto storage = open_storage(io, filename);
    const guint watcher = ::g_unix_fd_add(
        io.native_handle(),
        G_IO_IN,
        [](gint /*descriptor*/, GIOCondition /*condition*/, gpointer io) -> gboolean {
            static_cast<async_io_context*>(io)->poll();
            return G_SOURCE_CONTINUE;
        },
        &io);

    io.spawn(store_measurements(storage, loop));
    io.poll();

    ::g_main_loop_run(loop);  //  until the coroutine quits the loop

    //  the watcher is removed before the async_io_context, which owns the descriptor, is destroyed
    ::g_source_remove(watcher);
    ::g_source_remove(timer);
    ::g_main_loop_unref(loop);

    cout << "the loop served " << timerEvents << " timer events in the meantime" << endl;
#else
    std::cout << "the asynchronous storage is not available with this platform, compiler or C++ standard" << std::endl;
#endif
    return 0;
}
