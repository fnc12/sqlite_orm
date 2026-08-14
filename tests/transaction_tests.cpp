#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <cstdio>  //  std::remove
#include "catch_matchers.h"

using namespace sqlite_orm;

namespace {
    struct Object {
        int64 id = 0;
        std::string name;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        bool operator==(const Object&) const = default;
#else
        bool operator==(const Object& other) const {
            return this->id == other.id && this->name == other.name;
        }
#endif
    };
}

TEST_CASE("transaction") {
    auto filename = "transaction_test.sqlite";
    std::remove(filename);
    auto storage = make_storage(
        "test_transaction_guard.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));
    REQUIRE_FALSE(storage.is_opened());
    storage.sync_schema();
    REQUIRE_FALSE(storage.is_opened());
    storage.transaction([&] {
        storage.insert(Object{0, "Jack"});
        return true;
    });
    REQUIRE_FALSE(storage.is_opened());
}

TEST_CASE("begin_transaction") {
    auto storage = make_storage(
        {},
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));
    storage.sync_schema();

    SECTION("begin_transaction") {
        storage.begin_transaction();
    }
    SECTION("begin_deferred_transaction") {
        storage.begin_deferred_transaction();
    }
    SECTION("begin_exclusive_transaction") {
        storage.begin_exclusive_transaction();
    }
    SECTION("begin_immediate_transaction") {
        storage.begin_immediate_transaction();
    }

    storage.replace(Object{1, "Leony"});

    storage.commit();

    std::vector<Object> expected{{1, "Leony"}};
    REQUIRE(storage.get_all<Object>() == expected);
}

