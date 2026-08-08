#include <cstdio>  //  std::remove
#include <memory>  //  std::unique_ptr
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

/**
 *  These tests pin down `sync_schema` behavior for every column attribute which takes part
 *  in the schema comparison (existence, NOT NULL, default value presence, primary key
 *  membership/order, generated flag) as well as for every attribute which is deliberately
 *  NOT compared (column type, default value itself, generated expression and its storage kind,
 *  UNIQUE, CHECK, COLLATE, WITHOUT ROWID).
 *  Every scenario runs on the same database file so that a real migration takes place,
 *  and asserts the `sync_schema_simulate` == `sync_schema` invariant.
 */

TEST_CASE("sync_schema attribute matrix: NOT NULL toggle") {
    struct UserNullable {
        int id = 0;
        std::unique_ptr<int> age;
    };
    struct UserNotNull {
        int id = 0;
        int age = 0;
    };
    auto storagePath = "sync_schema_matrix_notnull.sqlite";
    std::remove(storagePath);
    SECTION("nullable -> NOT NULL") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &UserNullable::id, primary_key()),
                                                   make_column("age", &UserNullable::age)));
            storage.sync_schema();
            UserNullable user;
            user.id = 1;
            user.age = std::make_unique<int>(42);
            storage.replace(user);
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserNotNull::id, primary_key()),
                                               make_column("age", &UserNotNull::age)));
        SECTION("preserve = true") {
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            auto users = storage.get_all<UserNotNull>();
            REQUIRE(users.size() == 1);
            REQUIRE(users.front().age == 42);
        }
        SECTION("preserve = false") {
            auto simulateRes = storage.sync_schema_simulate();
            auto syncRes = storage.sync_schema();
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            REQUIRE(storage.count<UserNotNull>() == 0);
        }
    }
    SECTION("NOT NULL -> nullable") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &UserNotNull::id, primary_key()),
                                                   make_column("age", &UserNotNull::age)));
            storage.sync_schema();
            storage.replace(UserNotNull{1, 42});
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserNullable::id, primary_key()),
                                               make_column("age", &UserNullable::age)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<UserNullable>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().age);
        REQUIRE(*users.front().age == 42);
    }
    std::remove(storagePath);
}

TEST_CASE("sync_schema attribute matrix: default value presence toggle") {
    struct User {
        int id = 0;
        int score = 0;
    };
    auto storagePath = "sync_schema_matrix_default.sqlite";
    std::remove(storagePath);
    SECTION("no default -> default") {
        {
            auto storage = make_storage(
                storagePath,
                make_table("users", make_column("id", &User::id, primary_key()), make_column("score", &User::score)));
            storage.sync_schema();
            storage.replace(User{1, 10});
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("score", &User::score, default_value(42))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().score == 10);
    }
    SECTION("default -> no default") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &User::id, primary_key()),
                                                   make_column("score", &User::score, default_value(42))));
            storage.sync_schema();
            storage.replace(User{1, 10});
        }
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("score", &User::score)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().score == 10);
    }
    SECTION("default value change is NOT detected") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &User::id, primary_key()),
                                                   make_column("score", &User::score, default_value(1))));
            storage.sync_schema();
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("score", &User::score, default_value(2))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        //  only the presence of a default value is compared, not the value itself
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    std::remove(storagePath);
}

TEST_CASE("sync_schema attribute matrix: column type change is NOT detected") {
    struct UserInt {
        int id = 0;
        int data = 0;
    };
    struct UserText {
        int id = 0;
        std::string data;
    };
    auto storagePath = "sync_schema_matrix_type.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &UserInt::id, primary_key()), make_column("data", &UserInt::data)));
        storage.sync_schema();
        storage.replace(UserInt{1, 42});
    }
    auto storage = make_storage(
        storagePath,
        make_table("users", make_column("id", &UserText::id, primary_key()), make_column("data", &UserText::data)));
    auto simulateRes = storage.sync_schema_simulate(true);
    auto syncRes = storage.sync_schema(true);
    REQUIRE(simulateRes == syncRes);
    //  the column type is deliberately not compared (removed in 2020)
    REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    std::remove(storagePath);
}

