#include <catch2/catch_all.hpp>
#include <memory>  //  std::unique_ptr, std::shared_ptr
#include <optional>  //  std::optional
#include <sqlite_orm/sqlite_orm.h>
#include <string>  //  std::string
#include <vector>  //  std::vector

using namespace sqlite_orm;
using internal::is_integer_mapped;
using internal::is_integer_mapped_v;

namespace {
    struct CustomInteger {};
    struct CustomText {};
    struct Unmapped {};
}

namespace sqlite_orm {
    template<>
    struct type_printer<CustomInteger> : integer_printer {};
    template<>
    struct type_printer<CustomText> : text_printer {};
}

TEST_CASE("is_integer_mapped") {
    STATIC_REQUIRE(is_integer_mapped_v<bool>);
    STATIC_REQUIRE(is_integer_mapped_v<char>);
    STATIC_REQUIRE(is_integer_mapped_v<signed char>);
    STATIC_REQUIRE(is_integer_mapped_v<unsigned char>);
    STATIC_REQUIRE(is_integer_mapped_v<int>);
    STATIC_REQUIRE(is_integer_mapped_v<long long>);
    STATIC_REQUIRE(is_integer_mapped_v<unsigned long long>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<wchar_t>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<char16_t>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<char32_t>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<float>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<double>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<std::string>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<std::vector<char>>);
    STATIC_REQUIRE(is_integer_mapped_v<std::unique_ptr<int>>);
    STATIC_REQUIRE(is_integer_mapped_v<std::shared_ptr<int>>);
    STATIC_REQUIRE(is_integer_mapped_v<std::optional<int>>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<std::optional<std::string>>);
    STATIC_REQUIRE(is_integer_mapped_v<CustomInteger>);
    STATIC_REQUIRE(is_integer_mapped_v<std::optional<CustomInteger>>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<CustomText>);
    STATIC_REQUIRE_FALSE(is_integer_mapped_v<Unmapped>);
    STATIC_REQUIRE(is_integer_mapped<int>::value);
    STATIC_REQUIRE_FALSE(is_integer_mapped<std::string>::value);
}
