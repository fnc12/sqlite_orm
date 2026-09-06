#include <sqlite_orm/sqlite_orm.h>
#ifdef SQLITE_ORM_ASYNC_SUPPORTED
#include <catch2/catch_all.hpp>
#include <algorithm>  //  std::find
#include <cstdint>  //  std::uint64_t
#include <filesystem>  //  std::filesystem::path, temp_directory_path
#include <memory>  //  std::unique_ptr
#include <optional>  //  std::optional
#include <random>  //  std::mt19937, std::random_device
#include <string>  //  std::string
#include <system_error>  //  std::system_error
#include <thread>  //  std::this_thread
#include <utility>  //  std::move
#include <vector>  //  std::vector

using namespace sqlite_orm;

//  Note: awaited expressions are always stored in a variable before an
//  assertion. Catch2's assertion macros mention the expression twice (once in
//  `__builtin_constant_p`, which GCC evaluates when it contains `co_await`),
//  so `REQUIRE(co_await ...)` would run the operation twice.

namespace {
    struct User {
        int id = 0;
        std::string name;
        int age = 0;
    };

    auto users_table() {
        return make_table("users",
                          make_column("id", &User::id, primary_key().autoincrement()),
                          make_column("name", &User::name),
                          make_column("age", &User::age));
    }

    using Storage = decltype(make_async_storage(std::declval<io_context&>(), "", users_table()));

    struct temp_database {
        std::filesystem::path directory;
        std::string path;

        temp_database() {
            this->directory =
                std::filesystem::temp_directory_path() / ("sqlite_orm_async_" + std::to_string(std::random_device{}()));
            std::filesystem::create_directories(this->directory);
            this->path = (this->directory / "test.sqlite").string();
        }

        ~temp_database() {
            std::error_code error;
            std::filesystem::remove_all(this->directory, error);
        }
    };

    void require_io_uring() {
        if (!io_uring_available()) {
            SKIP("io_uring is not available in this environment");
        }
    }

    task<void> mark_done(task<void> body, bool& done) {
        co_await std::move(body);
        done = true;
    }

    /**
     *  Coroutine lambdas must outlive their frames: test bodies are stored in a
     *  named variable by the caller and only the task is handed over here.
     */
    void run_test(io_context& io, task<void> body) {
        bool done = false;
        io.spawn(mark_done(std::move(body), done));
        io.run();
        REQUIRE(done);
    }

    task<void> fill_users(Storage& storage, int count) {
        co_await storage.sync_schema();
        co_await storage.transaction([count](auto& storage) {
            for (int index = 1; index <= count; ++index) {
                storage.insert(User{0, "user-" + std::to_string(index), index % 90});
            }
            return true;
        });
    }
}

TEST_CASE("async storage: factory - VFS injection and lazy open") {
    require_io_uring();
    temp_database database;
    io_context io;
    auto storage = make_async_storage(io, database.path, users_table());

    //  Nothing is opened until the first operation, and nothing global changed.
    REQUIRE_FALSE(storage.is_opened());
    REQUIRE_FALSE(std::filesystem::exists(database.path));
    REQUIRE(sqlite3_vfs_find(nullptr) != sqlite3_vfs_find(async_vfs_name.data()));
    REQUIRE(storage.vfs_name() == async_vfs_name);
    REQUIRE(storage.open_mode() == db_open_mode::default_);
    REQUIRE(storage.filename() == database.path);
    REQUIRE_FALSE(storage.is_busy());

    auto body = [&storage, &database]() -> task<void> {
        co_await storage.sync_schema();
        REQUIRE(std::filesystem::exists(database.path));
        REQUIRE(storage.is_opened());
    };
    run_test(io, body());
    REQUIRE_FALSE(storage.is_busy());
}

TEST_CASE("async storage: user connection_control keeps its other fields") {
    require_io_uring();
    temp_database database;
    io_context io;
    connection_control control{};
    control.open_mode = db_open_mode::create_readwrite;
    control.vfs_name = "unix";  //  overridden by the asynchronous VFS
    auto storage = make_async_storage(io, database.path, control, users_table());
    REQUIRE(storage.vfs_name() == async_vfs_name);
    REQUIRE(storage.open_mode() == db_open_mode::create_readwrite);
}

