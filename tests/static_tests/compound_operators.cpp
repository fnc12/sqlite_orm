#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "static_tests_common.h"

using namespace sqlite_orm;

TEST_CASE("Compound operators") {
    auto unionValue = union_(select(&User::id), select(&Token::id));
    STATIC_REQUIRE(internal::is_base_of_template<decltype(unionValue), internal::compound_operator>::value);
    auto unionAllValue = union_all(select(&User::id), select(&Token::id));
    STATIC_REQUIRE(internal::is_base_of_template<decltype(unionAllValue), internal::compound_operator>::value);
    auto exceptValue = except(select(&User::id), select(&Token::id));
    STATIC_REQUIRE(internal::is_base_of_template<decltype(exceptValue), internal::compound_operator>::value);
    auto intersectValue = intersect(select(&User::id), select(&Token::id));
    STATIC_REQUIRE(internal::is_base_of_template<decltype(intersectValue), internal::compound_operator>::value);
}
