#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared get pointer") {
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

    {
        auto statement = storage.prepare(get_pointer<User>(1));
        REQUIRE(get<0>(statement) == 1);
        std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{1, "Team BS"});
            }
            get<0>(statement) = 2;
            REQUIRE(get<0>(statement) == 2);
            {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{2, "Shy'm"});
            }
        }
    }
    {
        auto statement = storage.prepare(get_pointer<User>(2));
        REQUIRE(get<0>(statement) == 2);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto user = storage.execute(statement);
            REQUIRE(user);
            REQUIRE(*user == User{2, "Shy'm"});
        }
    }
    {
        auto statement = storage.prepare(get_pointer<User>(3));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto user = storage.execute(statement);
            REQUIRE(user);
            REQUIRE(*user == User{3, "Maître Gims"});
        }
    }
    {
        auto statement = storage.prepare(get_pointer<User>(4));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto user = storage.execute(statement);
            REQUIRE(!user);
        }
    }
    {
        storage.replace(Visit{1, /*userId*/ 2, 1000});
        auto statement = storage.prepare(get_pointer<UserAndVisit>(2, 1));
        std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
        std::ignore = get<1>(static_cast<const decltype(statement)&>(statement));
        REQUIRE(get<0>(statement) == 2);
        REQUIRE(get<1>(statement) == 1);
        {
            auto userAndVisit = storage.execute(statement);
            REQUIRE(userAndVisit);
            REQUIRE(userAndVisit->userId == 2);
            REQUIRE(userAndVisit->visitId == 1);
            REQUIRE(userAndVisit->description == "Glad you came");
        }
        {
            get<0>(statement) = 3;
            auto userAndVisit = storage.execute(statement);
            REQUIRE(userAndVisit);
            REQUIRE(userAndVisit->userId == 3);
            REQUIRE(userAndVisit->visitId == 1);
            REQUIRE(userAndVisit->description == "Shine on");
        }
    }
    {
        auto id = 1;
        auto statement = storage.prepare(get_pointer<User>(std::ref(id)));
        REQUIRE(get<0>(statement) == id);
        REQUIRE(&get<0>(statement) == &id);
        {
            auto user = storage.execute(statement);
            REQUIRE(user);
            REQUIRE(*user == User{1, "Team BS"});
        }
        id = 2;
        REQUIRE(get<0>(statement) == id);
        REQUIRE(&get<0>(statement) == &id);
        {
            auto user = storage.execute(statement);
            REQUIRE(user);
            REQUIRE(*user == User{2, "Shy'm"});
        }
    }
}