TEST_CASE("async storage: CRUD") {
    require_io_uring();
    temp_database database;
    io_context io;
    reset_async_vfs_statistics();
    auto storage = make_async_storage(io, database.path, users_table());

    auto body = [&storage]() -> task<void> {
        co_await fill_users(storage, 200);
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 200);
        }

        //  get / get_pointer / get_optional
        User seventh = co_await storage.get<User>(7);
        REQUIRE(seventh.name == "user-7");
        auto pointer = co_await storage.get_pointer<User>(7);
        REQUIRE(pointer);
        REQUIRE(pointer->age == 7);
        auto optional = co_await storage.get_optional<User>(7);
        REQUIRE(optional.has_value());
        {
            const auto value = co_await storage.get_pointer<User>(999999);
            REQUIRE_FALSE(value);
        }
        {
            const auto value = co_await storage.get_optional<User>(999999);
            REQUIRE_FALSE(value.has_value());
        }

        //  insert returns the rowid; last_insert_rowid agrees
        int newId = co_await storage.insert(User{0, "new", 1});
        REQUIRE(newId == 201);
        {
            const auto value = co_await storage.last_insert_rowid();
            REQUIRE(value == 201);
        }
        {
            const auto value = co_await storage.changes();
            REQUIRE(value == 1);
        }

        //  update / replace / remove
        seventh.age = 120;
        co_await storage.update(seventh);
        {
            const User updated = co_await storage.get<User>(7);
            REQUIRE(updated.age == 120);
        }
        co_await storage.replace(User{7, "replaced", 121});
        {
            const User replaced = co_await storage.get<User>(7);
            REQUIRE(replaced.name == "replaced");
        }
        co_await storage.remove<User>(7);
        {
            const auto value = co_await storage.get_pointer<User>(7);
            REQUIRE_FALSE(value);
        }

        //  get_all with the DSL; rvalue arguments are moved, not copied
        auto olds = co_await storage.get_all<User>(where(c(&User::age) >= 85), order_by(&User::id), limit(10));
        REQUIRE(olds.size() == 10);
        std::string wanted = "user-42";
        auto names = co_await storage.select(columns(&User::name), where(c(&User::name) == std::move(wanted)));
        REQUIRE(names.size() == 1);
        REQUIRE(std::get<0>(names[0]) == "user-42");
        auto pointers = co_await storage.get_all_pointer<User>(where(c(&User::id) <= 3));
        REQUIRE(pointers.size() == 3);
        auto optionals = co_await storage.get_all_optional<User>(where(c(&User::id) <= 2));
        REQUIRE(optionals.size() == 2);

        //  update_all / remove_all
        co_await storage.update_all(set(c(&User::age) = 1), where(c(&User::id) <= 10));
        {
            const auto value = co_await storage.count<User>(where(c(&User::age) == 1 and c(&User::id) <= 10));
            REQUIRE(value == 9);
        }
        co_await storage.remove_all<User>(where(c(&User::id) > 150));
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 149);
        }

        //  insert_range / replace_range
        std::vector<User> batch{{0, "batch-1", 5}, {0, "batch-2", 6}};
        co_await storage.insert_range(batch.begin(), batch.end());
        {
            const auto value = co_await storage.count<User>(where(like(&User::name, "batch-%")));
            REQUIRE(value == 2);
        }
        std::vector<User> replacements{{1, "first", 50}};
        co_await storage.replace_range(replacements.begin(), replacements.end());
        {
            const User first = co_await storage.get<User>(1);
            REQUIRE(first.name == "first");
        }

        //  not-found error crosses the fiber boundary
        bool caught = false;
        try {
            co_await storage.get<User>(999999);
        } catch (const std::system_error& error) {
            caught = error.code() == orm_error_code::not_found;
        }
        REQUIRE(caught);
    };
    run_test(io, body());

    async_vfs_statistics statistics = get_async_vfs_statistics();
    REQUIRE(statistics.asyncReads > 0);
    REQUIRE(statistics.asyncWrites > 0);
    REQUIRE(statistics.asyncSyncs > 0);
    REQUIRE(io.scheduler().stats().synchronousFallbacks == 0);
}

