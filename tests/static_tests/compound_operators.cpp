#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "static_tests_common.h"

using namespace sqlite_orm;

TEST_CASE("Compound operators") {
    auto unionValue = union_(select(&User::id), select(&Token::id));
    static_assert(internal::is_base_of_template<decltype(unionValue), internal::compound_operator>::value,
                  "union must be base of compound_operator");
    auto exceptValue = except(select(&User::id), select(&Token::id));
    static_assert(internal::is_base_of_template<decltype(exceptValue), internal::compound_operator>::value,
                  "except must be base of compound_operator");
}
