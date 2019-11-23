#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

struct User {
    int id = 0;
    std::string name;
};

TEST_CASE("count_tuple") {
    using internal::count_tuple;
    {
        auto t = std::make_tuple(where(is_equal(&User::id, 5)), limit(5), order_by(&User::name));
        auto whereCount = count_tuple<decltype(t), conditions::is_where>::value;
        REQUIRE(whereCount == 1);
    }
    {
        auto t = std::make_tuple(where(lesser_than(&User::id, 10)),
                                 where(greater_than(&User::id, 5)),
                                 group_by(&User::name));
        auto whereCount = count_tuple<decltype(t), conditions::is_where>::value;
        REQUIRE(whereCount == 2);
    }
    {
        auto t = std::make_tuple(group_by(&User::name), limit(10, offset(5)));
        auto whereCount = count_tuple<decltype(t), conditions::is_where>::value;
        REQUIRE(whereCount == 0);
    }
}
