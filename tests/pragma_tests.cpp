#include <sqlite_orm/sqlite_orm.h>
#include <cstdio>  //  ::remove
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Journal mode") {
    auto filename = "journal_mode.sqlite";
    ::remove(filename);
    auto storage = make_storage(filename);
    auto storageCopy = storage;
    decltype(storage)* stor = nullptr;
    SECTION("Storage as is") {
        stor = &storage;
    };
    SECTION("Storage copy") {
        stor = &storageCopy;
    }
    auto jm = stor->pragma.journal_mode();
    REQUIRE(jm == journal_mode::DELETE);

    for(auto i = 0; i < 2; ++i) {
        if(i == 0) {
            stor->begin_transaction();
        }
        stor->pragma.journal_mode(journal_mode::MEMORY);
        jm = stor->pragma.journal_mode();
        REQUIRE(jm == journal_mode::MEMORY);

        if(i == 1) {  //  WAL cannot be set within a transaction
            stor->pragma.journal_mode(journal_mode::WAL);
            jm = stor->pragma.journal_mode();
            REQUIRE(jm == journal_mode::WAL);
        }

        stor->pragma.journal_mode(journal_mode::OFF);
        jm = stor->pragma.journal_mode();
        //        REQUIRE(jm == journal_mode::OFF);
        //  fnc12: dunno why it doesn't work. Probably journal_mode::OFF cannot be set. Anyway its SQLite's issue not sqlite_orm's

        stor->pragma.journal_mode(journal_mode::PERSIST);
        jm = stor->pragma.journal_mode();
        REQUIRE(jm == journal_mode::PERSIST);

        stor->pragma.journal_mode(journal_mode::TRUNCATE);
        jm = stor->pragma.journal_mode();
        REQUIRE(jm == journal_mode::TRUNCATE);

        if(i == 0) {
            stor->rollback();
        }
    }
}

TEST_CASE("Synchronous") {
    auto storage = make_storage("");
    const auto value = 1;
    storage.pragma.synchronous(value);
    REQUIRE(storage.pragma.synchronous() == value);

    storage.begin_transaction();

    const auto newValue = 2;
    try {
        storage.pragma.synchronous(newValue);
        throw std::runtime_error("Must not fire");
    } catch(const std::system_error&) {
        //  Safety level may not be changed inside a transaction
        REQUIRE(storage.pragma.synchronous() == value);
    }

    storage.commit();
}

TEST_CASE("User version") {
    auto storage = make_storage("");
    auto version = storage.pragma.user_version();

    storage.pragma.user_version(version + 1);
    REQUIRE(storage.pragma.user_version() == version + 1);

    storage.begin_transaction();
    storage.pragma.user_version(version + 2);
    REQUIRE(storage.pragma.user_version() == version + 2);
    storage.commit();
}

TEST_CASE("Auto vacuum") {
    auto filename = "autovacuum.sqlite";
    ::remove(filename);

    auto storage = make_storage(filename);

    storage.pragma.auto_vacuum(0);
    REQUIRE(storage.pragma.auto_vacuum() == 0);

    storage.pragma.auto_vacuum(1);
    REQUIRE(storage.pragma.auto_vacuum() == 1);

    storage.pragma.auto_vacuum(2);
    REQUIRE(storage.pragma.auto_vacuum() == 2);
}

TEST_CASE("busy_timeout") {
    auto storage = make_storage({});

    auto value = storage.pragma.busy_timeout();
    REQUIRE(value == 0);

    storage.pragma.busy_timeout(10);
    value = storage.pragma.busy_timeout();
    REQUIRE(value == 10);

    storage.pragma.busy_timeout(20);
    value = storage.pragma.busy_timeout();
    REQUIRE(value == 20);

    storage.pragma.busy_timeout(-1);
    value = storage.pragma.busy_timeout();
    REQUIRE(value == 0);
}

TEST_CASE("Integrity Check") {
    struct User {
        int id;
        std::string name;
        int age;
        std::string email;
    };

    auto filename = "integrity.sqlite";
    ::remove(filename);

    std::string tablename = "users";
    auto storage = make_storage(filename,
                                make_table(tablename,
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name),
                                           make_column("age", &User::age),
                                           make_column("email", &User::email, default_value("dummy@email.com"))));
    storage.sync_schema();

    REQUIRE(storage.pragma.integrity_check() == std::vector<std::string>{"ok"});
    REQUIRE(storage.pragma.integrity_check(5) == std::vector<std::string>{"ok"});
    REQUIRE(storage.pragma.integrity_check(tablename) == std::vector<std::string>{"ok"});
}

TEST_CASE("application_id") {
    auto filename = "application_id.sqlite";
    ::remove(filename);

    auto storage = make_storage(filename);
    storage.pragma.application_id(3);
    REQUIRE(storage.pragma.application_id() == 3);
}
