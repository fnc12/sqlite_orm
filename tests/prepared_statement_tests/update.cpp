#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared update") {
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

    User user{2, "Stromae"};
    SECTION("by ref") {
        auto statement = storage.prepare(update(std::ref(user)));
        REQUIRE(get<0>(statement) == user);
        std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
        REQUIRE(&get<0>(statement) == &user);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            storage.execute(statement);
            REQUIRE(storage.get<User>(2) == user);
            {
                auto names = storage.select(&User::name);
                REQUIRE(find(names.begin(), names.end(), "Shy'm") == names.end());
                REQUIRE(find(names.begin(), names.end(), "Stromae") != names.end());
            }
            user.name = "Sia";
            storage.execute(statement);
            REQUIRE(storage.get<User>(2) == user);
            {
                auto names = storage.select(&User::name);
                REQUIRE(find(names.begin(), names.end(), "Shy'm") == names.end());
                REQUIRE(find(names.begin(), names.end(), "Sia") != names.end());
            }
        }
    }
    SECTION("by val") {
        auto statement = storage.prepare(update(user));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            storage.execute(statement);
            REQUIRE(storage.get<User>(2) == user);
            {
                auto names = storage.select(&User::name);
                REQUIRE(find(names.begin(), names.end(), "Shy'm") == names.end());
                REQUIRE(find(names.begin(), names.end(), "Stromae") != names.end());
            }

            //  try to change original user's name and perform the query. We expect that nothing will change
            user.name = "Sia";
            storage.execute(statement);
            {
                auto names = storage.select(&User::name);
                REQUIRE(find(names.begin(), names.end(), "Sia") == names.end());
                REQUIRE(find(names.begin(), names.end(), "Stromae") != names.end());
            }

            //  now let's change statement's user's name. This time it musk work!
            get<0>(statement).name = "Sia";
            storage.execute(statement);
            REQUIRE(storage.count<User>(where(is_equal(&User::name, "Sia"))) == 1);

            get<0>(statement) = {user.id, "Paris"};
            storage.execute(statement);
            REQUIRE(storage.count<User>(where(is_equal(&User::name, "Paris"))) == 1);
        }
    }
}
