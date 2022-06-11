#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    struct A {};
    struct B {};
}

TEMPLATE_PRODUCT_TEST_CASE("tuple - empty member",
                           "[tuple]",
                           (mpl::tuple, mpl::uple),
                           ((int, A), (A, int), (A, int, B), (A, B, int), (int, A, B))) {
    STATIC_REQUIRE(sizeof(TestType) == sizeof(int));
}