TEST_CASE("async storage: aggregates") {
    require_io_uring();
    temp_database database;
    io_context io;
    auto storage = make_async_storage(io, database.path, users_table());

    auto body = [&storage]() -> task<void> {
        co_await fill_users(storage, 10);
        {
            const auto value = co_await storage.count(&User::age);
            REQUIRE(value == 10);
        }
        {
            const auto value = co_await storage.avg(&User::age);
            REQUIRE(value == Catch::Approx(5.5));
        }
        auto sum = co_await storage.sum(&User::age);
        REQUIRE(sum);
        REQUIRE(*sum == 55);
        {
            const auto value = co_await storage.total(&User::age);
            REQUIRE(value == Catch::Approx(55));
        }
        auto maximum = co_await storage.max(&User::age);
        REQUIRE(maximum);
        REQUIRE(*maximum == 10);
        auto minimum = co_await storage.min(&User::age, where(c(&User::age) > 3));
        REQUIRE(minimum);
        REQUIRE(*minimum == 4);
        {
            const auto value = co_await storage.group_concat(&User::name, ",", where(c(&User::id) <= 2));
            REQUIRE(value == "user-1,user-2");
        }
    };
    run_test(io, body());
}

TEST_CASE("async storage: prepared statements") {
    require_io_uring();
    temp_database database;
    io_context io;
    auto storage = make_async_storage(io, database.path, users_table());

    auto body = [&storage]() -> task<void> {
        co_await fill_users(storage, 20);

        auto selectStatement = co_await storage.prepare(select(columns(&User::name), where(c(&User::id) == 5)));
        auto rows = co_await storage.execute(selectStatement);
        REQUIRE(rows.size() == 1);
        REQUIRE(std::get<0>(rows[0]) == "user-5");
        get<0>(selectStatement) = 6;  //  rebinding needs no await
        rows = co_await storage.execute(selectStatement);
        REQUIRE(std::get<0>(rows[0]) == "user-6");

        auto insertStatement = co_await storage.prepare(insert(User{0, "prepared", 33}));
        int64 rowid = co_await storage.execute(insertStatement);
        REQUIRE(rowid == 21);
        get<0>(insertStatement).name = "prepared-2";
        {
            const auto value = co_await storage.execute(insertStatement);
            REQUIRE(value == 22);
        }

        auto getStatement = co_await storage.prepare(get<User>(21));
        User user = co_await storage.execute(getStatement);
        REQUIRE(user.name == "prepared");

        auto removeStatement = co_await storage.prepare(remove<User>(22));
        co_await storage.execute(removeStatement);
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 21);
        }

        REQUIRE(storage.dump(getStatement) == R"(SELECT "id", "name", "age" FROM "users" WHERE "id" = ?)");
    };
    run_test(io, body());
}

TEST_CASE("async storage: transactions and savepoints") {
    require_io_uring();
    temp_database database;
    io_context io;
    auto storage = make_async_storage(io, database.path, users_table());

    auto body = [&storage]() -> task<void> {
        co_await fill_users(storage, 5);

        //  returning false rolls back
        co_await storage.transaction([](auto& storage) {
            storage.insert(User{0, "ghost", 1});
            return false;
        });
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 5);
        }

        //  an exception rolls back and propagates
        bool caught = false;
        try {
            co_await storage.transaction([](auto& storage) -> bool {
                storage.insert(User{0, "ghost", 1});
                throw std::runtime_error("boom");
            });
        } catch (const std::runtime_error& error) {
            caught = std::string(error.what()) == "boom";
        }
        REQUIRE(caught);
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 5);
        }

        {
            const bool committed = co_await storage.savepoint("inner", [](auto& storage) {
                storage.insert(User{0, "kept", 1});
                return true;
            });
            REQUIRE(committed);
        }
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 6);
        }
        {
            const auto value = co_await storage.get_autocommit();
            REQUIRE(value);
        }
    };
    run_test(io, body());
}

