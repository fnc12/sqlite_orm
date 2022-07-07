#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>

using namespace sqlite_orm;

//
// This test checks that we can NOT construct a tuple holding array members,
// per the standard.
//

TEST_CASE("tuple - array elements") {
    STATIC_REQUIRE(!std::is_constructible<mpl::tuple<int[3]>, int[3]>{});
    STATIC_REQUIRE(!std::is_constructible<mpl::tuple<int[3], float[4]>, int[3], float[4]>{});
    STATIC_REQUIRE(!std::is_constructible<mpl::uple<int[3]>, int[3]>{});
    STATIC_REQUIRE(!std::is_constructible<mpl::uple<int[3], float[4]>, int[3], float[4]>{});
}
