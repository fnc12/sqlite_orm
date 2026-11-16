/**
 *  This example serves a database over HTTP with libhv [https://github.com/ithewei/libhv] and the
 *  asynchronous storage, on libhv's own event loop.
 *
 *  libhv runs one event loop per I/O thread. Each of them gets an `io_context` and a storage, i.e. a connection,
 *  of its own, embedded into the loop by watching the `io_context`'s descriptor (see `database` below).
 *  A request handler starts a coroutine and returns; the coroutine answers the request when the database has.
 *  No thread ever waits for the disk, and no thread is added.
 *
 *      async_libhv [port]
 *      curl -X POST 'http://127.0.0.1:8080/users?name=Ann&age=30'
 *      curl http://127.0.0.1:8080/users
 */

#include <iostream>
#include <sqlite_orm/sqlite_orm.h>

#ifdef SQLITE_ORM_WITH_ASYNC
#include <exception>
#include <hv/HttpServer.h>
#include <hv/hloop.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>

using namespace sqlite_orm;

struct User {
    int id = 0;
    std::string name;
    int age = 0;
};

auto open_storage(io_context& io, std::string filename) {
    return make_async_storage(io,
                              std::move(filename),
                              make_table("users",
                                         make_column("id", &User::id, primary_key().autoincrement()),
                                         make_column("name", &User::name),
                                         make_column("age", &User::age)));
}

using Storage = decltype(open_storage(std::declval<io_context&>(), ""));

/**
 *  The database as one event loop of libhv sees it.
 */
struct database {
    io_context io;
    Storage storage;

    explicit database(hloop_t* loop) : storage(open_storage(this->io, "async_libhv.sqlite")) {
        //  the whole integration: libhv calls poll() whenever the io_context has something to do
        hio_t* watcher = hio_get(loop, this->io.native_handle());
        hio_set_context(watcher, &this->io);
        hio_add(
            watcher,
            [](hio_t* watcher) {
                static_cast<io_context*>(hio_context(watcher))->poll();
            },
            HV_READ);
        //  the event loops share the database file: a connection finding it locked by another one waits,
        //  and that wait does not block its thread either
        this->start(this->storage.busy_timeout(5000));
    }

    /**
     *  Start an operation whose result nobody waits for.
     */
    template<class T>
    void start(task<T> operation) {
        this->io.spawn(database::discard(std::move(operation)));
        this->io.poll();
    }

    /**
     *  The database of the event loop serving `context`, created on its first request.
     */
    static database& of(const HttpContextPtr& context) {
        thread_local std::unique_ptr<database> instance;
        if (!instance) {
            instance = std::make_unique<database>(hevent_loop(context->writer->io()));
        }
        return *instance;
    }

  private:
    template<class T>
    static task<void> discard(task<T> operation) {
        co_await std::move(operation);
    }
};

task<void> create_schema(Storage& storage) {
    co_await storage.sync_schema();
}

void send_error(const HttpContextPtr& context, const std::exception& error) {
    context->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
    context->send(error.what(), TEXT_PLAIN);
}

task<void> list_users(database& database, HttpContextPtr context) {
    try {
        hv::Json body = hv::Json::array();
        auto users = database.storage.iterate<User>(order_by(&User::id));
        while (std::optional<User> user = co_await users.next()) {
            body.push_back({{"id", user->id}, {"name", user->name}, {"age", user->age}});
        }
        context->sendJson(body);
    } catch (const std::exception& error) {
        send_error(context, error);
    }
}

task<void> add_user(database& database, HttpContextPtr context) {
    try {
        User user{0, context->param("name"), std::stoi(context->param("age", "0"))};
        user.id = co_await database.storage.insert(user);
        context->sendJson(hv::Json{{"id", user.id}});
    } catch (const std::exception& error) {
        send_error(context, error);
    }
}

#endif

int main(int argc, char** argv) {
#ifdef SQLITE_ORM_WITH_ASYNC
    const int port = argc > 1 ? std::stoi(argv[1]) : 8080;
    if (!io_uring_available()) {
        printf("io_uring is not available in this environment\n");
        return 0;
    }

    //  the schema is synchronized once, before the event loops open their connections: two of them doing it
    //  at the same time would both try to create the table
    {
        io_context io;
        Storage storage = open_storage(io, "async_libhv.sqlite");
        io.spawn(create_schema(storage));
        io.run();
    }

    HttpService router;
    //  the handlers run on libhv's I/O threads; `HTTP_STATUS_UNFINISHED` leaves the response to the coroutine
    router.GET("/users", [](const HttpContextPtr& context) {
        database& database = database::of(context);
        database.start(list_users(database, context));
        return HTTP_STATUS_UNFINISHED;
    });
    router.POST("/users", [](const HttpContextPtr& context) {
        database& database = database::of(context);
        database.start(add_user(database, context));
        return HTTP_STATUS_UNFINISHED;
    });

    hv::HttpServer server(&router);
    server.setPort(port);
    server.setThreadNum(2);
    printf("listening on http://127.0.0.1:%d, stop with Ctrl-C\n", port);
    server.run();
#else
    (void)argc;
    (void)argv;
    std::cout << "the asynchronous storage is not available with this platform, compiler or C++ standard" << std::endl;
#endif
    return 0;
}
