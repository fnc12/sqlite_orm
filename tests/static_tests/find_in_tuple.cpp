#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

#include "static_tests_common.h"

using namespace sqlite_orm;

TEST_CASE("find_in_tuple") {
    using namespace internal;
    using tuple = std::tuple<into_t<User>, columns_t<decltype(&User::id)>>;
    {
        using found = find_in_tuple<tuple, is_into>::type;
        STATIC_REQUIRE(std::is_same<found, into_t<User>>::value);
    }
    {
        using found = find_in_tuple<tuple, is_columns>::type;
        STATIC_REQUIRE(std::is_same<found, columns_t<decltype(&User::id)>>::value);
    }
    {
        using found = find_in_tuple<tuple, is_column>::type;
        STATIC_REQUIRE(std::is_same<found, void>::value);
    }
    {
        using found = find_in_tuple<tuple, is_primary_key>::type;
        STATIC_REQUIRE(std::is_same<found, void>::value);
    }
}
