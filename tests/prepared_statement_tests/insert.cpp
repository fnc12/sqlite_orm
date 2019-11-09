#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared insert") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;

    const int defaultVisitTime = 50;

    auto filename = "prepared.sqlite";
    remove(filename);
    auto storage = make_storage(filename,
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key(), autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key(), autoincrement()),
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

    User user{0, "Stromae"};
    SECTION("by ref") {
        auto statement = storage.prepare(insert(std::ref(user)));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto insertedId = storage.execute(statement);
            {
                auto rows = storage.get_all<User>();
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                expected.push_back(User{3, "Maître Gims"});
                expected.push_back(User{4, "Stromae"});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
            REQUIRE(insertedId == 4);
            user.name = "Sia";
            std::ignore = get<0>(static_cast<const decltype(statement) &>(statement));
            REQUIRE(get<0>(statement) == user);
            REQUIRE(&get<0>(statement) == &user);
            insertedId = storage.execute(statement);
            {
                auto rows = storage.get_all<User>();
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                expected.push_back(User{3, "Maître Gims"});
                expected.push_back(User{4, "Stromae"});
                expected.push_back(User{5, "Sia"});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
            REQUIRE(insertedId == 5);
        }
    }
    SECTION("by val") {
        auto statement = storage.prepare(insert(user));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto insertedId = storage.execute(statement);
            {
                auto rows = storage.get_all<User>();
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                expected.push_back(User{3, "Maître Gims"});
                expected.push_back(User{4, "Stromae"});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
            REQUIRE(insertedId == 4);
            user.name = "Sia";
            insertedId = storage.execute(statement);
            {
                auto rows = storage.get_all<User>();
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                expected.push_back(User{3, "Maître Gims"});
                expected.push_back(User{4, "Stromae"});
                expected.push_back(User{5, "Stromae"});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
            REQUIRE(insertedId == 5);

            get<0>(statement).name = "Sia";
            insertedId = storage.execute(statement);
            {
                auto rows = storage.get_all<User>();
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                expected.push_back(User{3, "Maître Gims"});
                expected.push_back(User{4, "Stromae"});
                expected.push_back(User{5, "Stromae"});
                expected.push_back(User{6, "Sia"});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
    }
}
