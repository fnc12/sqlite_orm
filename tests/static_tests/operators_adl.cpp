#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using sqlite_orm::c;
using sqlite_orm::internal::binary_operator;
using sqlite_orm::internal::greater_or_equal_t;
using sqlite_orm::internal::greater_than_t;
using sqlite_orm::internal::is_equal_t;
using sqlite_orm::internal::is_not_equal_t;
using sqlite_orm::internal::lesser_or_equal_t;
using sqlite_orm::internal::lesser_than_t;
using sqlite_orm::polyfill::is_specialization_of_v;

TEST_CASE("ADL and expression operators") {
    struct User {
        int id;
    };

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) < 42), lesser_than_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 < c(&User::id)), lesser_than_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) <= 42), lesser_or_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 <= c(&User::id)), lesser_or_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) > 42), greater_than_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 > c(&User::id)), greater_than_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) >= 42), greater_or_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 >= c(&User::id)), greater_or_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) == 42), is_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 == c(&User::id)), is_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) != 42), is_not_equal_t>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 != c(&User::id)), is_not_equal_t>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) || 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 || c(&User::id)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) || c(&User::id)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) + 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 + c(&User::id)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) + c(&User::id)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) - 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 - c(&User::id)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) - c(&User::id)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) * 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 * c(&User::id)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) * c(&User::id)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) / 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 / c(&User::id)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) / c(&User::id)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) / 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 / c(&User::id)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) / c(&User::id)), binary_operator>);

    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) % 42), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(42 % c(&User::id)), binary_operator>);
    STATIC_REQUIRE(is_specialization_of_v<decltype(c(&User::id) % c(&User::id)), binary_operator>);
}
