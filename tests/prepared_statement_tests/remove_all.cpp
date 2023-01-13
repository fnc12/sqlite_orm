#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared remove all") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;

    const int defaultVisitTime = 50;

    auto filename = "prepared.sqlite";
    remove(filename);
    auto storage = make_storage(filename,
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key().autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key().autoincrement()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("time", &Visit::time, default_value(defaultVisitTime)),
                                           foreign_key(&Visit::userId).references(&User::id)),
                                make_table("users_and_visits",
                                           make_column("user_id", &UserAndVisit::userId),
                                           make_column("visit_id", &UserAndVisit::visitId),
                                           make_column("description", &UserAndVisit::description),
                                           primary_key(&UserAndVisit::userId, &UserAndVisit::visitId)));
    storage.sync_schema();

    storage.replace(User{1, "Team BS"});
    storage.replace(User{2, "Shy'm"});
    storage.replace(User{3, "Maître Gims"});

    storage.replace(UserAndVisit{2, 1, "Glad you came"});
    storage.replace(UserAndVisit{3, 1, "Shine on"});

    SECTION("Without conditions") {
        auto statement = storage.prepare(remove_all<User>());
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            storage.execute(statement);
            REQUIRE(storage.count<User>() == 0);
        }
    }
    SECTION("With conditions") {
        SECTION("1") {
            SECTION("by val") {
                auto statement = storage.prepare(remove_all<User>(where(is_equal(&User::id, 2))));
                REQUIRE(get<0>(statement) == 2);
                testSerializing(statement);
                storage.execute(statement);
                REQUIRE(storage.count<User>() == 2);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(1);
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
                get<0>(statement) = 1;
                REQUIRE(get<0>(statement) == 1);
                storage.execute(statement);
                REQUIRE(storage.count<User>() == 1);

                get<0>(statement) = 3;
                REQUIRE(get<0>(statement) == 3);
                storage.execute(statement);
                REQUIRE(storage.count<User>() == 0);
            }
            SECTION("by ref") {
                auto id = 2;
                auto statement = storage.prepare(remove_all<User>(where(is_equal(&User::id, std::ref(id)))));
                REQUIRE(get<0>(statement) == 2);
                REQUIRE(&get<0>(statement) == &id);
                testSerializing(statement);
                storage.execute(statement);
                REQUIRE(storage.count<User>() == 2);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(1);
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
                id = 1;
                REQUIRE(get<0>(statement) == 1);
                storage.execute(statement);
                REQUIRE(storage.count<User>() == 1);

                id = 3;
                REQUIRE(get<0>(statement) == 3);
                storage.execute(statement);
                REQUIRE(storage.count<User>() == 0);
            }
        }
        SECTION("2") {
            SECTION("by val") {
                auto statement = storage.prepare(
                    remove_all<User>(where(is_equal(&User::name, "Shy'm") and lesser_than(&User::id, 10))));
                REQUIRE(strcmp(get<0>(statement), "Shy'm") == 0);
                REQUIRE(get<1>(statement) == 10);
                testSerializing(statement);
                storage.execute(statement);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(1);
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
                get<0>(statement) = "Team BS";
                get<1>(statement) = 20.0;  //  assign double to int, sorry for warning
                REQUIRE(strcmp(get<0>(statement), "Team BS") == 0);
                REQUIRE(get<1>(statement) == 20);
                storage.execute(statement);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
                get<0>(statement) = "C Bool";
                get<1>(statement) = 30.0f;
                REQUIRE(strcmp(get<0>(statement), "C Bool") == 0);
                REQUIRE(get<1>(statement) == 30);
                storage.execute(statement);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
            }
            SECTION("by ref") {
                std::string name = "Shy'm";
                auto id = 10;
                auto statement = storage.prepare(remove_all<User>(
                    where(is_equal(&User::name, std::ref(name)) and lesser_than(&User::id, std::ref(id)))));
                REQUIRE(get<0>(statement) == "Shy'm");
                REQUIRE(&get<0>(statement) == &name);
                REQUIRE(get<1>(statement) == 10);
                REQUIRE(&get<1>(statement) == &id);
                testSerializing(statement);
                storage.execute(statement);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(1);
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
                name = "Team BS";
                id = 20.0;  //  assign double to int
                REQUIRE(get<0>(statement) == "Team BS");
                REQUIRE(get<1>(statement) == 20);
                storage.execute(statement);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
                name = "C Bool";
                id = 30.0f;
                REQUIRE(get<0>(statement) == "C Bool");
                REQUIRE(get<1>(statement) == 30);
                storage.execute(statement);
                {
                    auto ids = storage.select(&User::id);
                    decltype(ids) expected;
                    expected.push_back(3);
                    REQUIRE_THAT(ids, UnorderedEquals(expected));
                }
            }
        }
    }
}
