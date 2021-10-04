#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    namespace primary_key_case {

        template<typename TUser, typename S>
        void insertSection(S& storage) {
            TUser user{};
            user.id = -1;
            user.name = "Juan";
            user.age = 57;
            user.email = "dummy@email.com";
            const auto id = storage.insert(user);
            const auto users = storage.template get_all<TUser>();
            REQUIRE(users.size() == 1);
            REQUIRE(-1 != users.front().id);
            REQUIRE(id == users.front().id);
        }

        template<typename TUser, typename S>
        void insertRangeSection(S& storage) {
            std::vector<TUser> usersInput;
            usersInput.push_back({-1, "Juan", 57, "dummy@email.com"});
            usersInput.push_back({-1, "Kevin", 27, "dummy@email.com"});
            storage.insert_range(usersInput.begin(), usersInput.end());
            const auto users = storage.template get_all<TUser>();
            REQUIRE(users.size() == usersInput.size());
            for(size_t i = 0; i < users.size(); ++i) {
                REQUIRE(-1 != users[i].id);
                usersInput[i].id = users[i].id;
            }
            REQUIRE(users == usersInput);
        }

    }  // end of namespace primary_key_case
    namespace default_value_case {

        static const char* const defaultID = "100";
        static const char* const defaultName = "dummy_name";

        template<typename TUser, typename S>
        void insertSection(S& storage) {
            storage.template insert<TUser>({"_", "_"});
            const auto users = storage.template get_all<TUser>();
            REQUIRE(users.size() == 1);
            REQUIRE(defaultID == users.front().id);
            REQUIRE(defaultName == users.front().name);
        }

        template<typename TUser, typename S>
        void insertRangeSection(S& storage) {
            std::vector<TUser> inputUsers = {{"_", "_"}};
            storage.insert_range(inputUsers.begin(), inputUsers.end());
            const auto users = storage.template get_all<TUser>();
            REQUIRE(users.size() == 1);
            REQUIRE(defaultID == users.front().id);
            REQUIRE(defaultName == users.front().name);
        }

    }  // end of namespace default_value_case
}  // end of anonymous namespace

TEST_CASE("Issue 663 - pk inside") {

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

    auto storage = make_storage("",  ///
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name),
                                           make_column("age", &User::age),
                                           make_column("email", &User::email, default_value("dummy@email.com"))));
    storage.sync_schema();

    SECTION("insert") {
        primary_key_case::insertSection<User>(storage);
    }

    SECTION("insert_range") {
        primary_key_case::insertRangeSection<User>(storage);
    }
}

TEST_CASE("Issue 663 - pk outside") {

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

    auto storage = make_storage("",  ///
                                make_table("users",
                                           make_column("id", &User::id),
                                           make_column("name", &User::name),
                                           make_column("age", &User::age),
                                           make_column("email", &User::email, default_value("dummy@email.com")),
                                           primary_key(&User::id)));
    storage.sync_schema();

    SECTION("insert") {
        primary_key_case::insertSection<User>(storage);
    }

    SECTION("insert_range") {
        primary_key_case::insertRangeSection<User>(storage);
    }
}

TEST_CASE("Issue 663 - pk outside, with default") {

    struct User {
        std::string id;
        std::string name;
    };

    auto storage =
        make_storage("",  ///
                     make_table("users",
                                make_column("id", &User::id, default_value(default_value_case::defaultID)),
                                make_column("name", &User::name, default_value(default_value_case::defaultName)),
                                primary_key(&User::id, &User::name)));
    storage.sync_schema();

    SECTION("insert") {
        default_value_case::insertSection<User>(storage);
    }

    SECTION("insert_range") {
        default_value_case::insertRangeSection<User>(storage);
    }
}

TEST_CASE("Issue 663 - pk inside, with default") {
    struct User {
        std::string id;
    };

    auto storage = make_storage("",  ///
                                make_table("users", make_column("id", &User::id, primary_key(), default_value("200"))));
    storage.sync_schema();

    SECTION("insert") {
        storage.insert<User>({"_"});
        const auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE("200" == users.front().id);
    }

    SECTION("insert_range") {
        std::vector<User> inputUsers = {{"_"}};
        storage.insert_range(inputUsers.begin(), inputUsers.end());
        const auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE("200" == users.front().id);
    }
}

TEST_CASE("Issue 663 - fail test") {
    struct User {
        std::string id;
    };

    auto storage = make_storage("",  ///
                                make_table("users", make_column("id", &User::id, primary_key(), default_value("200"))));
    storage.sync_schema();

    std::vector<User> inputUsers = {{"_"}, {"_"}};
    try {
        storage.insert_range(inputUsers.begin(), inputUsers.end());
        REQUIRE(false);
    } catch(const std::system_error& e) {
        REQUIRE(e.code() == std::make_error_code(orm_error_code::cannot_use_default_value));
        REQUIRE(storage.count<User>() == 0);
    }
}