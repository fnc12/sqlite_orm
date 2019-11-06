#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Simple query") {
    auto storage = make_storage("");
    {
        //  SELECT 1
        auto one = storage.select(1);
        REQUIRE(one.size() == 1);
        REQUIRE(one.front() == 1);
    }
    {
        //  SELECT 'ototo'
        auto ototo = storage.select("ototo");
        REQUIRE(ototo.size() == 1);
        REQUIRE(ototo.front() == "ototo");
    }
    {
        //  SELECT 1 + 1
        auto two = storage.select(c(1) + 1);
        REQUIRE(two.size() == 1);
        REQUIRE(two.front() == 2);

        auto twoAgain = storage.select(add(1, 1));
        REQUIRE(two == twoAgain);
    }
    {
        //  SELECT 10 / 5, 2 * 4
        auto math = storage.select(columns(sqlite_orm::div(10, 5), mul(2, 4)));
        REQUIRE(math.size() == 1);
        REQUIRE(math.front() == std::make_tuple(2, 8));
    }
    {
        //  SELECT 1, 2
        auto twoRows = storage.select(columns(1, 2));
        REQUIRE(twoRows.size() == 1);
        REQUIRE(std::get<0>(twoRows.front()) == 1);
        REQUIRE(std::get<1>(twoRows.front()) == 2);
    }
    {
        //  SELECT 1, 2
        //  UNION ALL
        //  SELECT 3, 4;
        auto twoRowsUnion = storage.select(union_all(select(columns(1, 2)), select(columns(3, 4))));
        REQUIRE(twoRowsUnion.size() == 2);
        REQUIRE(twoRowsUnion[0] == std::make_tuple(1, 2));
        REQUIRE(twoRowsUnion[1] == std::make_tuple(3, 4));
    }
}
