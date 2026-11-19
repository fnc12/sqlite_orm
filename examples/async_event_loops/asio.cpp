/**
 *  This example uses the asynchronous storage from coroutines of Asio (Boost.Asio or the standalone library),
 *  on Asio's own event loop.
 *
 *  Two things are shown:
 *
 *  1. `storage_driver` embeds the storage's `async_io_context` into `asio::io_context`: Asio watches its descriptor and
 *     calls `poll()` whenever it is readable. Asio's loop stays the only one that runs.
 *  2. `await_task()` lets an `asio::awaitable` coroutine await a storage operation. Asio's coroutines only await
 *     Asio's own asynchronous operations, so the function presents an `async_task` as one; with it comes everything a
 *     completion token offers, e.g. `use_awaitable`, `use_future` or a plain callback.
 */

#include <cstdio>  //  std::remove
#include <iostream>
#include <sqlite_orm/sqlite_orm.h>

#if defined(SQLITE_ORM_WITH_ASYNC) && __has_include(<boost/asio.hpp>)
#include <boost/asio.hpp>
namespace asio = boost::asio;
using error_code = boost::system::error_code;
#define ENABLE_THIS_EXAMPLE
#elif defined(SQLITE_ORM_WITH_ASYNC) && __has_include(<asio.hpp>)
#include <asio.hpp>
using error_code = asio::error_code;
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
#include <chrono>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace sqlite_orm;
using std::cout;
using std::endl;

/**
 *  Embeds an `async_io_context` of sqlite_orm into an `asio::io_context`.
 */
class storage_driver {
  public:
    storage_driver(asio::io_context& host, async_io_context& io_) : io(io_), descriptor(host, io_.native_handle()) {
        this->wait();
    }

    ~storage_driver() {
        this->stop();
    }

    /**
     *  Stop watching. The descriptor is released rather than closed: it belongs to the `async_io_context`.
     */
    void stop() {
        if (this->descriptor.is_open()) {
            this->descriptor.release();
        }
    }

  private:
    void wait() {
        this->descriptor.async_wait(asio::posix::stream_descriptor::wait_read, [this](error_code error) {
            if (error) {
                return;  //  stopped
            }
            this->io.poll();
            this->wait();
        });
    }

    async_io_context& io;
    asio::posix::stream_descriptor descriptor;
};

/**
 *  Awaits `operation` and hands its result to the completion handler of Asio, through the handler's executor.
 */
template<class T, class Handler>
async_task<void> complete(async_task<T> operation, Handler handler) {
    std::optional<T> result;
    std::exception_ptr error;
    try {
        result.emplace(co_await std::move(operation));
    } catch (...) {
        error = std::current_exception();
    }
    auto executor = asio::get_associated_executor(handler);
    asio::post(executor, [handler = std::move(handler), result = std::move(result), error]() mutable {
        if (error) {
            handler(error, T{});
        } else {
            handler(error, std::move(*result));
        }
    });
}

template<class Handler>
async_task<void> complete(async_task<void> operation, Handler handler) {
    std::exception_ptr error;
    try {
        co_await std::move(operation);
    } catch (...) {
        error = std::current_exception();
    }
    auto executor = asio::get_associated_executor(handler);
    asio::post(executor, [handler = std::move(handler), error]() mutable {
        handler(error);
    });
}

/**
 *  A storage operation as an asynchronous operation of Asio:
 *
 *      std::vector<User> users = co_await await_task(io, storage.get_all<User>(), asio::use_awaitable);
 */
template<class T, class CompletionToken>
auto await_task(async_io_context& io, async_task<T> operation, CompletionToken&& token) {
    return asio::async_initiate<CompletionToken, void(std::exception_ptr, T)>(
        [&io](auto handler, async_task<T> operation) {
            io.spawn(complete(std::move(operation), std::move(handler)));
            io.poll();
        },
        token,
        std::move(operation));
}

template<class CompletionToken>
auto await_task(async_io_context& io, async_task<void> operation, CompletionToken&& token) {
    return asio::async_initiate<CompletionToken, void(std::exception_ptr)>(
        [&io](auto handler, async_task<void> operation) {
            io.spawn(complete(std::move(operation), std::move(handler)));
            io.poll();
        },
        token,
        std::move(operation));
}

struct User {
    int id = 0;
    std::string name;
    int age = 0;
};

auto open_storage(async_io_context& io, std::string filename) {
    return make_async_storage(io,
                              std::move(filename),
                              make_table("users",
                                         make_column("id", &User::id, primary_key().autoincrement()),
                                         make_column("name", &User::name),
                                         make_column("age", &User::age)));
}

using Storage = decltype(open_storage(std::declval<async_io_context&>(), ""));

/**
 *  A coroutine of Asio using the storage.
 */
asio::awaitable<void> use_storage(async_io_context& io, Storage& storage, bool& finished) {
    co_await await_task(io, storage.sync_schema(), asio::use_awaitable);
    for (int index = 1; index <= 100; ++index) {
        co_await await_task(io,
                            storage.insert(User{0, "user-" + std::to_string(index), 20 + index % 50}),
                            asio::use_awaitable);
    }
    User first = co_await await_task(io, storage.get<User>(1), asio::use_awaitable);
    first.age = 99;
    co_await await_task(io, storage.update(first), asio::use_awaitable);
    std::vector<User> elders =
        co_await await_task(io, storage.get_all<User>(where(c(&User::age) > 60)), asio::use_awaitable);
    cout << "users older than 60: " << elders.size() << endl;
    try {
        co_await await_task(io, storage.get<User>(1000), asio::use_awaitable);
    } catch (const std::system_error& error) {
        cout << "errors arrive as exceptions: " << error.what() << endl;
    }
    finished = true;
}

/**
 *  Another coroutine of Asio, a timer, served by the same loop while the storage waits for the disk.
 */
asio::awaitable<void> tick(const bool& finished, int& ticks) {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    while (!finished) {
        timer.expires_after(std::chrono::milliseconds(1));
        co_await timer.async_wait(asio::use_awaitable);
        ++ticks;
    }
}
#endif

int main(int, char**) {
#ifdef ENABLE_THIS_EXAMPLE
    if (!io_uring_available()) {
        cout << "io_uring is not available in this environment" << endl;
        return 0;
    }
    const char* filename = "async_asio.sqlite";
    std::remove(filename);

    asio::io_context host;
    async_io_context io;
    storage_driver driver(host, io);
    auto storage = open_storage(io, filename);

    bool finished = false;
    int ticks = 0;
    asio::co_spawn(host, use_storage(io, storage, finished), asio::detached);
    asio::co_spawn(host, tick(finished, ticks), [&driver](std::exception_ptr) {
        driver.stop();  //  nothing is left to do: lets host.run() return
    });
    host.run();
    cout << "the timer fired " << ticks << " times in the meantime" << endl;
#else
    std::cout << "the asynchronous storage or Asio is not available" << std::endl;
#endif
    return 0;
}
