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
using internal::is_bindable_v;

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
    struct statement_binder<Custom> {};
    template<>
    struct field_printer<Custom> {};
}

TEST_CASE("is_bindable") {
    STATIC_REQUIRE(is_bindable_v<bool>);
    STATIC_REQUIRE(is_bindable_v<char>);
    STATIC_REQUIRE(is_bindable_v<signed char>);
    STATIC_REQUIRE(is_bindable_v<unsigned char>);
    STATIC_REQUIRE(is_bindable_v<short>);
    STATIC_REQUIRE(is_bindable_v<unsigned short>);
    STATIC_REQUIRE(is_bindable_v<int>);
    STATIC_REQUIRE(is_bindable_v<unsigned int>);
    STATIC_REQUIRE(is_bindable_v<long>);
    STATIC_REQUIRE(is_bindable_v<unsigned long>);
    STATIC_REQUIRE(is_bindable_v<float>);
    STATIC_REQUIRE(is_bindable_v<long long>);
    STATIC_REQUIRE(is_bindable_v<unsigned long long>);
    STATIC_REQUIRE(is_bindable_v<double>);
    STATIC_REQUIRE(is_bindable_v<const char*>);
    STATIC_REQUIRE(is_bindable_v<std::string>);
    STATIC_REQUIRE(is_bindable_v<StringVeneer<char>>);
#ifndef SQLITE_ORM_OMITS_CODECVT
    STATIC_REQUIRE(is_bindable_v<const wchar_t*>);
    STATIC_REQUIRE(is_bindable_v<std::wstring>);
    STATIC_REQUIRE(is_bindable_v<StringVeneer<wchar_t>>);
#endif
    STATIC_REQUIRE(is_bindable_v<std::nullptr_t>);
    STATIC_REQUIRE(is_bindable_v<std::unique_ptr<int>>);
    STATIC_REQUIRE(is_bindable_v<std::shared_ptr<int>>);
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
    STATIC_REQUIRE(is_bindable_v<std::string_view>);
#ifndef SQLITE_ORM_OMITS_CODECVT
    STATIC_REQUIRE(is_bindable_v<std::wstring_view>);
#endif
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    STATIC_REQUIRE(is_bindable_v<std::nullopt_t>);
    STATIC_REQUIRE(is_bindable_v<std::optional<int>>);
    STATIC_REQUIRE(is_bindable_v<std::optional<Custom>>);
    STATIC_REQUIRE(!is_bindable_v<std::optional<User>>);
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
    STATIC_REQUIRE(is_bindable_v<static_pointer_binding<std::nullptr_t, carray_pvt>>);
#endif

    STATIC_REQUIRE(is_bindable_v<Custom>);
    STATIC_REQUIRE(is_bindable_v<std::unique_ptr<Custom>>);

    STATIC_REQUIRE(!is_bindable_v<void>);
    STATIC_REQUIRE(!is_bindable_v<internal::literal_holder<int>>);
    STATIC_REQUIRE(!is_bindable_v<User>);
    STATIC_REQUIRE(!is_bindable_v<std::unique_ptr<User>>);
    {
        auto isEqual = is_equal(&User::id, 5);
        STATIC_REQUIRE(!is_bindable_v<decltype(isEqual)>);
    }
    {
        auto notEqual = is_not_equal(&User::id, 10);
        STATIC_REQUIRE(!is_bindable_v<decltype(notEqual)>);
    }
    {
        auto lesserThan = lesser_than(&User::id, 10);
        STATIC_REQUIRE(!is_bindable_v<decltype(lesserThan)>);
    }
    {
        auto lesserOrEqual = lesser_or_equal(&User::id, 5);
        STATIC_REQUIRE(!is_bindable_v<decltype(lesserOrEqual)>);
    }
    {
        auto greaterThan = greater_than(&User::id, 5);
        STATIC_REQUIRE(!is_bindable_v<decltype(greaterThan)>);
    }
    {
        auto greaterOrEqual = greater_or_equal(&User::id, 5);
        STATIC_REQUIRE(!is_bindable_v<decltype(greaterOrEqual)>);
    }
    {
        auto func = datetime("now");
        STATIC_REQUIRE(!is_bindable_v<decltype(func)>);
        bool trueCalled = false;
        bool falseCalled = false;
        auto dummy = 5;  //  for gcc compilation
        internal::static_if<is_bindable_v<decltype(func)>>(
            [&trueCalled](int&) {
                trueCalled = true;
            },
            [&falseCalled](int&) {
                falseCalled = true;
            })(dummy);
        REQUIRE(!trueCalled);
        REQUIRE(falseCalled);
    }
}
