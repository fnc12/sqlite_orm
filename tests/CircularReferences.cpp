#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <filesystem>
#include <iostream>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;


TEST_CASE("circular dependency") {
    SECTION("bidirectional") {
        struct User {
            int id = 0;
            std::string name;
            int id_last_visit;
        };

        struct Visit {
            int id = 0;
            decltype(User::id) userId;
            long time = 0;
        };
        auto storage = make_storage("circular.sqlite",
            make_table("users",
                make_column("id", &User::id, primary_key(), autoincrement()),
                make_column("name", &User::name),
                make_column("last_visit", &User::id_last_visit),
                foreign_key(&User::id_last_visit).references(&Visit::userId)),
            make_table("visits",
                make_column("id", &Visit::id, primary_key(), autoincrement()),
                make_column("user_id", &Visit::userId),
                make_column("time", &Visit::time),
                foreign_key(&Visit::userId).references(&User::id)));

        auto ret = storage.sync_schema(true);
        for( auto r : ret) {
            std::cout << r.first << " " << r.second << std::endl;
        }
        namespace fs = std::filesystem;
        bool ok = fs::exists("circular.sqlite");
        REQUIRE(ok == true);
    }
}
