#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared get all") {
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

    {
        auto statement = storage.prepare(get_all<User>());
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<User> expected;
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    {
        auto statement = storage.prepare(get_all<User>(where(lesser_than(&User::id, 3))));
        {
            using Statement = decltype(statement);
            using ExpressionType = Statement::expression_type;
            using NodeTuple = internal::node_tuple<ExpressionType>::type;
            static_assert(std::tuple_size<NodeTuple>::value == 2, "");
            {
                using Arg0 = std::tuple_element<0, NodeTuple>::type;
                static_assert(std::is_same<Arg0, decltype(&User::id)>::value, "");
            }
            {
                using Arg1 = std::tuple_element<1, NodeTuple>::type;
                static_assert(std::is_same<Arg1, int>::value, "");
            }
        }
        REQUIRE(get<0>(statement) == 3);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<User> expected;
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    {
        auto statement =
            storage.prepare(get_all<User>(where(lesser_or_equal(&User::id, 1) and is_equal(&User::name, "Team BS"))));
        REQUIRE(get<0>(statement) == 1);
        REQUIRE(strcmp(get<1>(statement), "Team BS") == 0);
    }
    {
        auto statement = storage.prepare(get_all<User>(
            where(lesser_or_equal(&User::id, 2) and (like(&User::name, "T%") or glob(&User::name, "*S")))));
        REQUIRE(get<0>(statement) == 2);
        REQUIRE(strcmp(get<1>(statement), "T%") == 0);
        REQUIRE(strcmp(get<2>(statement), "*S") == 0);
    }
    {
        auto statement = storage.prepare(get_all<User>(where(lesser_than(&User::id, 2))));
        std::vector<User> expected;
        REQUIRE(get<0>(statement) == 2);
        expected.push_back(User{1, "Team BS"});
        {
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }

        get<0>(statement) = 3;
        REQUIRE(get<0>(statement) == 3);
        expected.push_back(User{2, "Shy'm"});
        {
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }

        get<0>(statement) = 4;
        REQUIRE(get<0>(statement) == 4);
        expected.push_back(User{3, "Maître Gims"});
        {
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
}
