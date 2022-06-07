#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <memory>  //  std::unique_ptr, std::shared_ptr
#include <string>  //  std::string
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  //  std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
#include <string_view>
#endif

using namespace sqlite_orm;
using internal::is_printable_v;

namespace {
    struct Custom {};
    template<class Elem>
    class StringVeneer : public std::basic_string<Elem> {};

    struct User {
        int id;
    };
}

namespace sqlite_orm {
    template<>
    struct field_printer<Custom> {};
}

TEST_CASE("is_printable") {
    STATIC_REQUIRE(is_printable_v<bool>);
    STATIC_REQUIRE(is_printable_v<char>);
    STATIC_REQUIRE(is_printable_v<signed char>);
    STATIC_REQUIRE(is_printable_v<unsigned char>);
    STATIC_REQUIRE(is_printable_v<short>);
    STATIC_REQUIRE(is_printable_v<unsigned short>);
    STATIC_REQUIRE(is_printable_v<int>);
    STATIC_REQUIRE(is_printable_v<unsigned int>);
    STATIC_REQUIRE(is_printable_v<long>);
    STATIC_REQUIRE(is_printable_v<unsigned long>);
    STATIC_REQUIRE(is_printable_v<float>);
    STATIC_REQUIRE(is_printable_v<long long>);
    STATIC_REQUIRE(is_printable_v<unsigned long long>);
    STATIC_REQUIRE(is_printable_v<double>);
    STATIC_REQUIRE(!is_printable_v<const char*>);
    STATIC_REQUIRE(is_printable_v<std::string>);
    STATIC_REQUIRE(is_printable_v<StringVeneer<char>>);
#ifndef SQLITE_ORM_OMITS_CODECVT
    STATIC_REQUIRE(!is_printable_v<const wchar_t*>);
    STATIC_REQUIRE(is_printable_v<std::wstring>);
    STATIC_REQUIRE(is_printable_v<StringVeneer<wchar_t>>);
#endif
    STATIC_REQUIRE(is_printable_v<std::nullptr_t>);
    STATIC_REQUIRE(is_printable_v<std::unique_ptr<int>>);
    STATIC_REQUIRE(is_printable_v<std::shared_ptr<int>>);
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
    STATIC_REQUIRE(!is_printable_v<std::string_view>);
    STATIC_REQUIRE(!is_printable_v<std::wstring_view>);
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    STATIC_REQUIRE(is_printable_v<std::nullopt_t>);
    STATIC_REQUIRE(is_printable_v<std::optional<int>>);
    STATIC_REQUIRE(is_printable_v<std::optional<Custom>>);
    STATIC_REQUIRE(!is_printable_v<std::optional<User>>);
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
    STATIC_REQUIRE(!is_printable_v<static_pointer_binding<std::nullptr_t, carray_pvt>>);
#endif

    STATIC_REQUIRE(is_printable_v<Custom>);
    STATIC_REQUIRE(is_printable_v<std::unique_ptr<Custom>>);

    STATIC_REQUIRE(!is_printable_v<void>);
    STATIC_REQUIRE(!is_printable_v<User>);
    STATIC_REQUIRE(!is_printable_v<std::unique_ptr<User>>);
}
