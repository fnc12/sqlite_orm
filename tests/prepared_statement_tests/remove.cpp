#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared remove") {
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

    SECTION("by val") {
        {
            auto statement = storage.prepare(remove<User>(1));
            std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
            REQUIRE(get<0>(statement) == 1);
            testSerializing(statement);
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) != nullptr);
            REQUIRE(storage.get_pointer<User>(3) != nullptr);
            REQUIRE(storage.count<User>() == 2);
        }
        {
            auto statement = storage.prepare(remove<User>(2));
            REQUIRE(get<0>(statement) == 2);
            testSerializing(statement);
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) == nullptr);
            REQUIRE(storage.get_pointer<User>(3) != nullptr);
            REQUIRE(storage.count<User>() == 1);

            get<0>(statement) = 3;
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) == nullptr);
            REQUIRE(storage.get_pointer<User>(3) == nullptr);
            REQUIRE(storage.count<User>() == 0);
        }
    }
    SECTION("be ref") {
        auto id = 1;
        auto statement = storage.prepare(remove<User>(std::ref(id)));
        REQUIRE(get<0>(statement) == id);
        REQUIRE(&get<0>(statement) == &id);
        testSerializing(statement);
        storage.execute(statement);
        REQUIRE(storage.get_pointer<User>(1) == nullptr);
        REQUIRE(storage.get_pointer<User>(2) != nullptr);
        REQUIRE(storage.get_pointer<User>(3) != nullptr);

        id = 2;
        REQUIRE(get<0>(statement) == id);
        REQUIRE(&get<0>(statement) == &id);
        storage.execute(statement);
        REQUIRE(storage.get_pointer<User>(1) == nullptr);
        REQUIRE(storage.get_pointer<User>(2) == nullptr);
        REQUIRE(storage.get_pointer<User>(3) != nullptr);

        get<0>(statement) = 3;
        REQUIRE(id == 3);
        REQUIRE(get<0>(statement) == 3);
        REQUIRE(&get<0>(statement) == &id);
        storage.execute(statement);
        REQUIRE(storage.get_pointer<User>(1) == nullptr);
        REQUIRE(storage.get_pointer<User>(2) == nullptr);
        REQUIRE(storage.get_pointer<User>(3) == nullptr);
        REQUIRE(storage.count<User>() == 0);
    }
}
