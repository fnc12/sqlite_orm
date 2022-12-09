#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("In") {
    using Catch::Matchers::UnorderedEquals;
    {
        struct User {
            int id = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            User() = default;
            User(int id) : id{id} {}
#endif

            bool operator==(const User &other) const {
                return this->id == other.id;
            }

            std::ostream &operator<<(std::ostream &os) const {
                return os << this->id;
            }
        };

        auto storage = make_storage("", make_table("users", make_column("id", &User::id, primary_key())));
        storage.sync_schema();
        storage.replace(User{1});
        storage.replace(User{2});
        storage.replace(User{3});
        std::vector<User> rows;
        decltype(rows) expected;
        SECTION("as is") {
            SECTION("dynamic") {
                rows = storage.get_all<User>(where(in(&User::id, {1, 2, 3})));
                expected.push_back({1});
                expected.push_back({2});
                expected.push_back({3});
            }
            SECTION("static") {
                SECTION("simple") {
                    rows = storage.get_all<User>(where(c(&User::id).in(1, 2, 3)));
                    expected.push_back({1});
                    expected.push_back({2});
                    expected.push_back({3});
                }
                SECTION("prepared statement") {
                    auto statement = storage.prepare(get_all<User>(where(c(&User::id).in(1, 2, 3))));
                    REQUIRE(get<0>(statement) == 1);
                    REQUIRE(get<1>(statement) == 2);
                    REQUIRE(get<2>(statement) == 3);
                    SECTION("as is") {
                        expected.push_back({1});
                        expected.push_back({2});
                        expected.push_back({3});
                    }
                    SECTION("1 -> 4") {
                        get<0>(statement) = 4;
                        expected.push_back({2});
                        expected.push_back({3});
                    }
                    SECTION("1 -> 4, 2 -> 5") {
                        get<0>(statement) = 4;
                        get<1>(statement) = 5;
                        expected.push_back({3});
                    }
                    SECTION("1 -> 4, 2 -> 5, 3 -> 6") {
                        get<0>(statement) = 4;
                        get<1>(statement) = 5;
                        get<2>(statement) = 6;
                    }
                    rows = storage.execute(statement);
                }
            }
        }
        SECTION("vector") {
            std::vector<int> inArgument;
            inArgument.push_back(1);
            inArgument.push_back(2);
            inArgument.push_back(3);
            rows = storage.get_all<User>(where(in(&User::id, inArgument)));
            expected.push_back({1});
            expected.push_back({2});
            expected.push_back({3});
        }
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
    {
        struct Letter {
            int id;
            std::string name;
        };
        auto storage = make_storage(
            "",
            make_table("letters", make_column("id", &Letter::id, primary_key()), make_column("name", &Letter::name)));
        storage.sync_schema();

        storage.replace(Letter{1, "A"});
        storage.replace(Letter{2, "B"});
        storage.replace(Letter{3, "C"});

        {
            auto letters = storage.get_all<Letter>(where(in(&Letter::id, {1, 2, 3})));
            REQUIRE(letters.size() == 3);
        }
        {
            auto rows = storage.select(columns(&Letter::name), where(in(&Letter::id, {1, 2, 3})));
            REQUIRE(rows.size() == 3);
        }
        {
            auto rows2 = storage.select(&Letter::name, where(in(&Letter::id, {1, 2, 3})));
            REQUIRE(rows2.size() == 3);
        }
    }
}
