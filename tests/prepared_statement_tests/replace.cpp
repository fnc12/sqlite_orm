#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared replace") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;

    const int defaultVisitTime = 50;

    auto filename = "prepared.sqlite";
    remove(filename);
    auto storage = make_storage(filename,
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key().autoincrement()),
                                           make_column("name", &User::name, default_value(""))),
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

    std::vector<User> expected;
    User user;
    SECTION("raw") {
        SECTION("values") {
            SECTION("one user") {
                SECTION("statement") {
                    auto statement = storage.prepare(
                        replace(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, "Ellie"))));
                    storage.execute(statement);
                    REQUIRE(get<0>(statement) == 1);
                    REQUIRE(::strcmp(get<1>(statement), "Ellie") == 0);
                }
                SECTION("no statement") {
                    storage.replace(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, "Ellie")));
                }
                expected.push_back({1, "Ellie"});
            }
            SECTION("two users") {
                SECTION("statement") {
                    auto statement =
                        storage.prepare(replace(into<User>(),
                                                columns(&User::id, &User::name),
                                                values(std::make_tuple(1, "Ellie"), std::make_tuple(5, "Calvin"))));
                    storage.execute(statement);
                    REQUIRE(get<0>(statement) == 1);
                    REQUIRE(::strcmp(get<1>(statement), "Ellie") == 0);
                    REQUIRE(get<2>(statement) == 5);
                    REQUIRE(::strcmp(get<3>(statement), "Calvin") == 0);
                }
                SECTION("no statement") {
                    storage.replace(into<User>(),
                                    columns(&User::id, &User::name),
                                    values(std::make_tuple(1, "Ellie"), std::make_tuple(5, "Calvin")));
                }
                expected.push_back({1, "Ellie"});
                expected.push_back({5, "Calvin"});
            }
        }
        SECTION("default values") {
            SECTION("statement") {
                auto statement = storage.prepare(replace(into<User>(), default_values()));
                storage.execute(statement);
            }
            SECTION("no statement") {
                storage.replace(into<User>(), default_values());
            }
            expected.push_back({1, ""});
        }
        SECTION("select") {
            SECTION("statement") {
                auto statement = storage.prepare(replace(into<User>(), select(columns(5, "Carma"))));
                storage.execute(statement);
            }
            SECTION("no statement") {
                storage.replace(into<User>(), select(columns(5, "Carma")));
            }
            expected.push_back({5, "Carma"});
        }
    }
    SECTION("crud") {
        storage.replace(User{1, "Team BS"});
        storage.replace(User{2, "Shy'm"});
        storage.replace(User{3, "Maître Gims"});

        storage.replace(UserAndVisit{2, 1, "Glad you came"});
        storage.replace(UserAndVisit{3, 1, "Shine on"});
        SECTION("by ref existing") {
            user = {1, "Stromae"};
            expected.push_back(User{1, "Stromae"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            auto statement = storage.prepare(replace(std::ref(user)));
            storage.execute(statement);

            std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
            REQUIRE(user == get<0>(statement));
            REQUIRE(&user == &get<0>(statement));
        }
        SECTION("by ref new") {
            user = {4, "Stromae"};
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            expected.push_back(user);
            auto statement = storage.prepare(replace(std::ref(user)));
            storage.execute(statement);
            auto rows = storage.get_all<User>();
            REQUIRE_THAT(rows, UnorderedEquals(expected));

            user = {5, "LP"};
            expected.push_back(user);
            storage.execute(statement);

            REQUIRE(user == get<0>(statement));
            REQUIRE(&user == &get<0>(statement));
        }
        SECTION("by val existing") {
            SECTION("straight assign") {
                user = {1, "Stromae"};
            }
            expected.push_back(User{1, "Stromae"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            auto statement = storage.prepare(replace(user));
            REQUIRE(&user != &get<0>(statement));
            SECTION("assign with get") {
                get<0>(statement) = {1, "Stromae"};
            }
            storage.execute(statement);
        }
        SECTION("by val new") {
            user = {4, "Stromae"};
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            expected.push_back(user);
            auto statement = storage.prepare(replace(user));
            REQUIRE(&user != &get<0>(statement));
            storage.execute(statement);
            auto rows = storage.get_all<User>();
            REQUIRE_THAT(rows, UnorderedEquals(expected));

            user = {5, "LP"};
            storage.execute(statement);
        }
    }
    auto rows = storage.get_all<User>();
    REQUIRE_THAT(rows, UnorderedEquals(expected));
}
