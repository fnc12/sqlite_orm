#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include <type_traits>  //  std::is_same
#include <string>  //  std::string
#include <tuple>  //  std::tuple

using namespace sqlite_orm;

TEST_CASE("same_or_void") {
    using internal::common_type_of_t;
    using internal::same_or_void_t;

    //  one argument
    STATIC_REQUIRE(std::is_same<same_or_void_t<int>, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<std::string>, std::string>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<long>, long>::value);

    //  two arguments
    STATIC_REQUIRE(std::is_same<same_or_void_t<int, int>, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<int, long>, void>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<std::string, std::string>, std::string>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<std::string, short>, void>::value);

    //  three arguments
    STATIC_REQUIRE(std::is_same<same_or_void_t<int, int, int>, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<long, long, long>, long>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<int, int, long>, void>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<long, int, int>, void>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<long, int, long>, void>::value);

    //  four arguments
    STATIC_REQUIRE(std::is_same<same_or_void_t<int, int, int, int>, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<long, long, long, long>, long>::value);
    STATIC_REQUIRE(std::is_same<same_or_void_t<int, int, int, long>, void>::value);

    //  type pack, e.g. tuple
    STATIC_REQUIRE(std::is_same<common_type_of_t<std::tuple<int, int>>, int>::value);
    STATIC_REQUIRE(std::is_same<common_type_of_t<std::tuple<int, long>>, long>::value);
    STATIC_REQUIRE(
        std::is_same<polyfill::detected_t<common_type_of_t, std::tuple<int, const char*>>, polyfill::nonesuch>::value);
}
