#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;
using internal::filter_tuple_t;

TEST_CASE("tuple_filter") {
    struct User {
        int id = 0;
    };
    SECTION("is_bindable") {
        using Arg = std::tuple<int, std::string, internal::where_t<bool>, internal::order_by_t<decltype(&User::id)>>;
        using Expected = std::tuple<int, std::string>;
        using ResultType = filter_tuple_t<Arg, internal::is_bindable>;
        STATIC_REQUIRE(std::is_same<ResultType, Expected>::value);
    }
    SECTION("is_column") {
        auto column = make_column({}, &User::id);
        using Column = decltype(column);
        using OrderBy = internal::order_by_t<decltype(&User::id)>;
        using Unique = decltype(unique(&User::id));
        using Arg = std::tuple<Column, OrderBy, Unique>;
        using Expected = std::tuple<Column>;
        using ResultType = filter_tuple_t<Arg, internal::is_column>;
        STATIC_REQUIRE(std::is_same<ResultType, Expected>::value);
    }
}
