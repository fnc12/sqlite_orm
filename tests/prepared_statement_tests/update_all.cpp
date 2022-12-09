#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared update all") {
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

    {  //  by val
        auto statement = storage.prepare(update_all(set(assign(&User::name, conc(&User::name, "_")))));
        using Statement = decltype(statement);
        using Expression = Statement::expression_type;
        using SetTuple = internal::node_tuple<Expression>::set_tuple;
        using SetBind = internal::bindable_filter_t<SetTuple>;
        STATIC_REQUIRE(std::tuple_size<SetBind>::value == 1);
        {
            using Arg0 = std::tuple_element_t<0, SetBind>;
            STATIC_REQUIRE(std::is_same<Arg0, const char*>::value);
        }
        REQUIRE(strcmp(get<0>(statement), "_") == 0);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            storage.execute(statement);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_");
                expected.push_back("Shy'm_");
                expected.push_back("Maître Gims_");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
            get<0>(statement) = "123";
            storage.execute(statement);
            REQUIRE(strcmp(get<0>(statement), "123") == 0);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_123");
                expected.push_back("Shy'm_123");
                expected.push_back("Maître Gims_123");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
            auto statement2 = storage.prepare(
                update_all(set(c(&User::name) = c(&User::name) || "!"), where(like(&User::name, "T%"))));
            REQUIRE(strcmp(get<0>(statement2), "!") == 0);
            REQUIRE(strcmp(get<1>(statement2), "T%") == 0);
            storage.execute(statement2);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_123!");
                expected.push_back("Shy'm_123");
                expected.push_back("Maître Gims_123");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
            get<0>(statement2) = "@";
            get<1>(statement2) = "Sh%";
            REQUIRE(strcmp(get<0>(statement2), "@") == 0);
            REQUIRE(strcmp(get<1>(statement2), "Sh%") == 0);
            storage.execute(statement2);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_123!");
                expected.push_back("Shy'm_123@");
                expected.push_back("Maître Gims_123");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
        }
    }
    {  //  by ref
        std::string str = "_";
        auto statement = storage.prepare(update_all(set(assign(&User::name, conc(&User::name, std::ref(str))))));
        using Statement = decltype(statement);
        using Expression = Statement::expression_type;
        using SetTuple = internal::node_tuple<Expression>::set_tuple;
        using SetBind = internal::bindable_filter_t<SetTuple>;
        STATIC_REQUIRE(std::tuple_size<SetBind>::value == 1);
        {
            using Arg0 = std::tuple_element_t<0, SetBind>;
            STATIC_REQUIRE(std::is_same<Arg0, std::string>::value);
        }
        REQUIRE(get<0>(statement) == "_");
        REQUIRE(&get<0>(statement) == &str);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            storage.execute(statement);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_");
                expected.push_back("Shy'm_");
                expected.push_back("Maître Gims_");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
            str = "123";
            storage.execute(statement);
            REQUIRE(get<0>(statement) == "123");
            REQUIRE(&get<0>(statement) == &str);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_123");
                expected.push_back("Shy'm_123");
                expected.push_back("Maître Gims_123");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
            std::string name = "!";
            std::string pattern = "T%";
            auto statement2 = storage.prepare(update_all(set(c(&User::name) = c(&User::name) || std::ref(name)),
                                                         where(like(&User::name, std::ref(pattern)))));
            REQUIRE(get<0>(statement2) == "!");
            REQUIRE(&get<0>(statement2) == &name);
            REQUIRE(get<1>(statement2) == "T%");
            REQUIRE(&get<1>(statement2) == &pattern);
            storage.execute(statement2);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_123!");
                expected.push_back("Shy'm_123");
                expected.push_back("Maître Gims_123");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
            name = "@";
            pattern = "Sh%";
            REQUIRE(get<0>(statement2) == "@");
            REQUIRE(get<1>(statement2) == "Sh%");
            storage.execute(statement2);
            {
                auto names = storage.select(&User::name);
                std::vector<decltype(User::name)> expected;
                expected.push_back("Team BS_123!");
                expected.push_back("Shy'm_123@");
                expected.push_back("Maître Gims_123");
                REQUIRE_THAT(names, UnorderedEquals(expected));
            }
        }
    }
}
