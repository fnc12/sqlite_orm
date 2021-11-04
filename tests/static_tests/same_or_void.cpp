#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include <type_traits>  //  std::is_same
#include <string>  //  std::string

using namespace sqlite_orm;

TEST_CASE("same_or_void") {
    using internal::same_or_void;

    //  one argument
    static_assert(std::is_same<same_or_void<int>::type, int>::value, "int");
    static_assert(std::is_same<same_or_void<std::string>::type, std::string>::value, "std::string");
    static_assert(std::is_same<same_or_void<long>::type, long>::value, "long");

    //  two arguments
    static_assert(std::is_same<same_or_void<int, int>::type, int>::value, "int, int");
    static_assert(std::is_same<same_or_void<int, long>::type, void>::value, "int, long");
    static_assert(std::is_same<same_or_void<std::string, std::string>::type, std::string>::value,
                  "std::string, std::string");
    static_assert(std::is_same<same_or_void<std::string, short>::type, void>::value, "std::string, short");

    //  three arguments
    static_assert(std::is_same<same_or_void<int, int, int>::type, int>::value, "int, int, int");
    static_assert(std::is_same<same_or_void<long, long, long>::type, long>::value, "long, long, long");
    static_assert(std::is_same<same_or_void<int, int, long>::type, void>::value, "int, int, long");
    static_assert(std::is_same<same_or_void<long, int, int>::type, void>::value, "long, int, int");
    static_assert(std::is_same<same_or_void<long, int, long>::type, void>::value, "long, int, long");

    //  four arguments
    static_assert(std::is_same<same_or_void<int, int, int, int>::type, int>::value, "int, int, int, int");
    static_assert(std::is_same<same_or_void<long, long, long, long>::type, long>::value, "long, long, long, long");
    static_assert(std::is_same<same_or_void<int, int, int, long>::type, void>::value, "int, int, int, long");
}