TEST_CASE("async storage: schema and connection information") {
    require_io_uring();
    temp_database database;
    io_context io;
    auto storage = make_async_storage(io, database.path, users_table());

    auto body = [&storage, &database]() -> task<void> {
        auto simulated = co_await storage.sync_schema_simulate();
        REQUIRE(simulated.at("users") == sync_schema_result::new_table_created);
        auto results = co_await storage.sync_schema();
        REQUIRE(results.at("users") == sync_schema_result::new_table_created);
        {
            const auto value = co_await storage.table_exists("users");
            REQUIRE(value);
        }
        {
            const auto value = co_await storage.table_exists("nope");
            REQUIRE_FALSE(value);
        }
        auto tables = co_await storage.table_names();
        REQUIRE(tables.size() == 2);  //  users and sqlite_sequence (autoincrement)
        REQUIRE(std::find(tables.begin(), tables.end(), "users") != tables.end());
        {
            const auto views = co_await storage.view_names();
            REQUIRE(views.empty());
            const auto triggers = co_await storage.trigger_names();
            REQUIRE(triggers.empty());
        }
        co_await storage.run([](auto& storage) {
            storage.pragma.journal_mode(journal_mode::WAL);
        });
        co_await storage.vacuum();
        co_await storage.analyze();
        {
            const auto value = co_await storage.db_readonly();
            REQUIRE_FALSE(value);
        }
        {
            const auto value = (co_await storage.current_date()).size();
            REQUIRE(value == 10);
        }
        {
            const auto value = (co_await storage.current_time()).size();
            REQUIRE(value == 8);
        }
        {
            const auto value = (co_await storage.current_timestamp()).size();
            REQUIRE(value == 19);
        }
        {
            const auto value = co_await storage.busy_timeout(1000);
            REQUIRE(value == SQLITE_OK);
        }
        {
            const auto value = co_await storage.total_changes();
            const auto value64 = co_await storage.total_changes64();
            REQUIRE(value == value64);
        }
        {
            const auto value = co_await storage.db_name(0);
            REQUIRE(value == std::optional<std::string>{"main"});
        }
        {
            const auto value = co_await storage.txn_state();
            REQUIRE(value == transaction_state::none);
        }

        const std::string backupPath = database.path + ".backup";
        co_await storage.insert(User{0, "backed up", 1});
        co_await storage.backup_to(backupPath);
        REQUIRE(std::filesystem::exists(backupPath));
        co_await storage.remove_all<User>();
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 0);
        }
        co_await storage.backup_from(backupPath);
        {
            const auto value = co_await storage.count<User>();
            REQUIRE(value == 1);
        }

        co_await storage.drop_table("users");
        {
            const auto value = co_await storage.table_exists("users");
            REQUIRE_FALSE(value);
        }
        REQUIRE(storage.libversion() == SQLITE_VERSION);
    };
    run_test(io, body());
}

TEST_CASE("async storage: operations from several coroutines are queued and the thread stays free") {
    require_io_uring();
    temp_database database;
    io_context io;
    auto storage = make_async_storage(io, database.path, users_table());

    std::uint64_t epoch = 0;
    std::uint64_t interleaved = 0;
    auto reader = [&storage, &epoch, &interleaved](int seed) -> task<long> {
        std::mt19937 generator(static_cast<unsigned>(seed));
        long total = 0;
        for (int index = 0; index < 30; ++index) {
            const int id = static_cast<int>(generator() % 2000) + 1;
            const std::uint64_t before = ++epoch;
            User user = co_await storage.get<User>(id);
            if (epoch != before) {
                ++interleaved;
            }
            REQUIRE(user.id == id);
            total += user.age;
        }
        co_return total;
    };

    auto body = [&storage, &reader, &interleaved]() -> task<void> {
        co_await fill_users(storage, 2000);
        std::vector<task<long>> readers;
        for (int index = 0; index < 8; ++index) {
            readers.push_back(reader(index + 1));
        }
        std::vector<long> totals = co_await when_all(std::move(readers));
        REQUIRE(totals.size() == 8);
        //  Requests from the eight coroutines were interleaved on one connection...
        REQUIRE(interleaved > 0);
        //  ...and the storage is free again.
        REQUIRE_FALSE(storage.is_busy());
    };
    run_test(io, body());
}

TEST_CASE("async storage: io_pool spreads work over threads and resumes on the origin") {
    require_io_uring();
    temp_database database;
    io_context io;
    io_pool pool(3);
    auto storage = make_async_storage(io, database.path, users_table());

    auto body = [&storage, &pool]() -> task<void> {
        co_await storage.sync_schema();
        const std::thread::id origin = std::this_thread::get_id();
        int answer = co_await pool.async([origin] {
            REQUIRE(std::this_thread::get_id() != origin);
            return 42;
        });
        REQUIRE(answer == 42);
        REQUIRE(std::this_thread::get_id() == origin);
    };
    run_test(io, body());
    REQUIRE(pool.run_blocking([] {
        return 7;
    }) == 7);
    pool.shutdown();
}
#endif
