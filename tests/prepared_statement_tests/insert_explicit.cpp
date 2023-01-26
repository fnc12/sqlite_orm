#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared insert explicit") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::ContainsSubstring;
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

    SECTION("user two columns") {
        User user{5, "Eminem"};
        SECTION("by ref") {
            auto statement = storage.prepare(insert(std::ref(user), columns(&User::id, &User::name)));
            std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
            {
                auto insertedId = storage.execute(statement);
                REQUIRE(insertedId == user.id);
                REQUIRE(storage.count<User>(where(is_equal(&User::name, "Eminem"))) == 1);
            }
            {
                user.id = 6;
                user.name = "Nate Dogg";
                auto insertedId = storage.execute(statement);
                REQUIRE(insertedId == 6);
                REQUIRE(storage.count<User>(where(is_equal(&User::name, "Nate Dogg"))) == 1);
                REQUIRE(&user == &get<0>(statement));
            }
        }
        SECTION("by val") {
            auto statement = storage.prepare(insert(user, columns(&User::id, &User::name)));
            {
                auto insertedId = storage.execute(statement);
                REQUIRE(insertedId == user.id);
                REQUIRE(storage.count<User>(where(is_equal(&User::name, "Eminem"))) == 1);
            }
            {
                user.id = 6;
                user.name = "Nate Dogg";
                REQUIRE_THROWS_WITH(storage.execute(statement), ContainsSubstring("constraint failed"));

                get<0>(statement) = user;
                auto insertedId = storage.execute(statement);
                REQUIRE(insertedId == user.id);
                REQUIRE(storage.count<User>(where(is_equal(&User::name, "Nate Dogg"))) == 1);
                REQUIRE(&user != &get<0>(statement));
            }
        }
    }
    SECTION("user id column only") {
        User user{4, "Eminem"};
        auto statement = storage.prepare(insert(user, columns(&User::name)));
        auto insertedId = storage.execute(statement);
        REQUIRE(insertedId == user.id);
    }
    SECTION("visit") {
        {
            Visit visit{1, 1, 100000};
            auto statement = storage.prepare(insert(visit, columns(&Visit::id, &Visit::userId, &Visit::time)));
            auto insertedId = storage.execute(statement);
            REQUIRE(insertedId == visit.id);
        }
        {
            Visit visit{2, 1, defaultVisitTime + 1};  //  time must differ
            auto statement = storage.prepare(insert(visit, columns(&Visit::id, &Visit::userId)));
            auto insertedId = storage.execute(statement);
            REQUIRE(insertedId == visit.id);
            auto insertedVisit = storage.get<Visit>(insertedId);
            REQUIRE(insertedVisit.id == visit.id);
            REQUIRE(insertedVisit.userId == visit.userId);
            REQUIRE(insertedVisit.time == defaultVisitTime);
        }
        {
            Visit visit{-1, 2, defaultVisitTime + 2};
            auto statement = storage.prepare(insert(visit, columns(&Visit::userId)));
            auto insertedId = storage.execute(statement);
            REQUIRE(insertedId == 3);
            auto insertedVisit = storage.get<Visit>(insertedId);
            REQUIRE(insertedVisit.id == 3);
            REQUIRE(insertedVisit.userId == visit.userId);
            REQUIRE(insertedVisit.time == defaultVisitTime);
        }
    }
}
