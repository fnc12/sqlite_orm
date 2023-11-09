#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <tuple>  //  std::tuple_size

#include "static_tests_common.h"

using namespace sqlite_orm;
using internal::is_compound_operator_v;
using std::tuple_size;

template<class Compound>
void runTest(Compound) {
    STATIC_REQUIRE(is_compound_operator_v<Compound>);
    STATIC_REQUIRE(tuple_size<typename Compound::expressions_tuple>::value == 3);
}

TEST_CASE("Compound operators") {
    runTest(union_(select(&User::id), select(&User::id), select(&Token::id)));
    runTest(union_all(select(&User::id), select(&User::id), select(&Token::id)));
    runTest(except(select(&User::id), select(&User::id), select(&Token::id)));
    runTest(intersect(select(&User::id), select(&User::id), select(&Token::id)));
}