TEST_CASE("sync_schema attribute matrix: composite primary key") {
    struct Record {
        int year = 0;
        int month = 0;
        int amount = 0;

        bool operator==(const Record& other) const {
            return this->year == other.year && this->month == other.month && this->amount == other.amount;
        }
    };
    auto storagePath = "sync_schema_matrix_pk.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &Record::year),
                                               make_column("month", &Record::month),
                                               make_column("amount", &Record::amount),
                                               primary_key(&Record::year, &Record::month)));
        storage.sync_schema();
        storage.replace(Record{2026, 8, 100});
    }
    SECTION("reordering columns within the primary key is detected") {
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &Record::year),
                                               make_column("month", &Record::month),
                                               make_column("amount", &Record::amount),
                                               primary_key(&Record::month, &Record::year)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("records") == sync_schema_result::dropped_and_recreated);
        auto records = storage.get_all<Record>();
        REQUIRE(records == std::vector<Record>{Record{2026, 8, 100}});
    }
    SECTION("extending the primary key is detected") {
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &Record::year),
                                               make_column("month", &Record::month),
                                               make_column("amount", &Record::amount),
                                               primary_key(&Record::year, &Record::month, &Record::amount)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("records") == sync_schema_result::dropped_and_recreated);
        auto records = storage.get_all<Record>();
        REQUIRE(records == std::vector<Record>{Record{2026, 8, 100}});
    }
    std::remove(storagePath);
}

#if SQLITE_VERSION_NUMBER >= 3031000
TEST_CASE("sync_schema attribute matrix: generated columns") {
    struct User {
        int id = 0;
        int hash = 0;
    };
    auto storagePath = "sync_schema_matrix_generated.sqlite";
    std::remove(storagePath);
    SECTION("regular column becomes generated") {
        {
            auto storage = make_storage(
                storagePath,
                make_table("users", make_column("id", &User::id, primary_key()), make_column("hash", &User::hash)));
            storage.sync_schema();
            storage.replace(User{5, 1000});
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &User::id, primary_key()),
                                    make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        //  the value is recomputed by the generated column expression
        REQUIRE(users.front().hash == 9);
    }
    SECTION("generated column becomes regular") {
        {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &User::id, primary_key()),
                                        make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4))));
            storage.sync_schema();
            storage.replace(User{5, 0});
        }
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("hash", &User::hash)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        //  the formerly generated value is materialized into the regular column
        REQUIRE(users.front().hash == 9);
    }
    SECTION("VIRTUAL <-> STORED change is NOT detected") {
        {
            auto storage = make_storage(
                storagePath,
                make_table("users",
                           make_column("id", &User::id, primary_key()),
                           make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4).virtual_())));
            storage.sync_schema();
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &User::id, primary_key()),
                                    make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4).stored())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        //  only the fact that a column is generated is compared, not its storage kind
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("generated expression change is NOT detected") {
        {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &User::id, primary_key()),
                                        make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4))));
            storage.sync_schema();
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &User::id, primary_key()),
                                    make_column("hash", &User::hash, generated_always_as(c(&User::id) + 5))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        //  the generated column expression is not compared
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    std::remove(storagePath);
}
#endif

