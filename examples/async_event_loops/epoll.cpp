/**
 *  This example embeds the asynchronous storage into an event loop the application already has;
 *  here a hand-written epoll loop, to show everything there is to it.
 *
 *  The `async_io_context` of the storage is not a second loop the application has to run. It offers a descriptor,
 *  `native_handle()`, that becomes readable whenever it has something to do, and a function, `poll()`, that
 *  does what can be done without waiting and returns. The host loop watches the descriptor like any socket:
 *
 *      on readable(io.native_handle()):  io.poll();
 *      after io.spawn(...):              io.poll();
 *
 *  The coroutines are resumed from inside `poll()`, hence on the thread of the host loop.
 *  The other files next to this one do the same with libuv, Qt, GLib, libhv and Asio.
 */

#include <cstdio>  //  std::remove
#include <iostream>
#include <sqlite_orm/sqlite_orm.h>
#include <string>

#ifdef SQLITE_ORM_WITH_ASYNC
#include <cerrno>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

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

async_task<void> store_measurements(Storage& storage, bool& finished) {
    co_await storage.sync_schema();
    for (int index = 0; index < 200; ++index) {
        //  every insert is a transaction of its own and waits for the disk
        co_await storage.insert(Measurement{0, index * 0.5});
    }
    const int count = co_await storage.count<Measurement>();
    cout << "stored " << count << " measurements" << endl;
    finished = true;
}

void watch_readable(int loop, int descriptor) {
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = descriptor;
    ::epoll_ctl(loop, EPOLL_CTL_ADD, descriptor, &event);
}
#endif

int main(int, char**) {
#ifdef SQLITE_ORM_WITH_ASYNC
    if (!io_uring_available()) {
        cout << "io_uring is not available in this environment" << endl;
        return 0;
    }
    const char* filename = "async_epoll.sqlite";
    std::remove(filename);

    //  the application's own event loop, with an event source of its own: a timer firing every millisecond
    const int loop = ::epoll_create1(EPOLL_CLOEXEC);
    const int timer = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    const itimerspec everyMillisecond{{0, 1000000}, {0, 1000000}};
    ::timerfd_settime(timer, 0, &everyMillisecond, nullptr);
    watch_readable(loop, timer);

    //  the storage joins the loop with one more descriptor
    async_io_context io;
    auto storage = open_storage(io, filename);
    watch_readable(loop, io.native_handle());

    bool finished = false;
    io.spawn(store_measurements(storage, finished));
    io.poll();

    int timerEvents = 0;
    int storageEvents = 0;
    while (!finished) {
        epoll_event event{};
        const int ready = ::epoll_wait(loop, &event, 1, -1);
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        if (ready != 1) {
            break;
        }
        if (event.data.fd == io.native_handle()) {
            ++storageEvents;
            io.poll();
        } else {
            std::uint64_t expirations = 0;
            if (::read(timer, &expirations, sizeof expirations) > 0) {
                ++timerEvents;
            }
        }
    }
    cout << "the loop served " << storageEvents << " storage events and " << timerEvents
         << " timer events in the meantime" << endl;

    ::close(timer);
    ::close(loop);
#else
    std::cout << "the asynchronous storage is not available with this platform, compiler or C++ standard" << std::endl;
#endif
    return 0;
}
