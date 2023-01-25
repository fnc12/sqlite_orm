#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("Arithmetic operators result type") {
    STATIC_REQUIRE(
        std::is_same<internal::column_result_t<internal::storage_t<>, decltype(add(1, 2))>::type, double>::value);
    STATIC_REQUIRE(
        std::is_same<internal::column_result_t<internal::storage_t<>, decltype(sub(2, 1))>::type, double>::value);
    STATIC_REQUIRE(
        std::is_same<internal::column_result_t<internal::storage_t<>, decltype(mul(2, 3))>::type, double>::value);
    STATIC_REQUIRE(std::is_same<internal::column_result_t<internal::storage_t<>, decltype(sqlite_orm::div(2, 3))>::type,
                                double>::value);
}