TEST_CASE("sync_schema attribute matrix: constraints which are NOT compared") {
    struct User {
        int id = 0;
        std::string email;
    };
    auto storagePath = "sync_schema_matrix_constraints.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("email", &User::email)));
        storage.sync_schema();
    }
    SECTION("adding UNIQUE is NOT detected") {
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("email", &User::email, unique())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("adding CHECK is NOT detected") {
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("email", &User::email, check(length(&User::email) > 0))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("adding COLLATE is NOT detected") {
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("email", &User::email, collate_nocase())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("adding WITHOUT ROWID is NOT detected") {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("email", &User::email))
                .without_rowid());
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    std::remove(storagePath);
}

/**
 *  SQLite forbids `ALTER TABLE ... ADD COLUMN` for columns with a PRIMARY KEY or UNIQUE
 *  constraint and for columns with a non-constant default value. `sync_schema` must
 *  classify such new columns as `dropped_and_recreated` (going through the backup table)
 *  instead of attempting `ADD COLUMN` and failing at runtime.
 */
TEST_CASE("sync_schema new column edge cases") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storagePath = "sync_schema_matrix_add_column.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
        storage.sync_schema();
        storage.replace(User{1, "Michael"});
        storage.replace(User{2, "Lincoln"});
    }
    SECTION("new column with a column-level UNIQUE constraint") {
        struct UserV2 {
            int id = 0;
            std::string name;
            std::unique_ptr<std::string> email;
        };
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserV2::id, primary_key()),
                                               make_column("name", &UserV2::name),
                                               make_column("email", &UserV2::email, unique())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 2);
    }
    SECTION("new column referenced by a table-level UNIQUE constraint") {
        struct UserV2 {
            int id = 0;
            std::string name;
            std::unique_ptr<std::string> email;
        };
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserV2::id, primary_key()),
                                               make_column("name", &UserV2::name),
                                               make_column("email", &UserV2::email),
                                               unique(&UserV2::email)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 2);
        //  the UNIQUE constraint must actually be in effect now
        UserV2 user3;
        user3.name = "Sucre";
        user3.email = std::make_unique<std::string>("email@example.org");
        storage.insert(user3);
        UserV2 user4;
        user4.name = "Sara";
        user4.email = std::make_unique<std::string>("email@example.org");
        REQUIRE_THROWS(storage.insert(user4));
    }
    SECTION("new column with a non-constant default value") {
        struct UserV2 {
            int id = 0;
            std::string name;
            std::string created;
        };
        SECTION("CURRENT_TIMESTAMP") {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &UserV2::id, primary_key()),
                                        make_column("name", &UserV2::name),
                                        make_column("created", &UserV2::created, default_value(current_timestamp()))));
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            auto users = storage.get_all<UserV2>();
            REQUIRE(users.size() == 2);
        }
        SECTION("expression") {
            auto storage = make_storage(
                storagePath,
                make_table("users",
                           make_column("id", &UserV2::id, primary_key()),
                           make_column("name", &UserV2::name),
                           make_column("created", &UserV2::created, default_value(datetime("now", "localtime")))));
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            auto users = storage.get_all<UserV2>();
            REQUIRE(users.size() == 2);
        }
    }
    SECTION("new INTEGER PRIMARY KEY column") {
        struct EventV1 {
            int value = 0;
        };
        struct EventV2 {
            std::unique_ptr<int> id;
            int value = 0;
        };
        auto eventsPath = "sync_schema_matrix_add_pk_column.sqlite";
        std::remove(eventsPath);
        {
            auto storage = make_storage(eventsPath, make_table("events", make_column("value", &EventV1::value)));
            storage.sync_schema();
            storage.insert(EventV1{10});
            storage.insert(EventV1{20});
        }
        auto storage = make_storage(eventsPath,
                                    make_table("events",
                                               make_column("id", &EventV2::id, primary_key()),
                                               make_column("value", &EventV2::value)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("events") == sync_schema_result::dropped_and_recreated);
        auto events = storage.get_all<EventV2>();
        REQUIRE(events.size() == 2);
        std::remove(eventsPath);
    }
#if SQLITE_VERSION_NUMBER >= 3031000
    SECTION("new STORED generated column together with a new NOT NULL column without default") {
        struct UserV2 {
            int id = 0;
            std::string name;
            int hash = 0;
            std::string token;
        };
        //  the STORED generated column comes first on purpose: scanning of new columns must not
        //  stop on it and must still discover the NOT NULL column which makes data preservation impossible
        auto storage = make_storage(
            storagePath,
            make_table("users",
                       make_column("id", &UserV2::id, primary_key()),
                       make_column("name", &UserV2::name),
                       make_column("hash", &UserV2::hash, generated_always_as(c(&UserV2::id) + 4).stored()),
                       make_column("token", &UserV2::token)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated_with_data_loss);
        REQUIRE(storage.count<UserV2>() == 0);
    }
#endif
    std::remove(storagePath);
}

/**
 *  SQLite's `ALTER TABLE ... DROP COLUMN` refuses to drop a column which is part of the
 *  primary key, has a UNIQUE constraint, is indexed or is referenced in a generated column,
 *  index, trigger or view expression. `sync_schema` must fall back to recreating the table
 *  through a backup table in such cases instead of failing at runtime.
 */
TEST_CASE("sync_schema removed column edge cases") {
    SECTION("removed column is referenced by an index") {
        struct UserV1 {
            int id = 0;
            std::string name;
            int category = 0;
        };
        struct UserV2 {
            int id = 0;
            std::string name;
        };
        auto storagePath = "sync_schema_matrix_drop_indexed.sqlite";
        std::remove(storagePath);
        {
            auto storage = make_storage(storagePath,
                                        make_index("idx_users_category", &UserV1::category),
                                        make_table("users",
                                                   make_column("id", &UserV1::id, primary_key()),
                                                   make_column("name", &UserV1::name),
                                                   make_column("category", &UserV1::category)));
            storage.sync_schema();
            storage.replace(UserV1{1, "Michael", 10});
            storage.replace(UserV1{2, "Lincoln", 20});
        }
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &UserV2::id, primary_key()), make_column("name", &UserV2::name)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::old_columns_removed);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 2);
        std::remove(storagePath);
    }
    SECTION("removed column is part of the primary key") {
        struct RecordV1 {
            int year = 0;
            int month = 0;
            int amount = 0;
        };
        struct RecordV2 {
            int year = 0;
            int amount = 0;
        };
        auto storagePath = "sync_schema_matrix_drop_pk_member.sqlite";
        std::remove(storagePath);
        {
            auto storage = make_storage(storagePath,
                                        make_table("records",
                                                   make_column("year", &RecordV1::year),
                                                   make_column("month", &RecordV1::month),
                                                   make_column("amount", &RecordV1::amount),
                                                   primary_key(&RecordV1::year, &RecordV1::month)));
            storage.sync_schema();
            storage.replace(RecordV1{2025, 8, 100});
            storage.replace(RecordV1{2026, 8, 200});
        }
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &RecordV2::year),
                                               make_column("amount", &RecordV2::amount),
                                               primary_key(&RecordV2::year)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("records") == sync_schema_result::old_columns_removed);
        auto records = storage.get_all<RecordV2>();
        REQUIRE(records.size() == 2);
        std::remove(storagePath);
    }
#if SQLITE_VERSION_NUMBER >= 3031000
    SECTION("extra db column is removed while a generated column is present") {
        //  on SQLite < 3.35 this goes through the backup table, which must not try
        //  to copy the generated column
        struct UserV1 {
            int id = 0;
            int hash = 0;
            std::string obsolete;
        };
        struct UserV2 {
            int id = 0;
            int hash = 0;
        };
        auto storagePath = "sync_schema_matrix_drop_with_generated.sqlite";
        std::remove(storagePath);
        {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &UserV1::id, primary_key()),
                                        make_column("hash", &UserV1::hash, generated_always_as(c(&UserV1::id) + 4)),
                                        make_column("obsolete", &UserV1::obsolete, default_value(""))));
            storage.sync_schema();
            UserV1 user;
            user.id = 5;
            storage.replace(user);
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &UserV2::id, primary_key()),
                                    make_column("hash", &UserV2::hash, generated_always_as(c(&UserV2::id) + 4))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::old_columns_removed);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().hash == 9);
        std::remove(storagePath);
    }
#endif
}
