/**
 *  This example demonstrates the asynchronous storage: the ordinary storage, awaited from C++20 coroutines.
 *
 *  While an operation waits for the disk, the thread is free to run other coroutines; the file I/O of SQLite
 *  goes through io_uring instead of blocking system calls. The `async_io_context` is what submits that I/O and resumes
 *  the coroutines. Here the application has no event loop of its own, so it simply calls `io.run()`;
 *  `examples/async_event_loops/` shows how the `async_io_context` is embedded into a loop the application already has.
 *
 *  Available on Linux with C++20 coroutines, see `docs/async.md`.
 */

#include <cstdio>  //  std::remove
#include <iostream>
#include <optional>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <utility>
#include <vector>

#ifdef SQLITE_ORM_WITH_ASYNC
using namespace sqlite_orm;
using std::cout;
using std::endl;

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

async_task<void> basics(Storage& storage) {
    co_await storage.sync_schema();

    //  the interface of the synchronous storage, with `co_await` in front
    const int annId = co_await storage.insert(User{0, "Ann", 30});
    co_await storage.insert(User{0, "Bob", 41});
    co_await storage.insert(User{0, "Kim", 17});

    User ann = co_await storage.get<User>(annId);
    ann.age = 31;
    co_await storage.update(ann);

    std::vector<User> adults = co_await storage.get_all<User>(where(c(&User::age) >= 18), order_by(&User::name));
    cout << "adults: " << adults.size() << endl;

    //  a transaction is one operation: the callable receives the synchronous storage and runs as a whole
    co_await storage.transaction([](Storage::storage_type& storage) {
        storage.insert(User{0, "Eve", 25});
        storage.remove_all<User>(where(c(&User::age) < 18));
        return true;  //  false rolls back
    });

    //  row by row: only one row is in memory at a time, and the connection is free between two rows
    auto users = storage.iterate<User>(order_by(&User::id));
    while (std::optional<User> user = co_await users.next()) {
        cout << user->id << " " << user->name << " " << user->age << endl;
    }
}

async_task<int> count_users(Storage& storage) {
    co_return co_await storage.count<User>();
}

/**
 *  A storage is one connection and executes its operations one after another.
 *  Two storages work at the same time, on one thread.
 */
async_task<void> two_connections(Storage& first, Storage& second) {
    std::vector<async_task<int>> counts;
    counts.push_back(count_users(first));
    counts.push_back(count_users(second));
    std::vector<int> results = co_await async_when_all(std::move(counts));
    cout << "counted concurrently: " << results[0] << " and " << results[1] << endl;
}

async_task<void> example(Storage& storage, Storage& sameDatabase) {
    co_await basics(storage);
    co_await two_connections(storage, sameDatabase);
}
#endif

int main(int, char**) {
#ifdef SQLITE_ORM_WITH_ASYNC
    if (!io_uring_available()) {
        cout << "io_uring is not available in this environment" << endl;
        return 0;
    }
    const char* filename = "async_storage.sqlite";
    std::remove(filename);

    async_io_context io;
    auto storage = open_storage(io, filename);
    auto sameDatabase = open_storage(io, filename);

    io.spawn(example(storage, sameDatabase));
    io.run();  //  returns when everything spawned has finished
#else
    std::cout << "the asynchronous storage is not available with this platform, compiler or C++ standard" << std::endl;
#endif
    return 0;
}
