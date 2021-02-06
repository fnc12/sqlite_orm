#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Primary key 1") {
    struct User {
        bool operator==(const User& rhs) const {
            return std::tie(id, name, age, email) == std::tie(rhs.id, rhs.name, rhs.age, rhs.email);
        }
        bool operator!=(const User& rhs) const {
            return !(rhs == *this);
        }
        int id;
        std::string name;
        int age;
        std::string email;
    };

    auto storage = make_storage("primary_key.sqlite",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name),
                                           make_column("age", &User::age),
                                           make_column("email", &User::email, default_value("dummy@email.com"))));
    storage.sync_schema();

    SECTION("insert") {
        storage.remove_all<User>();
        User user{};
        user.id = -1;
        user.name = "Juan";
        user.age = 57;
        user.email = "dummy@email.com";
        const auto id = storage.insert(user);
        const auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE(-1 != users.front().id);
        REQUIRE(id == users.front().id);
    }

    SECTION("insert_range") {
        storage.remove_all<User>();
        std::vector<User> usersInput;
        usersInput.push_back({-1, "Juan", 57, "dummy@email.com"});
        usersInput.push_back({-1, "Kevin", 27, "dummy@email.com"});
        storage.insert_range(usersInput.begin(), usersInput.end());
        const auto users = storage.get_all<User>();
        REQUIRE(users.size() == usersInput.size());
        for (size_t i=0;i<users.size();++i)
        {
            REQUIRE(-1 != users[i].id);
            usersInput[i].id = users[i].id;
        }
        REQUIRE(users == usersInput);
    }

    SECTION("replace") {
        // TODO: implement this
    }
}

TEST_CASE("Primary key 2") {
    struct User {
        bool operator==(const User& rhs) const {
            return std::tie(id, name, age, email) == std::tie(rhs.id, rhs.name, rhs.age, rhs.email);
        }
        bool operator!=(const User& rhs) const {
            return !(rhs == *this);
        }
        int id;
        std::string name;
        int age;
        std::string email;
    };

    auto storage = make_storage("primary_key2.sqlite",
                                make_table("users",
                                           make_column("id", &User::id),
                                           make_column("name", &User::name),
                                           make_column("age", &User::age),
                                           make_column("email", &User::email, default_value("dummy@email.com")),
                                           primary_key(&User::id)));
    storage.sync_schema();
    storage.remove_all<User>();

    SECTION("insert") {
        storage.remove_all<User>();
        User user{};
        user.id = -1;
        user.name = "Juan";
        user.age = 57;
        user.email = "dummy@email.com";
        const auto id = storage.insert(user);
        const auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE(-1 != users.front().id);
        REQUIRE(id == users.front().id);
    }

    SECTION("insert_range") {
        storage.remove_all<User>();
        std::vector<User> usersInput;
        usersInput.push_back({-1, "Juan", 57, "dummy@email.com"});
        usersInput.push_back({-1, "Kevin", 27, "dummy@email.com"});
        storage.insert_range(usersInput.begin(), usersInput.end());
        const auto users = storage.get_all<User>();
        REQUIRE(users.size() == usersInput.size());
        for (size_t i=0;i<users.size();++i)
        {
            REQUIRE(-1 != users[i].id);
            usersInput[i].id = users[i].id;
        }
        REQUIRE(users == usersInput);
    }

    SECTION("replace") {
        // TODO: implement this
    }
}