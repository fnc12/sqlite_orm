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
        using T = decltype(t);
        static_assert(count_tuple<T, internal::is_where>::value == 1, "");
        static_assert(count_tuple<T, internal::is_group_by>::value == 0, "");
        static_assert(count_tuple<T, internal::is_order_by>::value == 1, "");
        static_assert(count_tuple<T, internal::is_limit>::value == 1, "");
    }
    {
        auto t = std::make_tuple(where(lesser_than(&User::id, 10)),
                                 where(greater_than(&User::id, 5)),
                                 group_by(&User::name));
        using T = decltype(t);
        static_assert(count_tuple<T, internal::is_where>::value == 2, "");
        static_assert(count_tuple<T, internal::is_group_by>::value == 1, "");
        static_assert(count_tuple<T, internal::is_order_by>::value == 0, "");
        static_assert(count_tuple<T, internal::is_limit>::value == 0, "");
    }
    {
        auto t = std::make_tuple(group_by(&User::name), limit(10, offset(5)));
        using T = decltype(t);
        static_assert(count_tuple<T, internal::is_where>::value == 0, "");
        static_assert(count_tuple<T, internal::is_group_by>::value == 1, "");
        static_assert(count_tuple<T, internal::is_order_by>::value == 0, "");
        static_assert(count_tuple<T, internal::is_limit>::value == 1, "");
    }
    {
        auto t =
            std::make_tuple(where(is_null(&User::name)), order_by(&User::id), multi_order_by(order_by(&User::name)));
        using T = decltype(t);
        static_assert(count_tuple<T, internal::is_where>::value == 1, "");
        static_assert(count_tuple<T, internal::is_group_by>::value == 0, "");
        static_assert(count_tuple<T, internal::is_order_by>::value == 2, "");
        static_assert(count_tuple<T, internal::is_limit>::value == 0, "");
    }
    {
        auto t = std::make_tuple(dynamic_order_by(make_storage("")));
        using T = decltype(t);
        static_assert(count_tuple<T, internal::is_where>::value == 0, "");
        static_assert(count_tuple<T, internal::is_group_by>::value == 0, "");
        static_assert(count_tuple<T, internal::is_order_by>::value == 1, "");
        static_assert(count_tuple<T, internal::is_limit>::value == 0, "");
    }
}
