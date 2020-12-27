#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("transaction") {
    struct Object {
        int id = 0;
        std::string name;
    };
    auto filename = "transaction_test.sqlite";
    ::remove(filename);
    auto storage = make_storage(
        "test_transaction_guard.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));
    REQUIRE(!storage.is_opened());
    storage.sync_schema();
    REQUIRE(!storage.is_opened());
    storage.transaction([&] {
        storage.insert(Object{0, "Jack"});
        return true;
    });
    REQUIRE(!storage.is_opened());
}

TEST_CASE("transaction_rollback") {
    struct Object {
        int id = 0;
        std::string name;
    };

    auto storage = make_storage(
        "test_transaction_guard.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));

    storage.sync_schema();
    storage.remove_all<Object>();

    storage.insert(Object{0, "Jack"});

    SECTION("insert, call make a storage to call an exception and check that rollback was fired") {
        auto countBefore = storage.count<Object>();
        try {
            storage.transaction([&] {
                storage.insert(Object{0, "John"});
                storage.get<Object>(-1);
                REQUIRE(false);
                return true;
            });
        } catch(...) {
            auto countNow = storage.count<Object>();

            REQUIRE(countBefore == countNow);
        }
    }
}

TEST_CASE("Transaction guard") {
    struct Object {
        int id = 0;
        std::string name;
    };

    auto storage = make_storage(
        "test_transaction_guard.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));

    storage.sync_schema();
    storage.remove_all<Object>();

    storage.insert(Object{0, "Jack"});

    SECTION("insert, call make a storage to call an exception and check that rollback was fired") {
        auto countBefore = storage.count<Object>();
        try {
            auto guard = storage.transaction_guard();

            storage.insert(Object{0, "John"});

            storage.get<Object>(-1);

            REQUIRE(false);
        } catch(...) {
            auto countNow = storage.count<Object>();

            REQUIRE(countBefore == countNow);
        }
    }
    SECTION("check that one can call other transaction functions without exceptions") {
        storage.transaction([&] {
            return false;
        });
    }
    SECTION("commit explicitly and check that after exception data was saved") {
        auto countBefore = storage.count<Object>();
        try {
            auto guard = storage.transaction_guard();
            storage.insert(Object{0, "John"});
            guard.commit();
            storage.get<Object>(-1);
            REQUIRE(false);
        } catch(...) {
            auto countNow = storage.count<Object>();

            REQUIRE(countNow == countBefore + 1);
        }
    }
    SECTION("rollback explicitly") {
        auto countBefore = storage.count<Object>();
        try {
            auto guard = storage.transaction_guard();
            storage.insert(Object{0, "Michael"});
            guard.rollback();
            storage.get<Object>(-1);
            REQUIRE(false);
        } catch(...) {
            auto countNow = storage.count<Object>();
            REQUIRE(countNow == countBefore);
        }
    }
    SECTION("commit on exception") {
        auto countBefore = storage.count<Object>();
        try {
            auto guard = storage.transaction_guard();
            guard.commit_on_destroy = true;
            storage.insert(Object{0, "Michael"});
            storage.get<Object>(-1);
            REQUIRE(false);
        } catch(...) {
            auto countNow = storage.count<Object>();
            REQUIRE(countNow == countBefore + 1);
        }
    }
    SECTION("work witout exception") {
        auto countBefore = storage.count<Object>();
        try {
            auto guard = storage.transaction_guard();
            guard.commit_on_destroy = true;
            storage.insert(Object{0, "Lincoln"});
        } catch(...) {
            throw std::runtime_error("Must not fire");
        }
        auto countNow = storage.count<Object>();
        REQUIRE(countNow == countBefore + 1);
    }
}
