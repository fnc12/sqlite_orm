#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("column tests is_generated") {
    struct User {
        int id = 0;
        int age = 0;
    };
    SECTION("no constraints") {
        auto column = make_column("id", &User::id);
        REQUIRE_FALSE(column.is_generated());
    }
#if SQLITE_VERSION_NUMBER >= 3031000
    SECTION("1 constraint: generated") {
        SECTION("full") {
            auto column = make_column("age", &User::age, generated_always_as(add(&User::id, 5)));
            REQUIRE(column.is_generated());
        }

        SECTION("not full") {
            auto column = make_column("age", &User::age, as(add(&User::id, 5)));
            REQUIRE(column.is_generated());
        }
    }
#endif
    SECTION("1 constraint: primary key") {
        auto column = make_column("id", &User::id, primary_key());
        REQUIRE_FALSE(column.is_generated());
    }
}

TEST_CASE("column is_not_null") {
    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };
    SECTION("non-nullable") {
        SECTION("implicit") {
            auto column = make_column("id", &User::id);
            REQUIRE(column.is_not_null());
        }
        SECTION("explicit not null") {
            auto column = make_column("id", &User::id, not_null());
            REQUIRE(column.is_not_null());
        }
        SECTION("explicit null") {
            auto column = make_column("id", &User::id, null());
            REQUIRE(column.is_not_null());
        }
    }
    SECTION("nullable") {
        SECTION("implicit") {
            auto column = make_column("name", &User::name);
            REQUIRE(!column.is_not_null());
        }
        SECTION("explicit not null") {
            auto column = make_column("name", &User::name, not_null());
            REQUIRE(!column.is_not_null());
        }
        SECTION("explicit null") {
            auto column = make_column("name", &User::name, null());
            REQUIRE(!column.is_not_null());
        }
    }
}
