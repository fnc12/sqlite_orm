#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("has_type") {
    using empty_tuple_type = std::tuple<>;
    using tuple_type = std::tuple<int, char, std::string>;

    static_assert(internal::tuple_contains_type<std::string, tuple_type>::value, "");
    static_assert(!internal::tuple_contains_type<std::shared_ptr<int>, tuple_type>::value, "");
    static_assert(!internal::tuple_contains_type<std::string, empty_tuple_type>::value, "");
}