TEST_CASE("Transaction guard") {
    const ErrorCodeExceptionMatcher notFoundExceptionMatcher(orm_error_code::not_found);
    const ErrorCodeExceptionMatcher busyExceptionMatcher(sqlite_errc(SQLITE_BUSY));

    std::remove("guard.sqlite");
    auto table =
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name));
    auto storage = make_storage("guard.sqlite", table);

    storage.sync_schema();
    storage.remove_all<Object>();

    storage.insert(Object{0, "Jack"});

    SECTION("insert, call make a storage to call an exception and check that rollback was fired") {
        auto countBefore = storage.count<Object>();
        SECTION("transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.transaction_guard();
                    storage.insert(Object{0, "John"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("deferred_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.deferred_transaction_guard();
                    storage.insert(Object{0, "John"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("exclusive_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.exclusive_transaction_guard();
                    storage.insert(Object{0, "John"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("immediate_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.immediate_transaction_guard();
                    storage.insert(Object{0, "John"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        auto countNow = storage.count<Object>();
        REQUIRE(countBefore == countNow);
    }
    SECTION("check that one can call other transaction functions without exceptions") {
        REQUIRE_NOTHROW(storage.transaction([] {
            return false;
        }));
    }
    SECTION("commit explicitly and check that after exception data was saved") {
        auto countBefore = storage.count<Object>();
        SECTION("transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.transaction_guard();
                    storage.insert(Object{0, "John"});
                    guard.commit();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("deferred_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.deferred_transaction_guard();
                    storage.insert(Object{0, "John"});
                    guard.commit();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("exclusive_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.exclusive_transaction_guard();
                    storage.insert(Object{0, "John"});
                    guard.commit();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("immediate_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.immediate_transaction_guard();
                    storage.insert(Object{0, "John"});
                    guard.commit();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        auto countNow = storage.count<Object>();
        REQUIRE(countNow == countBefore + 1);
    }
    SECTION("rollback explicitly") {
        auto countBefore = storage.count<Object>();
        SECTION("transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.transaction_guard();
                    storage.insert(Object{0, "Michael"});
                    guard.rollback();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("deferred_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.deferred_transaction_guard();
                    storage.insert(Object{0, "Michael"});
                    guard.rollback();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("exclusive_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.exclusive_transaction_guard();
                    storage.insert(Object{0, "Michael"});
                    guard.rollback();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("immediate_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.immediate_transaction_guard();
                    storage.insert(Object{0, "Michael"});
                    guard.rollback();
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        auto countNow = storage.count<Object>();
        REQUIRE(countNow == countBefore);
    }
    SECTION("commit on exception") {
        auto countBefore = storage.count<Object>();
        SECTION("transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.transaction_guard();
                    guard.commit_on_destroy = true;
                    storage.insert(Object{0, "Michael"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("deferred_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.deferred_transaction_guard();
                    guard.commit_on_destroy = true;
                    storage.insert(Object{0, "Michael"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("exclusive_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.exclusive_transaction_guard();
                    guard.commit_on_destroy = true;
                    storage.insert(Object{0, "Michael"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        SECTION("immediate_transaction_guard") {
            REQUIRE_THROWS_MATCHES(
                [&storage] {
                    auto guard = storage.immediate_transaction_guard();
                    guard.commit_on_destroy = true;
                    storage.insert(Object{0, "Michael"});
                    storage.get<Object>(-1);
                }(),
                std::system_error,
                notFoundExceptionMatcher);
        }
        auto countNow = storage.count<Object>();
        REQUIRE(countNow == countBefore + 1);
    }
    SECTION("work without exception") {
        auto countBefore = storage.count<Object>();
        SECTION("transaction_guard") {
            auto guard = storage.transaction_guard();
            guard.commit_on_destroy = true;
            REQUIRE_NOTHROW(storage.insert(Object{0, "Lincoln"}));
        }
        SECTION("deferred_transaction_guard") {
            auto guard = storage.deferred_transaction_guard();
            guard.commit_on_destroy = true;
            REQUIRE_NOTHROW(storage.insert(Object{0, "Lincoln"}));
        }
        SECTION("exclusive_transaction_guard") {
            auto guard = storage.exclusive_transaction_guard();
            guard.commit_on_destroy = true;
            REQUIRE_NOTHROW(storage.insert(Object{0, "Lincoln"}));
        }
        SECTION("immediate_transaction_guard") {
            auto guard = storage.immediate_transaction_guard();
            guard.commit_on_destroy = true;
            REQUIRE_NOTHROW(storage.insert(Object{0, "Lincoln"}));
        }
        auto countNow = storage.count<Object>();
        REQUIRE(countNow == countBefore + 1);
    }
    SECTION("std::move ctor") {
        std::vector<internal::transaction_guard_t> guards;
        auto countBefore = storage.count<Object>();
        SECTION("transaction_guard") {
            auto guard = storage.transaction_guard();
            storage.insert(Object{0, "Lincoln"});
            guards.push_back(std::move(guard));
            REQUIRE(storage.count<Object>() == countBefore + 1);
        }
        SECTION("deferred_transaction_guard") {
            auto guard = storage.deferred_transaction_guard();
            storage.insert(Object{0, "Lincoln"});
            guards.push_back(std::move(guard));
            REQUIRE(storage.count<Object>() == countBefore + 1);
        }
        SECTION("exclusive_transaction_guard") {
            auto guard = storage.exclusive_transaction_guard();
            storage.insert(Object{0, "Lincoln"});
            guards.push_back(std::move(guard));
            REQUIRE(storage.count<Object>() == countBefore + 1);
        }
        SECTION("immediate_transaction_guard") {
            auto guard = storage.immediate_transaction_guard();
            storage.insert(Object{0, "Lincoln"});
            guards.push_back(std::move(guard));
            REQUIRE(storage.count<Object>() == countBefore + 1);
        }
        REQUIRE(storage.count<Object>() == countBefore + 1);
        guards.clear();
        REQUIRE(storage.count<Object>() == countBefore);
    }
    SECTION("exception propagated from dtor") {
        // create a second database connection
        auto storage2 = make_storage("guard.sqlite", table);
        auto guard2 = storage2.transaction_guard();
        storage2.get_all<Object>();

        alignas(internal::transaction_guard_t) char buffer[sizeof(internal::transaction_guard_t)];
        auto guard = new (&buffer) internal::transaction_guard_t{storage.transaction_guard()};
        storage.insert<Object>({});
        guard->commit_on_destroy = true;
        REQUIRE_THROWS_MATCHES(guard->~transaction_guard_t(), std::system_error, busyExceptionMatcher);
    }
    std::remove("guard.sqlite");
}

TEST_CASE("savepoint") {
    auto storage = make_storage(
        {},
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));
    storage.sync_schema();

    SECTION("release keeps changes") {
        storage.savepoint("first");
        storage.replace(Object{1, "Leony"});
        storage.release_savepoint("first");
        std::vector<Object> expected{{1, "Leony"}};
        REQUIRE(storage.get_all<Object>() == expected);
    }
    SECTION("rollback to undoes changes") {
        storage.savepoint("first");
        storage.replace(Object{1, "Leony"});
        storage.rollback_to_savepoint("first");
        storage.release_savepoint("first");
        REQUIRE(storage.get_all<Object>().empty());
    }
    SECTION("release removes savepoints back to the most recent matching name") {
        storage.savepoint("point");
        storage.replace(Object{1, "outer"});
        storage.savepoint("point");
        storage.replace(Object{2, "inner"});
        //  releases the inner savepoint only
        storage.release_savepoint("point");
        REQUIRE(storage.count<Object>() == 2);
        //  now targets the outer savepoint
        storage.rollback_to_savepoint("point");
        REQUIRE(storage.count<Object>() == 0);
        storage.release_savepoint("point");
    }
    SECTION("unknown savepoint name") {
        const ErrorCodeExceptionMatcher sqliteErrorMatcher(sqlite_errc(SQLITE_ERROR));
        storage.savepoint("real");
        REQUIRE_THROWS_MATCHES(storage.release_savepoint("unknown"), std::system_error, sqliteErrorMatcher);
        REQUIRE_THROWS_MATCHES(storage.rollback_to_savepoint("unknown"), std::system_error, sqliteErrorMatcher);
        storage.release_savepoint("real");
    }
}

TEST_CASE("savepoint function") {
    auto filename = "savepoint_function.sqlite";
    std::remove(filename);
    auto storage = make_storage(
        filename,
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));
    storage.sync_schema();

    SECTION("returning true releases the savepoint") {
        storage.savepoint("first", [&storage] {
            storage.replace(Object{1, "Leony"});
            return true;
        });
        REQUIRE(storage.count<Object>() == 1);
    }
    SECTION("returning false rolls the savepoint back") {
        storage.savepoint("first", [&storage] {
            storage.replace(Object{1, "Leony"});
            return false;
        });
        REQUIRE(storage.count<Object>() == 0);
    }
    std::remove(filename);
}

TEST_CASE("savepoint guard") {
    const ErrorCodeExceptionMatcher notFoundExceptionMatcher(orm_error_code::not_found);

    std::remove("savepoint_guard.sqlite");
    auto storage = make_storage(
        "savepoint_guard.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));
    storage.sync_schema();
    storage.insert(Object{0, "Jack"});
    auto countBefore = storage.count<Object>();

    SECTION("changes are rolled back on destroy") {
        {
            auto savepointGuard = storage.savepoint_guard("first");
            storage.insert(Object{0, "John"});
        }
        REQUIRE(storage.count<Object>() == countBefore);
    }
    SECTION("changes are kept with an explicit release") {
        {
            auto savepointGuard = storage.savepoint_guard("first");
            storage.insert(Object{0, "John"});
            savepointGuard.release();
        }
        REQUIRE(storage.count<Object>() == countBefore + 1);
    }
    SECTION("changes are kept with release_on_destroy") {
        {
            auto savepointGuard = storage.savepoint_guard("first");
            savepointGuard.release_on_destroy = true;
            storage.insert(Object{0, "John"});
        }
        REQUIRE(storage.count<Object>() == countBefore + 1);
    }
    SECTION("rollback_to keeps the savepoint active") {
        {
            auto savepointGuard = storage.savepoint_guard("first");
            storage.insert(Object{0, "John"});
            savepointGuard.rollback_to();
            storage.insert(Object{0, "Jane"});
            savepointGuard.release();
        }
        REQUIRE(storage.count<Object>() == countBefore + 1);
        REQUIRE(storage.count<Object>(where(c(&Object::name) == "Jane")) == 1);
    }
    SECTION("an exception rolls the savepoint back") {
        REQUIRE_THROWS_MATCHES(
            [&storage] {
                auto savepointGuard = storage.savepoint_guard("first");
                storage.insert(Object{0, "John"});
                storage.get<Object>(-1);
            }(),
            std::system_error,
            notFoundExceptionMatcher);
        REQUIRE(storage.count<Object>() == countBefore);
    }
    SECTION("partial rollback inside a transaction") {
        {
            auto transactionGuard = storage.transaction_guard();
            storage.insert(Object{0, "kept"});
            {
                auto savepointGuard = storage.savepoint_guard("inner");
                storage.insert(Object{0, "undone"});
            }
            transactionGuard.commit();
        }
        REQUIRE(storage.count<Object>() == countBefore + 1);
        REQUIRE(storage.count<Object>(where(c(&Object::name) == "undone")) == 0);
    }
    SECTION("an inner release is undone by an outer rollback") {
        {
            auto transactionGuard = storage.transaction_guard();
            {
                auto savepointGuard = storage.savepoint_guard("inner");
                storage.insert(Object{0, "John"});
                savepointGuard.release();
            }
            transactionGuard.rollback();
        }
        REQUIRE(storage.count<Object>() == countBefore);
    }
    std::remove("savepoint_guard.sqlite");
}
