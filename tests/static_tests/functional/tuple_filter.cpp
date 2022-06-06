#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;
using std::make_tuple;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };
}

TEST_CASE("tuple_filter") {
    using internal::filter_tuple_t;
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

TEST_CASE("count_tuple") {
    using internal::count_tuple;
    {
        auto t = make_tuple(where(is_equal(&User::id, 5)), limit(5), order_by(&User::name));
        using T = decltype(t);
        STATIC_REQUIRE(count_tuple<T, internal::is_where>::value == 1);
        STATIC_REQUIRE(count_tuple<T, internal::is_group_by>::value == 0);
        STATIC_REQUIRE(count_tuple<T, internal::is_order_by>::value == 1);
        STATIC_REQUIRE(count_tuple<T, internal::is_limit>::value == 1);
    }
    {
        auto t =
            make_tuple(where(lesser_than(&User::id, 10)), where(greater_than(&User::id, 5)), group_by(&User::name));
        using T = decltype(t);
        STATIC_REQUIRE(count_tuple<T, internal::is_where>::value == 2);
        STATIC_REQUIRE(count_tuple<T, internal::is_group_by>::value == 1);
        STATIC_REQUIRE(count_tuple<T, internal::is_order_by>::value == 0);
        STATIC_REQUIRE(count_tuple<T, internal::is_limit>::value == 0);
    }
    {
        auto t = make_tuple(group_by(&User::name), limit(10, offset(5)));
        using T = decltype(t);
        STATIC_REQUIRE(count_tuple<T, internal::is_where>::value == 0);
        STATIC_REQUIRE(count_tuple<T, internal::is_group_by>::value == 1);
        STATIC_REQUIRE(count_tuple<T, internal::is_order_by>::value == 0);
        STATIC_REQUIRE(count_tuple<T, internal::is_limit>::value == 1);
    }
    {
        auto t = make_tuple(where(is_null(&User::name)), order_by(&User::id), multi_order_by(order_by(&User::name)));
        using T = decltype(t);
        STATIC_REQUIRE(count_tuple<T, internal::is_where>::value == 1);
        STATIC_REQUIRE(count_tuple<T, internal::is_group_by>::value == 0);
        STATIC_REQUIRE(count_tuple<T, internal::is_order_by>::value == 2);
        STATIC_REQUIRE(count_tuple<T, internal::is_limit>::value == 0);
    }
    {
        auto t = make_tuple(dynamic_order_by(make_storage("")));
        using T = decltype(t);
        STATIC_REQUIRE(count_tuple<T, internal::is_where>::value == 0);
        STATIC_REQUIRE(count_tuple<T, internal::is_group_by>::value == 0);
        STATIC_REQUIRE(count_tuple<T, internal::is_order_by>::value == 1);
        STATIC_REQUIRE(count_tuple<T, internal::is_limit>::value == 0);
    }
}
