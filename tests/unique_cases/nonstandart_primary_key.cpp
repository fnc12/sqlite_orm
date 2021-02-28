#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    const std::string expectedErrorText =
        "Attempting to execute 'insert' request resulted in an error like \"NOT NULL constraint failed: "
        "users.username: constraint failed\". Perhaps ordinary 'insert' is not acceptable for this table and you "
        "should try 'replace' or 'insert' with explicit column listing?: constraint failed";
}

TEST_CASE("Nonstandart primary key - fail test") {
    struct User {
        std::string username;
        std::string password;
        bool isActive;
    };

    auto storage = make_storage({},
                                make_table("users",
                                           make_column("username", &User::username),
                                           make_column("password", &User::password),
                                           make_column("isActive", &User::isActive),
                                           primary_key(&User::username)));
    storage.sync_schema();

    SECTION("insert") {
        try {
            storage.insert(User{"testName", "testPassword2", false});
            REQUIRE(false);
        } catch(const std::system_error& e) {
            if(e.code() != std::error_code(SQLITE_CONSTRAINT, get_sqlite_error_category()))
                throw;
            REQUIRE(e.what() == expectedErrorText);
        }
    }

    SECTION("insert_range") {
        try {
            std::vector<User> users = {///
                                       {"testName1", "testPassword1", false},
                                       {"testName2", "testPassword2", true}};
            storage.insert_range(users.begin(), users.end());
            REQUIRE(false);
        } catch(const std::system_error& e) {
            if(e.code() != std::error_code(SQLITE_CONSTRAINT, get_sqlite_error_category()))
                throw;
            REQUIRE(e.what() == expectedErrorText);
        }
    }
}
