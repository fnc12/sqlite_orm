#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Cast") {
    struct Student {
        int id;
        float scoreFloat;
        std::string scoreString;
    };

    auto storage = make_storage("",
                                make_table("students",
                                           make_column("id", &Student::id, primary_key()),
                                           make_column("score_float", &Student::scoreFloat),
                                           make_column("score_str", &Student::scoreString)));
    storage.sync_schema();

    storage.replace(Student{1, 10.1f, "14.5"});

    {
        auto rows = storage.select(columns(cast<int>(&Student::scoreFloat), cast<int>(&Student::scoreString)));
        REQUIRE(rows.size() == 1);
        auto& row = rows.front();
        REQUIRE(std::get<0>(row) == 10);
        REQUIRE(std::get<1>(row) == 14);
    }
    {
        auto rows = storage.select(cast<std::string>(5));
        REQUIRE(rows.size() == 1);
        auto& row = rows.front();
        REQUIRE(row == "5");
    }
}
