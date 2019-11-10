#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared select") {
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

    {//  one simple argument
     {//  by val
      auto statement = storage.prepare(select(10));
    REQUIRE(get<0>(statement) == 10);
    {
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({10}));
    }
    get<0>(statement) = 20;
    REQUIRE(get<0>(statement) == 20);
    {
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({20}));
    }
}
{  //  by ref
    auto id = 10;
    auto statement = storage.prepare(select(std::ref(id)));
    REQUIRE(get<0>(statement) == 10);
    REQUIRE(&get<0>(statement) == &id);
    {
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({10}));
    }
    id = 20;
    REQUIRE(get<0>(statement) == 20);
    REQUIRE(&get<0>(statement) == &id);
    {
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({20}));
    }
}
}
{//  two simple arguments
 {//  by val
  auto statement = storage.prepare(select(columns("ototo", 25)));
REQUIRE(strcmp(get<0>(statement), "ototo") == 0);
REQUIRE(get<1>(statement) == 25);
{
    auto rows = storage.execute(statement);
    REQUIRE(rows.size() == 1);
    auto &row = rows.front();
    REQUIRE(get<0>(row) == "ototo");
    REQUIRE(get<1>(row) == 25);
}
get<0>(statement) = "Rock";
get<1>(statement) = -15;
{
    auto rows = storage.execute(statement);
    REQUIRE(rows.size() == 1);
    auto &row = rows.front();
    REQUIRE(get<0>(row) == "Rock");
    REQUIRE(get<1>(row) == -15);
}
}
{  //  by ref
    std::string ototo = "ototo";
    auto id = 25;
    auto statement = storage.prepare(select(columns(std::ref(ototo), std::ref(id))));
    REQUIRE(get<0>(statement) == "ototo");
    REQUIRE(&get<0>(statement) == &ototo);
    REQUIRE(get<1>(statement) == 25);
    REQUIRE(&get<1>(statement) == &id);
    {
        auto rows = storage.execute(statement);
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(get<0>(row) == "ototo");
        REQUIRE(get<1>(row) == 25);
    }
    ototo = "Rock";
    REQUIRE(get<0>(statement) == ototo);
    REQUIRE(&get<0>(statement) == &ototo);
    id = -15;
    REQUIRE(get<1>(statement) == id);
    REQUIRE(&get<1>(statement) == &id);
    {
        auto rows = storage.execute(statement);
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(get<0>(row) == "Rock");
        REQUIRE(get<1>(row) == -15);
    }
}
}
{//  three columns, aggregate func and where
 {//  by val
  auto statement =
      storage.prepare(select(columns(5.0, &User::id, count(&User::name)), where(lesser_than(&User::id, 10))));
REQUIRE(get<0>(statement) == 5.0);
REQUIRE(get<1>(statement) == 10);
{
    auto rows = storage.execute(statement);
    REQUIRE(rows.size() == 1);
    auto &row = rows.front();
    REQUIRE(get<0>(row) == 5.0);
    REQUIRE(get<2>(row) == 3);
}
get<0>(statement) = 4;
get<1>(statement) = 2;
{
    auto rows = storage.execute(statement);
    REQUIRE(rows.size() == 1);
    auto &row = rows.front();
    REQUIRE(get<0>(row) == 4.0);
    REQUIRE(get<2>(row) == 1);
}
}
{  //  by ref
    auto first = 5.0;
    auto id = 10;
    auto statement = storage.prepare(
        select(columns(std::ref(first), &User::id, count(&User::name)), where(lesser_than(&User::id, std::ref(id)))));
    REQUIRE(get<0>(statement) == 5.0);
    REQUIRE(&get<0>(statement) == &first);
    REQUIRE(get<1>(statement) == 10);
    REQUIRE(&get<1>(statement) == &id);
    {
        auto rows = storage.execute(statement);
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(get<0>(row) == 5.0);
        REQUIRE(get<2>(row) == 3);
    }
    first = 4;
    REQUIRE(&get<0>(statement) == &first);
    id = 2;
    {
        auto rows = storage.execute(statement);
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(get<0>(row) == 4.0);
        REQUIRE(get<2>(row) == 1);
    }
}
}
{
    for(auto i = 0; i < 2; ++i) {
        auto statement = storage.prepare(select(&User::id));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals<int>({1, 2, 3}));
        }
    }
}
{
    for(auto i = 0; i < 2; ++i) {
        auto statement = storage.prepare(select(&User::name, order_by(&User::id)));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals<std::string>({"Team BS", "Shy'm", "Maître Gims"}));
        }
    }
}
{{//  by val
  auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5)));
REQUIRE(get<0>(statement) == 5);
testSerializing(statement);
SECTION("nothing") {
    //..
}
SECTION("execute") {
    auto rows = storage.execute(statement);
    REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
}
}
{  //  by ref
    auto len = 5;
    auto statement = storage.prepare(select(&User::id, where(length(&User::name) > std::ref(len))));
    REQUIRE(get<0>(statement) == len);
    REQUIRE(&get<0>(statement) == &len);
    testSerializing(statement);
    SECTION("nothing") {
        //..
    }
    SECTION("execute") {
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
    }
}
}
{{//  by val
  auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5 and like(&User::name, "T%"))));
REQUIRE(get<0>(statement) == 5);
REQUIRE(strcmp(get<1>(statement), "T%") == 0);
testSerializing(statement);
SECTION("nothing") {
    //..
}
SECTION("execute") {
    auto rows = storage.execute(statement);
    REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
}
}
{  //  by ref
    auto len = 5;
    std::string pattern = "T%";
    auto statement = storage.prepare(
        select(&User::id, where(length(&User::name) > std::ref(len) and like(&User::name, std::ref(pattern)))));
    REQUIRE(get<0>(statement) == len);
    REQUIRE(&get<0>(statement) == &len);
    REQUIRE(get<1>(statement) == pattern);
    REQUIRE(&get<1>(statement) == &pattern);
    testSerializing(statement);
    SECTION("nothing") {
        //..
    }
    SECTION("execute") {
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
    }
}
}
{
    auto statement = storage.prepare(select(columns(&User::id, &User::name)));
    testSerializing(statement);
    SECTION("nothing") {
        //..
    }
    SECTION("execute") {
        auto rows = storage.execute(statement);
        std::vector<std::tuple<int, std::string>> expected;
        expected.push_back(std::make_tuple(1, "Team BS"));
        expected.push_back(std::make_tuple(2, "Shy'm"));
        expected.push_back(std::make_tuple(3, "Maître Gims"));
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
}
{
    {  //  by val
        auto statement = storage.prepare(
            select(columns(&User::name, &User::id), where(is_equal(mod(&User::id, 2), 0)), order_by(&User::name)));
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<std::tuple<std::string, int>> expected;
            expected.push_back(std::make_tuple("Shy'm", 2));
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    {  //  by ref
        auto m = 2;
        auto v = 0;
        auto statement = storage.prepare(select(columns(&User::name, &User::id),
                                                where(is_equal(mod(&User::id, std::ref(m)), std::ref(v))),
                                                order_by(&User::name)));
        testSerializing(statement);
        REQUIRE(get<0>(statement) == m);
        REQUIRE(&get<0>(statement) == &m);
        REQUIRE(get<1>(statement) == v);
        REQUIRE(&get<1>(statement) == &v);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<std::tuple<std::string, int>> expected;
            expected.push_back(std::make_tuple("Shy'm", 2));
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
}
}
