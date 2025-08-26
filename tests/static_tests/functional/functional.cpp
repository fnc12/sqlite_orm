#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include <type_traits>  //  std::true_type

using namespace sqlite_orm;

TEST_CASE("always_default") {
    STATIC_REQUIRE(internal::always_default<std::true_type>()());
}

// https://rextester.com/WTEWN51158
// THE BROKEN CASE, Fixed In: Visual Studio 2019 version 16.0 Preview 5
// It appears that when the base base class in question has more than 1 template parameter
// and the second template parameter is not varadic, the dudction in above in [A] is
// unable to deduce Ts... This works in both gcc and clang
template<typename first, typename second>
struct basea {};

// WORKING with 1 template parameter
template<typename first>
struct baseb {};

// WORKING if the second parameter is varadic
template<typename first, typename... seconds>
struct basec {};

TEST_CASE("is_base_template_of") {
    struct testa : basea<int, int> {};
    struct testb : baseb<int> {};
    struct testc : basec<int, int, float, double> {};

    STATIC_REQUIRE(internal::is_base_template_of<basea, testa>::value);
    STATIC_REQUIRE(internal::is_base_template_of<baseb, testb>::value);
    STATIC_REQUIRE(internal::is_base_template_of<basec, testc>::value);
}
