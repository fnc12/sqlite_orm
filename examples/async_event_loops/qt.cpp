/**
 *  This example embeds the asynchronous storage into the event loop of Qt [https://www.qt.io].
 *
 *  The `async_io_context` of the storage offers a descriptor, `native_handle()`, that is readable whenever it has
 *  something to do, and `poll()`, which does what can be done without waiting and returns. A `QSocketNotifier` watches
 *  the descriptor and calls `poll()`.
 *  The coroutine is resumed from inside `poll()`, hence on the thread of the loop, next to the loop's own
 *  events; here a timer firing every millisecond.
 *
 *  `epoll.cpp` next to this file shows the same with a hand-written loop.
 */

#include <cstdio>  //  std::remove
#include <iostream>
#include <sqlite_orm/sqlite_orm.h>
#include <string>

#ifdef SQLITE_ORM_WITH_ASYNC
#include <QCoreApplication>
#include <QSocketNotifier>
#include <QTimer>
#include <exception>
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
 *  Nothing may leave this coroutine: `poll()` would rethrow it into the event loop of Qt, which does not support exceptions.
 */
async_task<void> store_measurements(Storage& storage, QCoreApplication& application) {
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
    application.quit();
}
#endif

int main(int argc, char** argv) {
#ifdef SQLITE_ORM_WITH_ASYNC
    if (!io_uring_available()) {
        cout << "io_uring is not available in this environment" << endl;
        return 0;
    }
    const char* filename = "async_qt.sqlite";
    std::remove(filename);

    QCoreApplication application(argc, argv);

    //  an event source of the application itself
    int timerEvents = 0;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&timerEvents] {
        ++timerEvents;
    });
    timer.start(1);

    //  the whole integration: Qt calls poll() whenever the async_io_context has something to do.
    //  The notifier is declared after the async_io_context, which owns the descriptor, so it is destroyed before it.
    async_io_context io;
    auto storage = open_storage(io, filename);
    QSocketNotifier watcher(io.native_handle(), QSocketNotifier::Read);
    QObject::connect(&watcher, &QSocketNotifier::activated, [&io] {
        io.poll();
    });

    io.spawn(store_measurements(storage, application));
    io.poll();

    application.exec();  //  until the coroutine quits the application

    cout << "the loop served " << timerEvents << " timer events in the meantime" << endl;
#else
    std::cout << "the asynchronous storage is not available with this platform, compiler or C++ standard" << std::endl;
#endif
    return 0;
}
